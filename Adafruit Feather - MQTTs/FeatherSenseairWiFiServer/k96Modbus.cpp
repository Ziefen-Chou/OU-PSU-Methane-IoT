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
#include "k96Modbus.h"

#if PCB_VERSION == 0
  Uart sensorSerial (&sercom1, K96_TX, K96_RX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
  void SERCOM1_Handler(){
    sensorSerial.IrqHandler();
  }
#elif PCB_VERSION == 1
  #define sensorSerial Serial1
#elif PCB_VERSION == 2
  #define sensorSerial Serial1
#endif

k96Modbus::k96Modbus()
{
}

/*
 * Serial Port Output pins TX, RX
 * Baud Rate = 115200, Parity = None, Stop bits = 2
 */
int k96Modbus::init(){
  
  sensorSerial.begin(K96_BAUD,SERIAL_8N2);
  sensorSerial.setTimeout(K96_TIMEOUT);
  if (PCB_VERSION == 0){
    pinPeripheral(K96_TX, PIO_SERCOM);       // Assign TX function to pin
    pinPeripheral(K96_RX, PIO_SERCOM);       // Assign RX function to pin
  }
  pinMode(K96_POWER,OUTPUT);
  digitalWrite(K96_POWER,HIGH);
  delay(1000);                               // Wait for serial port to enable
  
  // getDeviceID();                          // ERROR ACCESSING RAM
  writeCommand(0);                           // Check communication port
  int readStatus = readResponse(14);
  if (readStatus > 14) status = 0;
  else status = -1;
  return status;
  
}

/*
 *  signed 16 bit number
 */

int k96Modbus::wordS16(byte high, byte low){
  int result = ((high*256) + low);
  if (result > 32768) result -= 65535;
  return result;
}

/*
 *  unsigned 16 bit number
 */
int k96Modbus::wordU16(byte high, byte low){
  return ((high*256) + low);
}

/*
 *  unsigned 32 bit number
 */
long k96Modbus::longConvert(byte highest, byte higher, byte lower, byte lowest){
  return ((highest*16777216)+(higher*65536)+(lower*256) + lowest);
}

/*
 *  Read 2 bytes for an unsigned 16 bit number
 */
int k96Modbus::readResponse(){
  byte test[8];
  sensorSerial.readBytes(test,8);
  return wordU16(test[3],test[4]);
}


long k96Modbus::readResponseLong(){
  byte test[10];
  int responseBytes = sensorSerial.readBytes(test,10);
  if (responseBytes){
    status = 0;
    return longConvert(test[3],test[4],test[5],test[6]);
  }
  else{
    status = -1;
    return status;
  }
  
}

/*
 * Read 1 or more consectuive bytes
 */
int k96Modbus::readResponse(int numBytes){
  byte test[6+numBytes];
  int responseBytes = sensorSerial.readBytes(test,6+numBytes);
  k96_memory[0] = wordS16(test[3],test[4]);
  k96_memory[1] = wordS16(test[5],test[6]);
  k96_memory[2] = wordS16(test[7],test[8]);
  k96_memory[3] = wordS16(test[9],test[10]);
  k96_memory[4] = wordS16(test[11],test[12]);
  k96_memory[5] = wordS16(test[13],test[14]);
  k96_memory[6] = wordS16(test[15],test[16]);
  k96_memory[7] = wordS16(test[17],test[18]);
  k96_memory[8] = wordS16(test[19],test[20]);
  k96_memory[9] = wordS16(test[21],test[22]);
  return responseBytes;
}

/***********************************************
 * Serial Write Operation
 * 
 * Writes seven (7) bytes of data to serial port
 * 
 * ERROR: Serial port write will block if data is not received
 *        Check for avaialable buffer size
 * 
************************************************/

int k96Modbus::writeCommand(int byteAddress){
  if (sensorSerial.availableForWrite() >= K96_CMD_SIZE){
    for(int i=0; i < K96_CMD_SIZE; i++){
      sensorSerial.write(command[i+7*byteAddress]);
    }
    return 0;
  }
  return 1;
}

/*
 * Format data into comma seperated value string
 */
String k96Modbus::readCSVString(){
  String dataString = "";
  writeCommand(0);
  int readStatus = readResponse(14);
  if(readStatus > 14){
    for(int i=0; i<6; i++){
      dataString += (String(k96_memory[i]) + ',');
    }
    writeCommand(1);
    k96_memory[6] = readResponse();
    dataString += (String(k96_memory[6]) + ',');
    for(int i=2; i<5; i++){
      writeCommand(i);
      k96_memory[i+5] = readResponse();
      dataString += (String(k96_memory[i+5])+',');
    }
    return dataString;
  }
  else{
    return "bytes,"+String(readStatus)+",-,-,-,-,-,-,-,-,";
  }
  
}
/****************************************************************
 * Updated String Read with reference input
 * 
 * Reads total of 10 bytes
 */

int k96Modbus::readCSVString(String &output){
  
  if (!writeCommand(0)){
    int readStatus = readResponse(20);

    if(readStatus > 20){
      for(int i=0; i<K96_DATA_RAW; i++){
        output.concat(String(k96_memory[i]) + ',');
      }

      // Read the error status byte
      writeCommand(1);
      k96_memory[K96_DATA_RAW] = readResponse();
      output.concat(String(k96_memory[K96_DATA_RAW]) + ',');

      for(int i=2; i<5; i++){
        writeCommand(i);
        k96_memory[(i-2)+ K96_DATA_RAW + 1] = readResponse();
        output.concat(String(k96_memory[(i-2)+ K96_DATA_RAW + 1])+',');
      }
      return 0;
    }
    else if (readStatus > 0){
      output.concat("Error,"+String(readStatus)+",bytes,read,-,-,-,-,-,-,");
      return 1;
    }
    output.concat("Error,No,Sensor,Data,-,-,-,-,-,-,");
    return 2;
  }
  else{
    output.concat("Error,No,Sensor,Serial,Port,-,-,-,-,");
    return 3;
  }
}

/*
 * Returns
 */
String k96Modbus::readByteString(int byteAddress){
  return String(labels[byteAddress]+k96_memory[byteAddress]);
}

/****************************************************************
 * Read the serial number of K96 sensor
 * ERROR: CAN'T READ MEMORY, NOT A MODBUS REGISTER = NOT ALLOWED
 *
 * Return: status
 ****************************************************************/
int k96Modbus::getDeviceID(){
  if (!writeCommand(5)){                    // Command 5 for reading 4 bytes
    deviceID = readResponseLong();          // Sets status variable if read = success
  } 
  return status;
}

String k96Modbus::readSensorID(){
  return deviceID;
}

String k96Modbus::readSensorFW(){
  byte test[10];
  writeCommand(6);
  sensorSerial.readBytes(test,10);
  return (String(test[3])+"."+String(test[4]) +"."+ String(test[5]) +"."+ String(test[6]));
}
