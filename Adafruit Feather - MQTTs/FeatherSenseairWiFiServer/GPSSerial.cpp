/* GPSSerial.cpp - Library for reading and parsing serial NMEA data
 * Copyright (c) 2022 University of Oklahoma.  All right reserved.
 *
 * Uses Feather M0 Serial Com Port "SERCOM2"
 * No serial TX function needed device is constant output
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include "GPSSerial.h"

Uart gps2Serial (&sercom2, GPS_RX, GPS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler(){
  gps2Serial.IrqHandler();
}

// Constructor
GPSSerial::GPSSerial()
{
}

/*
 *  Intialize the hardwareserial port configuration
 *  Device is read-only but must configure TX pin
 *  Device when powered is in constant stream mode
 */

int GPSSerial::init(){
  
  gps2Serial.begin(GPS_BAUD);
  gps2Serial.setTimeout(GPS_TIMEOUT);
  pinPeripheral(GPS_RX, PIO_SERCOM);        // Assign RX function, constant stream
  pinPeripheral(GPS_TX, PIO_SERCOM);        // Assign TX function, unused
  delay(500);
  if (gps2Serial.available() > 0){
    return 0;                               // Stream should begin immediately
  }
  else{
    return -1;                              // No data = no device
  }
}

/*
 *  Read the serial port streaminto the parser
 *  Return latitude, longitude, elevation, and UTC timein CSV string
 */
 
String GPSSerial::readRaw(){
  
  String temp;
  while (gps2Serial.available() > 0){
    temp += String(gps2Serial.read());
  }
  
  return temp;
}

/*
 *  Read the serial port streaminto the parser
 *  Return latitude, longitude, elevation, and UTC timein CSV string
 */
 
String GPSSerial::readResponse(){
  
  while (gps2Serial.available() > 0){
    gps.encode(gps2Serial.read());
  }
  
  return (String(gps.location.rawLat().deg) + '.' + String(gps.location.rawLat().billionths)+','
         +String(gps.location.rawLng().deg) + '.' + String(gps.location.rawLng().billionths)+','
         +String(gps.altitude.meters())+','
         +String(gps.time.hour())+':'+String(gps.time.minute())+':'+String(gps.time.second())+',');
}

/*
 * Returns number with six decimal places (lower precision reading)
 */
float GPSSerial::lat(){
  return gps.location.lat();
}

/*
 * Returns number with six decimal places (lower precision reading)
 */
float GPSSerial::lng(){
  return gps.location.lng();
}

String GPSSerial::time(){
  return (String(gps.time.hour())+':'+String(gps.time.minute())+':'+String(gps.time.second()));
}

String GPSSerial::date(){
  return (String(gps.date.year())+'/'+String(gps.date.month())+'/'+String(gps.date.day()));
}
