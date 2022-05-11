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

int UVOUT = A0; //Output from the sensor
int REF_3V3 = A1; //3.3V power on the Arduino board
 
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
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  while (!bme680.init()) {
      //Serial.println("bme680 init failed ! can't find device!");
      delay(10000);
   }
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("test.csv", FILE_WRITE);
}

void loop() {
  // put your main code here, to run repeatedly:
  int val;
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique18pt7b); 
  spr.setTextColor(TFT_BLUE);
  spr.drawString("BadBytEnvMo", 60 - 15, 10 , 1);// Print the test text in the custom font
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
  myFile = SD.open("test.csv", FILE_APPEND);
  myFile.print((bme680.sensor_result_value.pressure / 1000)*7.5006168);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.temperature);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.humidity);
  myFile.println(",");
  myFile.close();
  
  
//UV sensor
  int uvLevel = averageAnalogRead(UVOUT);
  int refLevel = averageAnalogRead(REF_3V3);
  float outputVoltage = 3.3 / refLevel * uvLevel;
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
  spr.setTextColor(TFT_WHITE);
  spr.drawString("UV:", 230 - 24, 180 -24 , 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24,180,112,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(uvIntensity,230 - 20,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("mW/cm2", 230 + 12, 180+8, 1);
  spr.pushSprite(0, 0);
  delay(10000);
}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
