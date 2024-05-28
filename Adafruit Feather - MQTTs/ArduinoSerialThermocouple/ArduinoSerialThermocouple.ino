/***************************************************
  This is an example for the Adafruit Thermocouple Sensor w/MAX31855K

  Designed specifically to work with the Adafruit Thermocouple Sensor
  ----> https://www.adafruit.com/products/269

  These displays use SPI to communicate, 3 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <SPI.h>
#include "Adafruit_MAX31855.h"

// Default connection is using software SPI, but comment and uncomment one of
// the two examples below to switch between software SPI and hardware SPI:

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5

// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Example creating a thermocouple instance with hardware SPI
// on a given CS pin.
//#define MAXCS   10
//Adafruit_MAX31855 thermocouple(MAXCS);

// Example creating a thermocouple instance with hardware SPI
// on SPI1 using specified CS pin.
//#define MAXCS   10
//Adafruit_MAX31855 thermocouple(MAXCS, SPI1);

void setup() {
  Serial.begin(115200);

  delay(500);
  
  // OPTIONAL: Can configure fault checks as desired (default is ALL)
  // Multiple checks can be logically OR'd together.
  // thermocouple.setFaultChecks(MAX31855_FAULT_OPEN | MAX31855_FAULT_SHORT_VCC);  // short to GND fault is ignored
}

void loop() {
   
   double c = thermocouple.readCelsius();
   if (isnan(c)) {
     Serial.println("Thermocouple fault(s) detected!");
     uint8_t e = thermocouple.readError();
     if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: Thermocouple is open - no connections.");
     if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: Thermocouple is short-circuited to GND.");
     if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: Thermocouple is short-circuited to VCC.");
   } else {
     
     if (Serial.available()){
      Serial.read();
      Serial.println(c);
     }
   }

   delay(200);
}
