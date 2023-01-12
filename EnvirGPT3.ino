//Code developed by Diego Malpica, for the Antarctica

#include <TFT_eSPI.h>
#include "seeed_bme680.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "RTC_SAMD51.h"
#include "DateTime.h"
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define IIC_ADDR  uint8_t(0x76)

Seeed_BME680 bme680(IIC_ADDR); /* IIC PROTOCOL */
//Seeed_BME680 bme680;             /* SPI PROTOCOL */
//Seeed_BME680 bme680(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);/*SPI PROTOCOL*/

RTC_SAMD51 rtc;
TFT_eSPI tft; 
// Stock font and GFXFF reference handle
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite
SensirionI2CScd4x scd4x;

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

File myFile;

//Save data every minute
unsigned long lastMinuteMillis = 0;
const int interval = 60000; // interval at which to save data, in milliseconds (1 minute)

void setup() {
  // put your setup code here, to run once:
  rtc.begin();
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height());
  Serial.begin(115200);
  while (!bme680.init()) {
      //Serial.println("bme680 init failed ! can't find device!");
      delay(10000);
   }
   Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
  
  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("Envir.csv", FILE_WRITE);
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  myFile.println("Date,Time,BP,Temp,HR,Co2,Temp2,HR%");
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
  spr.drawString("Antarctic IX Exp", 60 - 15, 10 , 1);// Print the test text in the custom font
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
  
  uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool isDataReady = false;
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        Serial.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(co2);
    }

  spr.setTextColor(TFT_WHITE);
  spr.drawString("CO2:", 230 -24 , 180 - 24, 1);// Print the test text in the custom font
  spr.drawRoundRect(230 - 24 ,180,80,40,5,TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(co2,230 - 20 ,180+10,1);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("ppm", 230 + 12, 180+8, 1);
    
  myFile = SD.open("Envir.csv", FILE_APPEND);  
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
  myFile.print((bme680.sensor_result_value.pressure / 1000)*7.5006168);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.temperature);
  myFile.print(",");
  myFile.print(bme680.sensor_result_value.humidity);
  myFile.print(",");
  myFile.print(co2);
  myFile.print(",");
  myFile.print(temperature);
  myFile.print(",");
  myFile.print(humidity);
  myFile.println(",");
//  myFile.close();
  
    
  spr.pushSprite(0, 0);
  delay(1000);
}
