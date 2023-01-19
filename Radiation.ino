//Code developed by Diego Malpica, for the Antarctica Project

#include <TFT_eSPI.h>
#include "seeed_bme680.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include <Wire.h>
#include "Adafruit_VEML6075.h"
#include "SD/Seeed_SD.h"
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
Adafruit_VEML6075 uv = Adafruit_VEML6075();

File myFile;

void setup() {
  // put your setup code here, to run once:
  rtc.begin();
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height());
  Serial.begin(115200);
  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;      //calculating multiplier, depend on your log period
  attachInterrupt(0, tube_impulse, FALLING); //define external interrupts 
  
  Serial.println("VEML6075 Full Test");
  if (! uv.begin()) {
    Serial.println("Failed to communicate with VEML6075 sensor, check wiring?");
  }
  Serial.println("Found VEML6075 sensor");

  // Set the integration constant
  uv.setIntegrationTime(VEML6075_100MS);
  // Get the integration constant and print it!
  Serial.print("Integration time set to ");
  switch (uv.getIntegrationTime()) {
    case VEML6075_50MS: Serial.print("50"); break;
    case VEML6075_100MS: Serial.print("100"); break;
    case VEML6075_200MS: Serial.print("200"); break;
    case VEML6075_400MS: Serial.print("400"); break;
    case VEML6075_800MS: Serial.print("800"); break;
  }
  Serial.println("ms");

  // Set the high dynamic mode
  uv.setHighDynamic(true);
  // Get the mode
  if (uv.getHighDynamic()) {
    Serial.println("High dynamic reading mode");
  } else {
    Serial.println("Normal dynamic reading mode");
  }

  // Set the mode
  uv.setForcedMode(false);
  // Get the mode
  if (uv.getForcedMode()) {
    Serial.println("Forced reading mode");
  } else {
    Serial.println("Continuous reading mode");
  }

  // Set the calibration coefficients
  uv.setCoefficients(2.22, 1.33,  // UVA_A and UVA_B coefficients
                     2.95, 1.74,  // UVB_C and UVB_D coefficients
                     0.001461, 0.002591); // UVA and UVB responses
    
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("RadUV.csv", FILE_WRITE);
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  myFile.println("Date,Time,UVA,UVB,UVI,CPM,USv_h");
  //Serial.println("adjust time!");
  rtc.adjust(now);
  now = rtc.now();
}

void loop() {
  // put your main code here, to run repeatedly:
  DateTime now = rtc.now();
  int val;
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique18pt7b); 
  spr.setTextColor(TFT_RED);
  spr.drawString("Radiation+UV", 60 - 15, 10 , 1);// Print the test text in the custom font
  for(int8_t line_index = 0;line_index < 5 ; line_index++)
  {
    spr.drawLine(0, 50 + line_index, tft.width(), 50 + line_index, TFT_GREEN);
  }

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > LOG_PERIOD){
    previousMillis = currentMillis;
    cpm = counts * multiplier; 
  spr.setFreeFont(&FreeSansBoldOblique9pt7b);                 // Select the font
  
 
  Serial.print("Raw UVA reading:  "); Serial.println(uv.readUVA());
  Serial.print("Raw UVB reading:  "); Serial.println(uv.readUVB());
  Serial.print("UV Index reading: "); Serial.println(uv.readUVI());
  
  // cpm
  spr.setTextColor(TFT_WHITE);
  spr.drawString("cpm:", 60 - 24, 100 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(50 - 24,100,150,40,5,TFT_WHITE); 
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(cpm,60 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("cpm", 110 + 12, 100+8, 1);
    
// beta and gamma radiation uSv/h (conversion factor cpm/151 or cpm/153.8)
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Dose:", 230 -24 , 100 - 24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24,100,100,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber((cpm/151),230 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("uSv/h", 230 + 12, 100+8, 1);
    
//  UV Index
  spr.setTextColor(TFT_WHITE);
  spr.drawString("UV Index", 60 - 24, 180 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(uv.readUVI(),60 - 20,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("UV-I", 60 + 12, 180+8, 1);
  
  //SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("RadUV.csv", FILE_APPEND);
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
  myFile.print(uv.readUVA());
  myFile.print(",");
  myFile.print(uv.readUVB());
  myFile.print(",");
  myFile.print(uv.readUVI());
  myFile.print(",");
  myFile.print(cpm);
  myFile.print(",");
  myFile.print(cpm/151);
  myFile.println(",");
//  myFile.close();
    
  spr.pushSprite(0, 0);
  delay(1000);
  }
}
