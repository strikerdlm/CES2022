# CES2022
SpaceAnalogue Environmental Data 2022
Using SeeedStudio WioTerminal and open-source libraries for Arduino, using the TFT display for data for reading and writing data to the SD card. 

1) BME680Wio4.ino uses two sensors via grove ports. The i2c port reads BME680 and the Analog port reads UV light. Writes data into SD card as csv
2) BME680CO2.ino uses two sensors via grove ports. The i2c port reads BME680 and the Analog port reads CO2. Writes data into SD card as csv
3) MultiGasCO2COVOC.ino uses two sensors via grove ports. The i2c port reads Seeed Studio multichannel gas v2 and the Analog port reads CO2. Writes data into SD card as csv
