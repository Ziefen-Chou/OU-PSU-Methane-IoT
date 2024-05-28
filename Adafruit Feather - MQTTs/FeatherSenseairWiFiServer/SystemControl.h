/* SystemControl.h - Library for various IO operations
  
  Functions for enabling the different power converters using IO pins. Provides
  and auxilliary IO pin that can be configured into an output with commands.
  
  Copyright (c) 2023 University of Oklahoma.  All right reserved.

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

#ifndef SystemControl_h
#define SystemControl_h

#define POWER_SENSOR  A1              // Sensor has it's own exterior enable pin
#define POWER_GPS     A2              // GPS uses internal buck converter
#define POWER_FAN     A3              // Cooling fan control
#define POWER_PUMP    A4              // Gas movement control

#define AUX_IO        A5              // Open pin should route to exterior connector

#include <Arduino.h>

class SystemControl
{
private:
  int status = -1;                    // Not implemented

public:
  SystemControl();                    // Creator
  int enablePower(int option);
  int disablePower(int option);

  int readAux();                      // Analog read, returns 10-bit number
  
  int blinkCode(int count);           // Use built-in LED for info display
};

#endif
