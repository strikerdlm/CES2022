//Project with WioTerminal and CO2 sensor in the Analog grove port and Multichannel Gas sensor
//in the i2c port, storing data to the SD card

#include <TFT_eSPI.h>
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
GAS_GMXXX<TwoWire> gas;
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "CO2Sensor.h"
#include "RTC_SAMD51.h"
#include "DateTime.h"

CO2Sensor co2Sensor(A0, 0.99, 100);

RTC_SAMD51 rtc; 
TFT_eSPI tft; 
// Stock font and GFXFF reference handle
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite 

File myFile;
 
void setup() {
  // put your setup code here, to run once:
  rtc.begin();
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height()); 
  gas.begin(Wire, 0x08); // use the hardware I2C
  Serial.begin(115200);
  co2Sensor.calibrate();
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("gas.csv", FILE_WRITE);
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  Serial.println("adjust time!");
  rtc.adjust(now);
  now = rtc.now();
}
 
void loop() {
  // put your main code here, to run repeatedly:
  int vala;
  int valb;
  int valc;
  DateTime now = rtc.now();
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique18pt7b); 
  spr.setTextColor(TFT_RED);
  spr.drawString("BadByteGas", 60 - 15, 10 , 1);// Print the test text in the custom font
  for(int8_t line_index = 0;line_index < 5 ; line_index++)
  {
    spr.drawLine(0, 50 + line_index, tft.width(), 50 + line_index, TFT_GREEN);
  }
 
  spr.setFreeFont(&FreeSansBoldOblique9pt7b);                 // Select the font
  // GM102B NO2 sensor
  int co2 = co2Sensor.read();
  //if (co2 > 999) co2 = 999;
  spr.setTextColor(TFT_WHITE);
  spr.drawString("CO2:", 60 - 24, 100 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,100,80,40,5,TFT_WHITE); 
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(co2,60 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("ppm", 60 + 12, 100+8, 1);
  
  // GM302B C2H5CH sensor
  vala = gas.getGM302B();
  if (vala > 999) vala = 999;
  spr.setTextColor(TFT_WHITE);
  spr.drawString("C2H5CH:", 230 -24 , 100 - 24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24,100,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(vala,230 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("ppm", 230 + 12, 100+8, 1);
  // GM502B VOC sensor
  valb = gas.getGM502B();
  if (valb > 999) valb = 999;
  spr.setTextColor(TFT_WHITE);
  spr.drawString("VOC:", 60 - 24, 180 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(valb,60 - 20,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("ppm", 60 + 12, 180+8, 1);
  // GM702B CO sensor
  valc = gas.getGM702B();
  if (valc > 999) valc = 999;
  spr.setTextColor(TFT_WHITE);
  spr.drawString("CO:", 230 -24 , 180 - 24, 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24 ,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(valc,230 - 20 ,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("ppm", 230 + 12, 180+8, 1);

  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("gas.csv", FILE_APPEND);
  myFile.print(now.year(), DEC);
  myFile.print('/');
  myFile.print(now.month(), DEC);
  myFile.print('/');
  myFile.print(now.day(), DEC);
  myFile.print(",");
  myFile.print(now.hour(), DEC);
  myFile.print(':');
  myFile.print(now.minute(), DEC);
  myFile.print(':');
  myFile.print(now.second(), DEC);
  myFile.print(",");
  myFile.print(vala);
  myFile.print(",");
  myFile.print(valb);
  myFile.print(",");
  myFile.print(valc);
  myFile.print(",");
  myFile.print(co2);
  myFile.println(",");
  myFile.close();
 
  spr.pushSprite(0, 0);
  delay(1000);
 
}
