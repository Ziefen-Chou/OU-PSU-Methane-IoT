/* k96Modbus.cpp - Library for serial data transfer to Senseair K96 NDIR
  Copyright (c) 2022 University of Oklahoma.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include "SystemControl.h"

/*
 * Constructor
 * 
 * Set the direction for the IO pins. Then enable the IO pins.
 */
SystemControl::SystemControl()
{
  pinMode(LED_BUILTIN, OUTPUT);           // Use builtin LED for status blinking
  pinMode(POWER_SENSOR, OUTPUT);
  pinMode(POWER_GPS, OUTPUT);
  pinMode(POWER_FAN, OUTPUT);
  pinMode(POWER_PUMP, OUTPUT);
  pinMode(AUX_IO, INPUT);
  delay(1000);
  digitalWrite(POWER_SENSOR, HIGH);
  digitalWrite(POWER_GPS, HIGH);
  digitalWrite(POWER_FAN, HIGH);
  digitalWrite(POWER_PUMP, HIGH);
}

/*
 * enablePower
 * 
 * Set the selected input to the "on" value
 */

int SystemControl::enablePower(int option)
{
  if (option == 0){
    digitalWrite(POWER_SENSOR, HIGH);
  }
  else if (option == 1){
    digitalWrite(POWER_GPS, HIGH);
  }
  else if (option == 2){
    digitalWrite(POWER_FAN, HIGH);
  }
  else if (option == 3){
    digitalWrite(POWER_PUMP, HIGH);
  }
  else{
    return -1;
  }
  return 0;
}

/*
 * disablePower
 * 
 * Set the selected input to the "off" value
 */
int SystemControl::disablePower(int option)
{
  if (option == 0){
    digitalWrite(POWER_SENSOR, LOW);
  }
  else if (option == 1){
    digitalWrite(POWER_GPS, LOW);
  }
  else if (option == 2){
    digitalWrite(POWER_FAN, LOW);
  }
  else if (option == 3){
    digitalWrite(POWER_PUMP, LOW);
  }
  else{
    status -1;
  }
  return status;
}

/*
 * Read the input
 * 
 * Uses analog read for a 10-bit value (max 1024)
 */
int SystemControl::readAux()
{
  return analogRead(AUX_IO);
}

/*
 * Blink the built-in LED a count number of times
 * 
 * Each blink takes 100 ms (50 ms on)
 */
int SystemControl::blinkCode(int count){
  for (int i=0;i<count;i++){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(50);                       // wait for a second  
  }
  return 0;
}
