#include <Arduino.h>
#include <string>
#include <FS.h>

#include "M5Dial.h"

#include <TFT_eSPI.h>

#include "fonts/AsapFont.h"

//for web
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>


const char* htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <h1>IP: %IP%</h1>
  </body>
</html>
)";

const size_t htmlContentLength = strlen_P(htmlContent);

AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");

int wsClients = 0;

static TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite sprite = TFT_eSprite(&tft);

void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);

  USBSerial.begin(115200);

  //web
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");

  sprite.createSprite(240, 240);
  sprite.fillSprite(0x010B); //
  sprite.loadFont(asap48Font);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("TEST", 50, 50);

  M5Dial.Display.pushImage(0, 0, 240, 240, (uint16_t *)sprite.getPointer());
  M5Dial.update();  

  if(SPIFFS.begin(false)) {
    USBSerial.println("SPIFFS init ok");
  }
  else {
    USBSerial.println("SPIFFS failed to start");
  }

  File f = SPIFFS.open("/index.html", "w");
    if (f) {
      f.print(htmlContent);
      f.close();
  }

  //web
  server.on("/index-dynamic.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    USBSerial.println("Index Dynamic");
    request->send(SPIFFS, "/index.html", "text/html", false, [](const String& var) -> String {
      if (var == "IP")
      WiFi.localIP();
        return String(WiFi.localIP().toString());
      return emptyString;
    });
  });

  server.on("/test.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    USBSerial.println("server test.html");
    request->send(SPIFFS, "/test.html", "text/html", false, [](const String& var) -> String {  
      return emptyString;
    });

  });

  server.serveStatic("/", SPIFFS, "/");

  server.onNotFound([](AsyncWebServerRequest* request) {
    USBSerial.println("Not found");
    request->send(404, "text/plain", "Not found");
  });

  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    (void)len;

    USBSerial.println("WS onEvent");

    if (type == WS_EVT_CONNECT) {
      wsClients++;
      ws.textAll("new client connected");
      USBSerial.println("ws connect");
      client->setCloseClientOnQueueFull(false);
      client->ping();
    } else if (type == WS_EVT_DISCONNECT) {
      wsClients--;
      ws.textAll("client disconnected");
      USBSerial.println("ws disconnect");
    } else if (type == WS_EVT_ERROR) {
      USBSerial.println("ws error");
    } else if (type == WS_EVT_PONG) {
      USBSerial.println("ws pong");
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          USBSerial.printf("ws text: %s\n", (char*)data);
        }
      }
    }
  });

  
  server.addHandler(&ws).addMiddleware([](AsyncWebServerRequest* request, ArMiddlewareNext next) {
    if (ws.count() > 2) {
      // too many clients - answer back immediately and stop processing next middlewares and handler
      request->send(503, "text/plain", "Server is busy");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.begin();
}

uint32_t lastWS = 0;
uint32_t deltaWS = 100;

void loop() {
  static long looptime = 0l;
  static long loopcount = 0l;

  M5Dial.update();

  unsigned long currentMillis = millis();

  if (currentMillis - lastWS >= deltaWS) {
    ws.printfAll("%d", loopcount);
    // for (auto& client : ws.getClients()) {
    //   client.printf("kp%.4f", (10.0 / 3.0));
    // }
    lastWS = millis();
  }

  if( (currentMillis - looptime) > 500) {
    looptime = millis();    

    loopcount ++;
    sprite.createSprite(240, 240);    
    sprite.fillSprite(0x010B); //
    sprite.setTextDatum(TC_DATUM);

    char buff[100];
    snprintf(buff, sizeof(buff), "Count: %d", loopcount);    
    sprite.drawString(buff, 120, 50); 

    M5Dial.Display.pushImage(0, 0, 240, 240, (uint16_t *)sprite.getPointer());
  }


}

