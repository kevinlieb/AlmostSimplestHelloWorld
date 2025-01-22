#include <Arduino.h>
#include <string>
#include <FS.h>

#include "M5Dial.h"

#include <TFT_eSPI.h>

#include "fonts/AsapFont.h"

static TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite sprite = TFT_eSprite(&tft);

void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);

  USBSerial.begin(115200);

  sprite.createSprite(240, 240);
  sprite.fillSprite(0x010B); //
  sprite.loadFont(asap48Font);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("TEST", 50, 50);

  M5Dial.Display.pushImage(0, 0, 240, 240, (uint16_t *)sprite.getPointer());
  M5Dial.update();  
  sleep(1);

}

void loop() {
  static long looptime = 0l;
  static long loopcount = 0l;

  M5Dial.update();

  unsigned long currentMillis = millis();

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

