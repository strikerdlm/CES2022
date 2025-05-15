//Code developed by Diego Malpica, for the Chingaza Project

#include <TFT_eSPI.h>
#include "seeed_bme680.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
// Added Libraries for RTC and Multichannel Gas Sensor
// Using Wio Terminal's built-in RTC library as per specification
#include "RTC_SAMD51.h"
#include "DateTime.h" // DateTime is used by RTC_SAMD51.h
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
#include <math.h> // For powf function

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define IIC_ADDR  uint8_t(0x76)

Seeed_BME680 bme680(IIC_ADDR); /* IIC PROTOCOL */
//Seeed_BME680 bme680;             /* SPI PROTOCOL */
//Seeed_BME680 bme680(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);/*SPI PROTOCOL*/

// RTC Object for Wio Terminal's built-in RTC
RTC_SAMD51 rtc;

// Gas Sensor Object
GAS_GMXXX<TwoWire> gas;

// Sensor circuit voltage (assumed for Wio Terminal)
const float VC_SENSOR_POWER = 3.3f;

// R0_factor stores (VC_SENSOR_POWER / V0_out - 1) where V0_out is voltage in clean air.
// Initialize to a value indicating calibration is needed or failed.
float R0_factor_NO2 = -1.0f;
float R0_factor_C2H5OH = -1.0f;
float R0_factor_VOC = -1.0f;
float R0_factor_CO = -1.0f;

// Geiger Counter Variables
#define GEIGER_PIN D0 // Using D0 as an example, ensure this pin is suitable for interrupts
volatile unsigned long geiger_counts = 0;
unsigned long cpm = 0;
float uSv_h = 0.0f;
const int GEIGER_CONVERSION_FACTOR = 151; // 151 CPM = 1 uSv/h
unsigned long lastGeigerReadTime = 0;

// Gas sensor calibration constants (derived from notepad)
// For NO2 (GM-102B): Rs/R0 = A * C^B (oxidizing)
const float A_NO2 = 1.7f;
const float B_NO2 = 0.23f;
// For C2H5OH (GM-302B): Rs/R0 = A * C^(-B) (reducing)
const float A_C2H5OH = 1.0f;
const float B_C2H5OH = 0.3f;
// For VOC (GM-502B): Rs/R0 = A * C^(-B) (reducing, ethanol equivalent)
const float A_VOC = 1.0f;
const float B_VOC = 0.47f;
// For CO (GM-702B): Rs/R0 = A * C^(-B) (reducing)
const float A_CO = 1.4f;
const float B_CO = 0.29f;

// Default R0 factors if live calibration yields invalid V0_out.
// These are (VC_SENSOR_POWER / Typical_Clean_Air_Vout) - 1.0f
// For NO2 (oxidizing), clean air V0_out is high (low Rs). Rs/RL is low.
// E.g., if V0_out_NO2 is ~3.0V, R0_factor_NO2 = (3.3/3.0)-1 = 0.1
const float DEFAULT_R0_FACTOR_NO2 = 0.1f;
// For reducing gases, clean air V0_out is low (high Rs). Rs/RL is high.
// E.g., if V0_out_CO is ~0.5V, R0_factor_CO = (3.3/0.5)-1 = 5.6
const float DEFAULT_R0_FACTOR_REDUCING = 5.0f;

int UVOUT = A0; //Output from the sensor
int REF_3V3 = A1; //3.3V power on the Arduino board
 
TFT_eSPI tft; 
// Stock font and GFXFF reference handle
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite 

File myFile;

// Define UV intensity levels
const float UV_LOW_THRESHOLD = 2.9; // mW/cm^2
const float UV_MODERATE_THRESHOLD = 5.9; // mW/cm^2
// High is above moderate

void tube_impulse(){       //subprocedure for capturing events from Geiger Kit
  geiger_counts++;
}

void setup() {
  // put your setup code here, to run once:
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(tft.width(),tft.height());
  Serial.begin(115200);
//  while (!Serial);
//  Serial.println("Serial start!!!");
//  delay(100);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    spr.setTextColor(TFT_RED);
    spr.drawString("RTC Error", 10, 10, 2);
    spr.pushSprite(0,0);
    while (1) delay(10);
  }
  // Set the RTC time to the compilation time.
  // This will set the time if the RTC has lost power or is being initialized.
  // For RTC_SAMD51, we typically call adjust in setup.
  Serial.println("Setting RTC to compilation time.");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // The rtc.start() and checks for rtc.initialized() or rtc.lostPower()
  // are not typically used with Seeed's RTC_SAMD51.h in the same way as with Adafruit's RTClib.
  // rtc.begin() and rtc.adjust() handle the setup.

  pinMode(GEIGER_PIN, INPUT_PULLUP); // Initialize Geiger counter pin
  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), tube_impulse, FALLING); // Attach interrupt for Geiger counter

  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  while (!bme680.init()) {
      //Serial.println("bme680 init failed ! can't find device!");
      spr.setTextColor(TFT_RED);
      spr.drawString("BME680 Error", 10, 30, 2);
      spr.pushSprite(0,0);
      delay(10000);
   }

  // Initialize Gas Sensor
  gas.begin(Wire, 0x08); // Default I2C address for Grove Multichannel Gas Sensor v2

  // Warm-up and Calibrate R0 for Gas Sensors
  Serial.println("Gas Sensor Warm-up and R0 Calibration (approx. 3 minutes)...");
  Serial.println("Ensure the sensor is in a clean air environment during this time.");
  tft.fillScreen(TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_YELLOW);
  spr.setFreeFont(&FreeSansBoldOblique9pt7b);
  spr.drawString("Gas Sensor Calibrating...", 20, 100);
  spr.drawString("Please wait (~3 min)", 40, 130);
  spr.pushSprite(0,0);
  
  // Allow some time for sensors to warm up.
  // Note: For optimal R0, much longer warm-up might be needed as per datasheets (hours).
  // This is a practical short warm-up.
  delay(180000); // 3 minutes warm-up
  // Alternative: gas.powerOn(); delay based on preheat_times from library if available

  float V0_out_temp; // Temporary variable for clean air voltage

  Serial.println("Calibrating R0_factor for NO2...");
  V0_out_temp = gas.calcVol(gas.getGM102B());
  if (V0_out_temp > 0.0f && V0_out_temp < VC_SENSOR_POWER) {
    R0_factor_NO2 = (VC_SENSOR_POWER / V0_out_temp) - 1.0f;
    if (R0_factor_NO2 <= 0.001f) { // Factor should be positive. Smallest Rs/RL means V0_out close to Vc.
        Serial.println("Warning: Calculated R0_factor_NO2 is too low/non-positive, using default.");
        R0_factor_NO2 = DEFAULT_R0_FACTOR_NO2;
    }
  } else {
    Serial.print("Warning: Invalid V0_out for NO2 ("); Serial.print(V0_out_temp); Serial.println("V), using default R0_factor.");
    R0_factor_NO2 = DEFAULT_R0_FACTOR_NO2;
  }
  Serial.print("V0_out_NO2 (V): "); Serial.print(V0_out_temp, 3);
  Serial.print(", R0_factor_NO2: "); Serial.println(R0_factor_NO2, 3);

  Serial.println("Calibrating R0_factor for C2H5OH...");
  V0_out_temp = gas.calcVol(gas.getGM302B());
  if (V0_out_temp > 0.0f && V0_out_temp < VC_SENSOR_POWER) {
    R0_factor_C2H5OH = (VC_SENSOR_POWER / V0_out_temp) - 1.0f;
     if (R0_factor_C2H5OH <= 0.001f) { // Factor should be positive and typically > 1 for reducing gases in clean air
        Serial.println("Warning: Calculated R0_factor_C2H5OH is too low/non-positive, using default.");
        R0_factor_C2H5OH = DEFAULT_R0_FACTOR_REDUCING;
    }
  } else {
    Serial.print("Warning: Invalid V0_out for C2H5OH ("); Serial.print(V0_out_temp); Serial.println("V), using default R0_factor.");
    R0_factor_C2H5OH = DEFAULT_R0_FACTOR_REDUCING;
  }
  Serial.print("V0_out_C2H5OH (V): "); Serial.print(V0_out_temp, 3);
  Serial.print(", R0_factor_C2H5OH: "); Serial.println(R0_factor_C2H5OH, 3);

  Serial.println("Calibrating R0_factor for VOC...");
  V0_out_temp = gas.calcVol(gas.getGM502B());
  if (V0_out_temp > 0.0f && V0_out_temp < VC_SENSOR_POWER) {
    R0_factor_VOC = (VC_SENSOR_POWER / V0_out_temp) - 1.0f;
    if (R0_factor_VOC <= 0.001f) {
        Serial.println("Warning: Calculated R0_factor_VOC is too low/non-positive, using default.");
        R0_factor_VOC = DEFAULT_R0_FACTOR_REDUCING;
    }
  } else {
    Serial.print("Warning: Invalid V0_out for VOC ("); Serial.print(V0_out_temp); Serial.println("V), using default R0_factor.");
    R0_factor_VOC = DEFAULT_R0_FACTOR_REDUCING;
  }
  Serial.print("V0_out_VOC (V): "); Serial.print(V0_out_temp, 3);
  Serial.print(", R0_factor_VOC: "); Serial.println(R0_factor_VOC, 3);

  Serial.println("Calibrating R0_factor for CO...");
  V0_out_temp = gas.calcVol(gas.getGM702B());
  if (V0_out_temp > 0.0f && V0_out_temp < VC_SENSOR_POWER) {
    R0_factor_CO = (VC_SENSOR_POWER / V0_out_temp) - 1.0f;
    if (R0_factor_CO <= 0.001f) {
        Serial.println("Warning: Calculated R0_factor_CO is too low/non-positive, using default.");
        R0_factor_CO = DEFAULT_R0_FACTOR_REDUCING;
    }
  } else {
    Serial.print("Warning: Invalid V0_out for CO ("); Serial.print(V0_out_temp); Serial.println("V), using default R0_factor.");
    R0_factor_CO = DEFAULT_R0_FACTOR_REDUCING;
  }
  Serial.print("V0_out_CO (V): "); Serial.print(V0_out_temp, 3);
  Serial.print(", R0_factor_CO: "); Serial.println(R0_factor_CO, 3);

  Serial.println("Gas sensor R0_factor calibration finished.");
  delay(2000); // Brief pause before starting loop

  SD.begin(SDCARD_SS_PIN, SDCARD_SPI);
  myFile = SD.open("test.csv", FILE_WRITE);
  // Write CSV Headers
  if (myFile) {
    myFile.println("Timestamp,Temp_C,Humidity_%,Pressure_mmHg,UV_Intensity_mWcm2,NO2_ppm,C2H5OH_ppm,VOC_ppm,CO_ppm,CPM,uSv_h");
    myFile.close();
  } else {
    Serial.println("error opening test.csv for header");
    spr.setTextColor(TFT_RED);
    spr.drawString("SD Card Error", 10, 50, 2);
    spr.pushSprite(0,0);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  spr.fillSprite(TFT_BLACK); // Clear sprite
  // Use a smaller font for fitting more data
  spr.setFreeFont(&FreeSansBoldOblique9pt7b); // Smaller font

  // Get current time from RTC
  DateTime now = rtc.now();
  String timestamp = "";
  char buffer[20];
  sprintf(buffer, "%02d:%02d:%02d, %02d:%02d", now.day(), now.month(), now.year() % 100, now.hour(), now.minute());
  timestamp = String(buffer);

  // Calculate Geiger Counter CPM and uSv/h
  // Calculate time elapsed since last geiger read for CPM calculation
  unsigned long currentTime = millis();
  unsigned long geigerElapsedTime = currentTime - lastGeigerReadTime;
  lastGeigerReadTime = currentTime;

  // CPM calculation: (counts / elapsed_time_in_minutes)
  // elapsed time is in ms, so convert to minutes: (geigerElapsedTime / 1000.0 / 60.0)
  if (geigerElapsedTime > 0) {
    // Calculate CPM based on counts over the loop's delay period (10 seconds in this case)
    // (geiger_counts / (10 seconds / 60 seconds_per_minute)) = geiger_counts * 6
    cpm = geiger_counts * (60000 / 10000); // (60 seconds / 10 seconds interval)
  } else {
    cpm = 0; // Avoid division by zero if loop runs extremely fast (not expected here)
  }
  uSv_h = (float)cpm / GEIGER_CONVERSION_FACTOR;
  geiger_counts = 0; // Reset counts for the next interval

  // BME680 sensor data
  if (bme680.read_sensor_data()) {
        Serial.println("Failed to perform BME680 reading :(");
        spr.setTextColor(TFT_RED);
        spr.drawString("BME680 Read Error", 10, 10, 2);
        spr.pushSprite(0,0);
        delay(1000); // Wait a bit before retrying
        return;
  }

  float temperature = bme680.sensor_result_value.temperature;
  float humidity = bme680.sensor_result_value.humidity;
  float pressure_mmHg = (bme680.sensor_result_value.pressure / 100.0) * 0.750062; // hPa to mmHg

  // UV sensor data
  int uvLevel = averageAnalogRead(UVOUT);
  int refLevel = averageAnalogRead(REF_3V3);
  float outputVoltage = 3.3 / refLevel * uvLevel;
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0); 
  String uvClassification = "Low";
  if (uvIntensity > UV_MODERATE_THRESHOLD) {
    uvClassification = "High";
  } else if (uvIntensity > UV_LOW_THRESHOLD) {
    uvClassification = "Moderate";
  }

  // Multichannel Gas Sensor data
  // Note: The library provides raw ADC values. For actual ppm, calibration is usually needed.
  // These are example readings; refer to sensor datasheet and calibration for accuracy.
  // float no2_ppm = gas.calcVol(gas.measure_NO2());       // Example, replace with actual calculation if available
  // float c2h5oh_ppm = gas.calcVol(gas.measure_C2H5OH()); // Example
  // float voc_ppm = gas.calcVol(gas.measure_VOC());       // Example
  // float co_ppm = gas.calcVol(gas.measure_CO());         // Example

  // Read current V_out values
  float V_out_NO2 = gas.calcVol(gas.getGM102B());
  float V_out_C2H5OH = gas.calcVol(gas.getGM302B());
  float V_out_VOC = gas.calcVol(gas.getGM502B());
  float V_out_CO = gas.calcVol(gas.getGM702B());

  // Helper function to safely calculate PPM from V_out and calibrated R0_factor
  auto calculatePPM = [](float current_V_out, float calibrated_R0_factor, float A, float B, bool isOxidizing) -> float {
    if (calibrated_R0_factor <= 0.001f) { // Check if R0 calibration was valid
        // Serial.println("Error: R0_factor is invalid, cannot calculate PPM.");
        return 0.0f; 
    }
    if (current_V_out <= 0.0f || current_V_out >= VC_SENSOR_POWER) { // Invalid current voltage
        // Serial.print("Error: Current V_out ("); Serial.print(current_V_out); Serial.println("V) is invalid.");
        return 0.0f; 
    }

    float current_Rs_over_RL = (VC_SENSOR_POWER / current_V_out) - 1.0f;

    // Rs/RL must be positive. If V_out is very close to Vc, current_Rs_over_RL can be near zero or slightly negative due to float precision.
    // Clamp to a very small positive number if it's not expected to be negative.
    if (current_Rs_over_RL <= 0.0f) {
        current_Rs_over_RL = 0.00001f; 
    }

    float effective_Rs_R0_ratio = current_Rs_over_RL / calibrated_R0_factor;

    // Ensure the ratio is positive for calculations.
    if (effective_Rs_R0_ratio <= 0.0f) {
        effective_Rs_R0_ratio = 0.00001f;
    }
    
    float ppm = 0.0f;
    float base = effective_Rs_R0_ratio / A;

    if (base <= 0.0f) { // Base for powf must be positive if exponent is non-integer
        // This indicates either very high concentration (for reducing gases, low Rs/R0)
        // or very low concentration (for oxidizing gases, low Rs/R0)
        // or an issue with R0_factor or A.
        // For simplicity, return 0, but could indicate error or max/min range.
        return 0.0f;
    }

    if (isOxidizing) { // e.g., NO2: Rs/R0 = A * C^B  => C = (base)^(1/B)
      if (A == 0.0f || B == 0.0f) return 0.0f;
      ppm = powf(base, 1.0f / B);
    } else { // Reducing gases: Rs/R0 = A * C^(-B) => C = (base)^(-1/B)
      if (A == 0.0f || B == 0.0f) return 0.0f;
      ppm = powf(base, -1.0f / B);
    }
    
    if (isnan(ppm) || isinf(ppm)) {
        // Serial.println("PPM calculation resulted in NaN or Inf.");
        return 0.0f; // Or an error code
    }
    return ppm;
  };
  
  float no2_ppm = calculatePPM(V_out_NO2, R0_factor_NO2, A_NO2, B_NO2, true);
  float c2h5oh_ppm = calculatePPM(V_out_C2H5OH, R0_factor_C2H5OH, A_C2H5OH, B_C2H5OH, false);
  float voc_ppm = calculatePPM(V_out_VOC, R0_factor_VOC, A_VOC, B_VOC, false);
  float co_ppm = calculatePPM(V_out_CO, R0_factor_CO, A_CO, B_CO, false);

  // Constrain PPM values to typical sensor ranges or expected max values
  no2_ppm = constrain(no2_ppm, 0.0f, 10.0f);
  c2h5oh_ppm = constrain(c2h5oh_ppm, 0.0f, 500.0f);
  voc_ppm = constrain(voc_ppm, 0.0f, 500.0f); // Ethanol equivalent
  co_ppm = constrain(co_ppm, 0.0f, 1000.0f); // Max for display, sensor up to 5000

  // Display data on TFT
  int yPos = 10; // Initial Y position for text
  int yIncrement = 20; // Y increment for each line

  spr.setTextColor(TFT_WHITE);
  // Timestamp
  spr.drawString("Time: " + timestamp, 5, yPos);
  yPos += yIncrement;

  // Temperature
  spr.drawString("Temp: " + String(temperature, 1) + " C", 5, yPos);
  yPos += yIncrement;

  // Humidity
  spr.drawString("HR%: " + String(humidity, 1) + " %", 5, yPos);
  yPos += yIncrement;
  
  // Barometer
  spr.drawString("Barometer: " + String(pressure_mmHg, 2) + " mmHg", 5, yPos);
  yPos += yIncrement;

  // UV Intensity
  spr.drawString("UV: " + String(uvIntensity, 2) + " mW/cm2 (" + uvClassification + ")", 5, yPos);
  yPos += yIncrement;

  // Updated Gas Sensor Display
  String no2_str = "NO2: " + String(no2_ppm, 2) + "ppm (Ref:<0.1)";
  spr.drawString(no2_str, 5, yPos);
  yPos += yIncrement;

  String c2h5oh_str = "C2H5OH: " + String(c2h5oh_ppm, 1) + "ppm (Ref:~0)";
  spr.drawString(c2h5oh_str, 5, yPos);
  yPos += yIncrement;
  
  String voc_str = "VOC: " + String(voc_ppm, 2) + "ppm (Ref:<0.3)";
  spr.drawString(voc_str, 5, yPos);
  yPos += yIncrement;

  String co_str = "CO: " + String(co_ppm, 1) + "ppm (Ref:<9)";
  spr.drawString(co_str, 5, yPos);
  yPos += yIncrement;

  // Geiger Counter Display
  String cpm_str = "CPM: " + String(cpm);
  spr.drawString(cpm_str, 5, yPos);
  yPos += yIncrement;

  String uSv_h_str = "uSv/h: " + String(uSv_h, 2);
  spr.drawString(uSv_h_str, 5, yPos);
  yPos += yIncrement;

  // ---- BEGIN ADDED SERIAL OUTPUT ----
  Serial.println("START_DATA"); // Marker for start of data block
  Serial.println("TIMESTAMP:" + timestamp);
  Serial.println("TEMP:" + String(temperature, 2) + " C");
  Serial.println("HUMIDITY:" + String(humidity, 2) + " %");
  Serial.println("PRESSURE:" + String(pressure_mmHg, 2) + " mmHg");
  Serial.println("UV:" + String(uvIntensity, 2) + " mW/cm2");
  Serial.println("NO2:" + String(no2_ppm, 3) + " ppm");
  Serial.println("C2H5OH:" + String(c2h5oh_ppm, 3) + " ppm");
  Serial.println("VOC:" + String(voc_ppm, 3) + " ppm");
  Serial.println("CO:" + String(co_ppm, 3) + " ppm");
  Serial.println("CPM:" + String(cpm));
  Serial.println("USVH:" + String(uSv_h, 2) + " uSv/h");
  Serial.println("END_DATA"); // Marker for end of data block
  // ---- END ADDED SERIAL OUTPUT ----

  // Write data to SD card
  myFile = SD.open("test.csv", FILE_APPEND);
  if (myFile) {
    myFile.print(timestamp);
    myFile.print(",");
    myFile.print(temperature, 2); // Two decimal places for temperature
    myFile.print(",");
    myFile.print(humidity, 2);    // Two decimal places for humidity
    myFile.print(",");
    myFile.print(pressure_mmHg, 2); // Two decimal places for pressure
    myFile.print(",");
    myFile.print(uvIntensity, 2);   // Two decimal places for UV
    myFile.print(",");
    myFile.print(no2_ppm, 3);     // Three decimal places for gas sensor
    myFile.print(",");
    myFile.print(c2h5oh_ppm, 3);
    myFile.print(",");
    myFile.print(voc_ppm, 3);
    myFile.print(",");
    myFile.print(co_ppm, 3);
    myFile.print(",");
    myFile.print(cpm);
    myFile.print(",");
    myFile.print(uSv_h, 2); // Two decimal places for uSv/h
    myFile.println();
    myFile.close();
  } else {
    Serial.println("error opening test.csv for append");
    spr.setTextColor(TFT_RED);
    spr.drawString("SD Append Err", 5, yPos);
  }
  
  spr.pushSprite(0, 0);
  delay(1000); // Update every 1 second
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

// Helper to format numbers with a specific width and precision for display
// Not strictly needed with String formatting but can be useful
// String formatNumber(float num, int width, int prec) {
// char buff[20];
// dtostrf(num, width, prec, buff);
// return String(buff);
// }
