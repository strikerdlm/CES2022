//Code developed by Diego Malpica, for the Chingaza Project

#include <TFT_eSPI.h>
#include "seeed_bme680.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define IIC_ADDR  uint8_t(0x76)

Seeed_BME680 bme680(IIC_ADDR); /* IIC PROTOCOL */
//Seeed_BME680 bme680;             /* SPI PROTOCOL */
//Seeed_BME680 bme680(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);/*SPI PROTOCOL*/

const int GSR=A0;
int sensorValue=0;
int gsr_average=0;

TFT_eSPI tft; 
// Stock font and GFXFF reference handle
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite 

File myFile;

void setup() {
  // put your setup code here, to run once:
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height());
  Serial.begin(115200);
//  while (!Serial);
//  Serial.println("Serial start!!!");
//  delay(100);
  while (!bme680.init()) {
      //Serial.println("bme680 init failed ! can't find device!");
      delay(10000);
   }
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("EnvGSR.csv", FILE_WRITE);
}

void loop() {
  // put your main code here, to run repeatedly:
  long sum=0;
  for(int i=0;i<10;i++)           //Average the 10 measurements to remove the glitch
      {
      sensorValue=analogRead(GSR);
      sum += sensorValue;
      delay(5);
      }
   gsr_average = sum/10;
  int val;
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique18pt7b); 
  spr.setTextColor(TFT_RED);
  spr.drawString("BadByteEnv", 60 - 15, 10 , 1);// Print the test text in the custom font
  for(int8_t line_index = 0;line_index < 5 ; line_index++)
  {
    spr.drawLine(0, 50 + line_index, tft.width(), 50 + line_index, TFT_GREEN);
  }
 
  spr.setFreeFont(&FreeSansBoldOblique9pt7b);                 // Select the font
  // BME680 sensor
  if (bme680.read_sensor_data()) {
        Serial.println("Failed to perform reading :(");
        return;
    }
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Temp:", 60 - 24, 100 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,100,80,40,5,TFT_WHITE); 
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(bme680.sensor_result_value.temperature,60 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("C", 60 + 12, 100+8, 1);
    
// Baro
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Barometer:", 230 -24 , 100 - 24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24,100,100,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber((bme680.sensor_result_value.pressure / 1000)*7.5006168,230 - 20,100+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("mmHg", 230 + 12, 100+8, 1);
    
//  HR%
  spr.setTextColor(TFT_WHITE);
  spr.drawString("HR%:", 60 - 24, 180 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(60 - 24,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(bme680.sensor_result_value.humidity,60 - 20,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("%", 60 + 12, 180+8, 1);
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("EnvGSR.csv", FILE_APPEND);
  myFile.print((bme680.sensor_result_value.pressure / 1000)*7.5006168);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.temperature);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.humidity);
  myFile.print(",");
  myFile.print(gsr_average);
  myFile.println(",");
  myFile.close();

// GSR
  spr.setTextColor(TFT_WHITE);
  spr.drawString("GSR:", 230 -24 , 180 - 24, 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24 ,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(gsr_average,230 - 20 ,180+10,1);
  spr.setTextColor(TFT_GREEN);
  //spr.drawString("GSR", 230 + 12, 180+8, 1);
  
  spr.pushSprite(0, 0);
  delay(1000);
}
