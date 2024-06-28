/* k96Modbus.h - Library for serial data transfer to Senseair K96 NDIR
  
  Senseair K96 sensors use modbus protocol to transfer data over serial.
  Each command is 7 bytes that request a memory transfer of a given number of bytes.
  Each response is 8 bytes, the reply plus carriage return
  The error status register is 16 bits at memory location 0x0A
  
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

#ifndef k96Modbus_h
#define k96Modbus_h

#define K96_BAUD        115200
#define K96_TIMEOUT     200
#define K96_CMD_SIZE    7       // Modbus write operations are 7 bytes long
#define K96_DATA_SIZE   12      // Modbus read operations are 6 + "reply" bytes long
#define K96_DATA_RAW    10

#define PCB_VERSION               2

#if PCB_VERSION == 0
  #define K96_POWER      5      // Device enable pin
  #define K96_TX        11      // Sercom1 receive pin
  #define K96_RX        10      // Sercom1 transmit pin
#elif PCB_VERSION == 1
  #define K96_POWER     A0      // Device enable pin
  #define K96_TX         0      // Sercom1 receive pin
  #define K96_RX         1      // Sercom1 transmit pin
#elif PCB_VERSION == 2
  #define K96_POWER     A0      // Device enable pin
  #define K96_TX         0      // Sercom1 receive pin
  #define K96_RX         1      // Sercom1 transmit pin
#endif

#include <Arduino.h>            // required before wiring_private.h
#include "wiring_private.h"     // pinPeripheral() function

class k96Modbus
{
private:
    
    // Command structure: first two bytes x68, x44
    //                    third byte is function
    //                    fourth byte is address
    //                    fifth byte is "read" count
    //                    last two bytes are "Modbus" crc codes
    // CRC Code Generator: https://www.lammertbies.nl/comm/info/crc-calculation

    char command[63] = {0x68,0x44,0x00,0x00,0x14,0x51,0x36,   // Read twenty (20) bytes of data, from memory location 0
                        0x68,0x44,0x00,0x1C,0x02,0xD8,0x38,   // Read two (2) bytes
                        0x68,0x44,0x03,0xA4,0x02,0x5A,0x38,   // Read two (2) bytes
                        0x68,0x44,0x04,0x24,0x02,0x8A,0x39,   // Read two (2) bytes
                        0x68,0x44,0x04,0xA4,0x02,0xEB,0xF9,   // Read two (2) bytes
                        0x68,0x44,0x00,0x28,0x04,0x4E,0xFA,   // Read four (4) bytes, memory location x28 = 40 (ERROR RAM READ)
                        0x68,0x44,0x00,0x62,0x04,0x79,0x9A};  // Read four (4) bytes
      
    String labels[12] = {"<br>CH4: ","<br>CO2: ","<br>H2O: ","<br>Pressure: ","<br>T0: ",
                        "<br>T1: ", "<br>T2: ", "<br>Humidity: ", "<br>T3: ", "<br>D0: ",
                        "<br>D1: ", "<br>D2: "};
      
    int status = -1;
    String deviceID = "";
    
    int getDeviceID();                                        // ERROR attempting to read RAM
    
    int k96_memory[K96_DATA_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12};
    long k96MemoryLong[3] = {1,1,1};
    int wordS16(byte high, byte low);                         // Convert Modbus message, signed 16-bit integer
    int wordU16(byte high, byte low);                         // Convert Modbus message, unsigned 16-bit integer
    long longConvert(byte highest, byte higher, byte lower, byte lowest);
 
public:
    k96Modbus();
        
    int init();                                   // Create serial port and digital IO
    int readResponse();                           // Read 16-bit data
    long readResponseLong();                      // Read 32-bit data
    int readResponse(int numBytes);    
    int writeCommand(int byteAddress);            // Currently seven commands (0-6)
    
    String readCSVString();                       // Legacy data read
    int readCSVString(String &resultString);      // Faster implementation uses pointer
      
    String readByteString(int byteAddress);
    String readSensorID();                        // DOESN"T WORK TRYING TO READ RAM
    String readSensorFW();                        // DOESN"T WORK TRYING TO READ RAM
};

#endif
