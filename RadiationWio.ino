//Code developed by Diego Malpica, as Environmental Sensor for space habitats

#include <TFT_eSPI.h>
#include "seeed_bme680.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "CO2Sensor.h"
#include "RTC_SAMD51.h"
#include "DateTime.h"
#define LOG_PERIOD 15000  //Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000  //Maximum logging period without modifying this sketch

unsigned long counts;     //variable for GM Tube events
unsigned long cpm;        //variable for CPM
unsigned int multiplier;  //variable for calculation CPM in this sketch
unsigned long previousMillis;  //variable for time measurement



void tube_impulse(){       //subprocedure for capturing events from Geiger Kit
  counts++;
}


RTC_SAMD51 rtc;
TFT_eSPI tft; 
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite 

File myFile;

void setup() {
  // put your setup code here, to run once:
  pinMode(WIO_LIGHT, INPUT);
  rtc.begin();
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height());
  Serial.begin(115200);
  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;      //calculating multiplier, depend on your log period
  attachInterrupt(0, tube_impulse, FALLING); //define external interrupts 
 
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("rad.csv", FILE_WRITE);
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  Serial.println("adjust time!");
  rtc.adjust(now);
  now = rtc.now();
}

void loop() {
  // put your main code here, to run repeatedly:
  int val;
  DateTime now = rtc.now();
  int light = analogRead(WIO_LIGHT);
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique18pt7b); 
  spr.setTextColor(TFT_RED);
  spr.drawString("RadiationIXE", 60 - 15, 10 , 1);// Print the test text in the custom font
  for(int8_t line_index = 0;line_index < 5 ; line_index++)
  {
    spr.drawLine(0, 50 + line_index, tft.width(), 50 + line_index, TFT_GREEN);
  }

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > LOG_PERIOD){
    previousMillis = currentMillis;
    cpm = counts * multiplier;
  spr.setFreeFont(&FreeSansBoldOblique9pt7b);                 // Select the font
  spr.setTextColor(TFT_WHITE);
  spr.drawString("cpm:", 60 - 24, 100 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,100,80,40,5,TFT_WHITE); 
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(cpm,60 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("cpm", 60 + 12, 100+8, 1);
     
// Dose
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Dose:", 230 -24 , 100 - 24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24,100,100,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber((cpm/151),230 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("uSv/h", 230 + 12, 100+8, 1);
  
  
////  HR%
//  spr.setTextColor(TFT_WHITE);
//  spr.drawString("HR%:", 60 - 24, 180 -24 , 1);// Print the test text in the custom font
//  spr.drawRoundRect(60 - 24,180,80,40,5,TFT_WHITE);
//  spr.setTextColor(TFT_WHITE);
//  spr.drawNumber(bme680.sensor_result_value.humidity,60 - 20,180+10,1);
//  spr.setTextColor(TFT_GREEN);
//  spr.drawString("%", 60 + 12, 180+8, 1);
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("rad.csv", FILE_APPEND);
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
  myFile.print(cpm);
  myFile.print(",");
  myFile.print(cpm/151);
  myFile.print(",");
//  myFile.print(bme680.sensor_result_value.humidity);
//  myFile.print(",");
//  myFile.print(co2);
//  myFile.print(",");
//  myFile.print(light);
  myFile.println(",");
  myFile.close();
  
  
////CO Sensor
//  spr.setTextColor(TFT_WHITE);
//  spr.drawString("CO2:", 230 - 24, 180 -24 , 1);// Print the test text in the custom font
//  spr.drawRoundRect(230 - 24,180,112,40,5,TFT_WHITE);
//  spr.setTextColor(TFT_WHITE);
//  spr.drawNumber(co2,230 - 20,180+10,1);
//  spr.setTextColor(TFT_GREEN);
//  spr.drawString("ppm", 230 + 12, 180+8, 1);
  spr.pushSprite(0, 0);
  delay(10000);
  }
}
