/*
  SimModem.cpp - Library for sending AT+ commans to the Simcom7070G modem
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

#include "SimModem.h"

Uart SimModemSerial (&sercom3, modemRX, modemTX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM3_Handler()
{
  SimModemSerial.IrqHandler();
}

// Constructor
SimModem::SimModem()
{
}

/*
 *  initialization function for internal hardware and device operation
 *  Sets the serial port parameters
 *  Start or join active network connection
 *  Check the power status of device
 *  Check network status
 *  Configure NTP server
 *  Set internal clock
 *  Configure FTP
 */
int SimModem::init()
{
  
  SimModemSerial.begin(modemBaud);            // Modems have different UART speeds
  SimModemSerial.setTimeout(modemTimeout);    // Timeout should be minimum
  pinPeripheral(modemRX, PIO_SERCOM);         // Assign RX function
  pinPeripheral(modemTX, PIO_SERCOM);         // Assign TX function
  pinMode(MODEM_POWER,OUTPUT);                // Control pin for power on, off, and reset
  digitalWrite(MODEM_POWER, MODEM_PWR_OFF);   // Set to "off" value (default != "off" for all devices)
  // digitalWrite(MODEM_POWER, MODEM_PWR_ON);   // Set to "off" value (default != "off" for all devices)
  powerOn();                                  // Need to check for device on, off, not present
  
  // Enable the modem (do not cycle the power if not needed)
  if (!startSession()){
    return 0;
  }
  return -1;
}

/******************************************************************
// Empty string indicates communication error
// The word error indicates a programming error
******************************************************************/
String SimModem::readResponse(String command, int waitTime){
  SimModemSerial.flush();
  SimModemSerial.println(command);
  if (waitTime){
    SimModemSerial.setTimeout(waitTime);
  }
  else {
    SimModemSerial.setTimeout(modemTimeout);
  }
  int bufferLength = SimModemSerial.readBytes(buffer, 256);
  if (bufferLength){
    String responseString = String(buffer);
    responseString.trim();
    if (responseString.indexOf("SMSUB") >= 0){
      Serial.println("MQTT SUB");
    }
    if (responseString.endsWith("OK")){
      if (bufferLength == 6){
        return "OK";
      }
      else{
        return responseString.substring(0,bufferLength-2);
      }
    }
    else {
      return responseString.substring(0,responseString.indexOf('\n',1));
    }
  }
  else{
    return "";   // Timeout occurred
  }
  return "";
}

/* Empty string indicates communication error
 * The word error indicates a programming error
 */
String SimModem::readWaitResponse(String command, int waitTime, String back){
  
  String responseString = "";
  String bytesReady = "";
  
  if (command.length() >0){
    SimModemSerial.flush();
    SimModemSerial.println(command);                                                // Send AT command to modem
  }

  if (waitTime){    SimModemSerial.setTimeout(waitTime);  }                       // Choose timeout option
  else {            SimModemSerial.setTimeout(modemTimeout);  }                   // Default timeout option

  memset(buffer, 0, sizeof buffer);                                               // Clear the data buffer
  int bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);    // Read first line of bytes

  while (bufferLength > 0){                                                       // Data was available
    memset(buffer, 0, sizeof buffer);                                             // Clear the data buffer
    bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);      // Read incoming data
    responseString = String(buffer);

    responseString = responseString.substring(0,bufferLength);
    int backIndex = responseString.indexOf(back);
    if (responseString.indexOf("SMSUB") >= 0){
      Serial.println("MQTT SUB");
    }
    if (backIndex >= 0){
      return responseString.substring(backIndex+back.length(),responseString.indexOf('\n',backIndex));
    }
    else if (responseString.indexOf("ERROR") >= 0){
      return "ERROR";
    }
  }
  return "";   // Timeout occurred
}


/* Empty string indicates communication error
 * The word error indicates a programming error
 */
String SimModem::readBurst(String command, int waitTime, String back){
   String responseString = "";
   int bytesToRead = 0;
  
  SimModemSerial.println(command);                                                // Send AT command to modem

  if (waitTime){    SimModemSerial.setTimeout(waitTime);  }                       // Choose timeout option
  else {            SimModemSerial.setTimeout(modemTimeout);  }                   // Default timeout option

  memset(buffer, 0, sizeof buffer); 
  int bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);
  responseString = String(buffer);
  
  while (bufferLength > 0){
    Serial.println(responseString);                                               // Only print lines without "back"
    bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);
    responseString = String(buffer);
    
    responseString = responseString.substring(0,bufferLength);
    int backIndex = responseString.indexOf(back);
    if (backIndex >= 0){                                                          // Received "back" response
      responseString = responseString.substring(backIndex+back.length(),responseString.indexOf('\n',backIndex));
      bytesToRead = responseString.substring(responseString.indexOf(',')+1).toInt();
      return "";
    }
    
  }
  
  if (bytesToRead){
    
      SimModemSerial.readBytes(buffer, bytesToRead);
      responseString = String(buffer);
      bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);
      return responseString.substring(0,bytesToRead);
    }
    else{
      bufferLength = SimModemSerial.readBytesUntil('\n',buffer, MODEM_BUFFER);
      return "";   // Timeout occurred
    }

  
  return "";
}

/*  "powerToggle" the Modem using digital trigger
 *  
 *  Modem power pin toggles operation MUST check if device is active.
 *  Use the ATE_OFF string to enforce interface conditions
 */

void SimModem:: powerToggle(){
  int pulses = 1;
  String responseString = readResponse(ATE_OFF,0);
  if (responseString.length() > 0) pulses = 2;      // Device is on and communicating, turn off then on

  for (int i=0; i<pulses; i++){
    digitalWrite(MODEM_POWER, MODEM_PWR_OFF);       // Value should be low already    
    digitalWrite(MODEM_POWER, MODEM_PWR_ON);        // Start of "ON" pulse
    delay(1500);                                    // Datasheet Ton mintues = 1.2S
    digitalWrite(MODEM_POWER, MODEM_PWR_OFF);       // Return output state to low
    delay(1500);
  }

  while (responseString.indexOf("NORMAL") >= 0 | responseString.indexOf("ATE") >= 0){
    responseString = readResponse(ATE_OFF,0);
  }
}

/*  "powerOn" the Modem using digital trigger
 *  
 *  Modem power pin toggles operation MUST check if device is active.
 *  Use the ATE_OFF string to enforce interface conditions
 */
int SimModem::powerOn(){
  String responseString;
  if (status >= -1){
    // Check if device is on by disabling echo mode
    SimModemSerial.flush();
    responseString = readResponse(ATE_OFF,0);
    responseString = readResponse(ATE_OFF,0);
    responseString = readResponse(ATE_OFF,0);
    if(responseString != ""){
      return 0;
    }
  }
  
  // Device is not in reset, toggle IO pin and start timer
  else{
    responseString = readResponse(ATE_OFF,0);
    if(responseString != ""){
      return 0;
    }
    powerToggle();
  }
  return 0;   // Operation occured normally
}

/*  "startSession" of the Modem internet conncection
 * 
 *  Can be used as network status check, returns 0
 *  
 *  Check if network connected (CGATT: 1)
 *  Check if network active
 *  Disables RF functionality to setup network config.
 *  
 *  Output: Status = 0, everything OK
 *          Status = 1, network error
 *          Status = 2, NTP error
 *          Status = -1, modem error 
 */
int SimModem::startSession(){

  int returnValue = 0;
  checkStatus();
  if (status == MODEM_STATUS_ERROR || status == MODEM_STATUS_ERROR){
    return -1;
  }

  // Attempt to turn on the modem if no communication
  if (status == MODEM_STATUS_OFF){
    powerOn();
  }
    
  String networkStatus = readWaitResponse(AT_CGATT,0,"CGATT:");
  if (networkStatus.indexOf("1") < 0){
    // Enable the modem (do not cycle the power)
    if(powerOn()){
      status = -1;
      return -1;
    }    
  }

  // Check for network connection, if connected synchronize time
  returnValue = enableIP();
  if (returnValue >= 0){
    returnValue = startNTP();
    if (returnValue > 0){
      return 1;
    }
    returnValue = startFTP();
    if (returnValue > 0){
      return 1;
    }
    returnValue = startMQTT(1);
    if (returnValue > 0){
      return 1;
    }
    imei = readResponse(AT_GSN,0);
    imei = imei.substring(0,15) + "_";
    return 0;
  }
  
  imei = "none";
  return 1;
}

/*  "startNTPSession" of the Modem internet conncection
 * 
 *  Disables RF functionality to setup network config.
 *  No outputs currently.
 */
int SimModem::startNTP(){
  String responseString;
  readResponse(AT_NTP_ID,0);                                // Choose the clock configuration
  readResponse(AT_NTP_SET,0);                               // Set the NTP server and time zone
  responseString = readWaitResponse(AT_NTP_UP,0,"CNTP:");   // Update the internal clock, wait for confimation
  if (responseString.indexOf("1,\"")){
    return 0;
  }
  else{
    return 1;
  }
}

/*  "startFTPSession" of the Modem internet conncection
 * 
 *  Disables RF functionality to setup network config.
 *  No outputs currently.
 */
int SimModem::startFTP(){
  
  String responseString;
  // From Simcom Application Note EXCEPT using PDP 1
  responseString = readWaitResponse(AT_FTP_STA,100,"FTPSTATE:");

  // Previous operations may have left an active FTP server connection
  if (responseString.indexOf("1") >= 0){
    responseString = readResponse(AT_FTP_EXT,100);
    delay(MODEM_CMD_DELAY);
  }

  responseString = readResponse(AT_FTP_CID,100);      // Set the internet connection ID (determined by typedef)
  delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_SRV,100);      // Choose the FTP server (name or IP address)
  delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_MOD,100);      // Choose the FTP connection mode (active vs. passive)
  delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_TYP,100);      // Choose the FTP data type (ASCII vs. binary)
  delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_UN,100);       // Set the username (do not leave blank, can be "anonymous")
  delay(MODEM_CMD_DELAY);
  //responseString = readResponse(AT_FTP_PWD,100);      // Password will default to empty string
  //delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_PRT,100);      // Choose the FTP port (default = 21)
  delay(MODEM_CMD_DELAY);
  responseString = readResponse(AT_FTP_PUT_NEW,100);  // Choose the FTP file mode (default = APPEND)
  delay(MODEM_CMD_DELAY);

  readResponse(AT_FTP_GET_NAM,0);                     // Set the file name for "read" operations, includes list details
  delay(MODEM_CMD_DELAY);
  readResponse(AT_FTP_GET_PTH,0);                     // Set the path for "read" operations, includes list
  delay(MODEM_CMD_DELAY);
  readResponse(AT_FTP_PUT_PTH,0);                     // Set the path for "write" operations should end with folder separator
  delay(MODEM_CMD_DELAY);
  
  return 0;
}

/*
 * Set the MQTT configuration
 * Input: option
 *    0 = HiveMQ server
 *    1 = Unencrypted test server (mosquitto.org)
 * Return NEMA string
 */
int SimModem::startMQTT(int option){
  if (option ==0){
    readResponse(MQTT_SERVER,0);
    readResponse(MQTT_UN,0);
    readResponse(MQTT_PWD,0);
    readResponse(MQTT_ID,0);
    readResponse(AT_MQT_TIM,0);
    readResponse(AT_MQT_CSS,0);
  }
  // Option 1 is anonymous, no encryption test
  else if (option == 1){
    readResponse(MQTT_SERVER_TEST_BASIC,0);
    readResponse(MQTT_UN_ANON,0);
    readResponse(MQTT_PWD_ANON,0);
    readResponse(MQTT_ID,0);
    readResponse(AT_SSL_SNI_MOSQ,0);
    readResponse(AT_MQT_TIM,0);
    readResponse(AT_MQT_CSS,0);
    readResponse(AT_MQT_TOP,0);
  }
  else if (option == 2){
    readResponse(MQTT_SERVER_TEST_SSL,0);
    readResponse(MQTT_UN_ANON,0);
    readResponse(MQTT_PWD_ANON,0);
    readResponse(MQTT_ID,0);
    readResponse(AT_SSL_SNI_MOSQ,0);
    readResponse(AT_MQT_TIM,0);
    readResponse(AT_MQT_CSS,0);
    readResponse(AT_MQT_TOP,0);
  }
  else if (option == 3){
    readResponse(MQTT_SERVER_GUST_SSL,0);
    readResponse(MQTT_UN,0);
    readResponse(MQTT_PWD,0);
    readResponse(MQTT_ID,0);
    // readResponse(AT_SSL_SNI_MOSQ,0);
    readResponse(AT_MQT_TIM,0);
    readResponse(AT_MQT_CSS,0);
    readResponse(AT_MQT_TOP_STAB,0);
    // readResponse(MQTT_SSLFILECALL,0);
  }
  else if (option == 4){
    readResponse(MQTT_SERVER,0);
    readResponse(MQTT_UN,0);
    readResponse(MQTT_PWD,0);
    readResponse(MQTT_ID_App_Encry,0);
    // readResponse(AT_SSL_SNI_MOSQ,0);
    readResponse(AT_MQT_TIM,0);
    readResponse(AT_MQT_CSS,0);
    readResponse(AT_MQT_TOP_AppEncry,0);
    // readResponse(MQTT_SSLFILECALL,0);
  }
  statusMQTT = MODEM_STATUS_MQTT_CFG;
  return 0;
}

/******************************************************************************
 * MQTT Connect to Server
 * 
 * Attempts to connect without checking status
 * On error checks for reason (active connection is considered success)
 * Sets status variable if connection active
 *
 * Return:   0 - Connection started or active
 *          -1 - failed
 *****************************************************************************/
int SimModem::mqttConnect(){
  String response = readResponse(AT_MQT_CON,1000);
  if (response.indexOf("OK") >= 0){
    statusMQTT = MODEM_STATUS_MQTT_CON;
    return 0;
  }
  else if (response.indexOf("ERROR") >= 0){
    return 0;
  }
  
  return 0;
}

/******************************************************************************
 * MQTT Disconnect from Server
 * 
 * Attempts to disconnect without checking status
 * On error checks for reason (no connection is considered success)
 * Sets status variable for connection status
 *
 * Return:   0 - OK
 *           1 - Error
 *****************************************************************************/
int SimModem::mqttDisconnect(){
  String response = readWaitResponse(AT_MQT_DIS,1000,"OK");
  if (statusMQTT == MODEM_STATUS_MQTT_CON || statusMQTT == MODEM_STATUS_MQTT_CFG){
    statusMQTT = MODEM_STATUS_MQTT_CFG;
    return 0;
  }
  else{
    return 1;
  }
  return 0;
}

/******************************************************************************
 * Read MQTT settings
 * 
 * Print the MQTT configuration and MQTT state.
 *****************************************************************************/
int SimModem::mqttStatus(){
  Serial.println(readResponse(AT_MQT_CFG,1000));
  Serial.println(readResponse(AT_MQT_STA,1000));
  return 0;
}

/*
 * Read GPS string
 * No formatting
 * Return NEMA string
 */
String SimModem::readGPS(){
  String gpsResponse = readResponse(AT_GPS_RD,0);
  return gpsResponse.substring(gpsResponse.indexOf(':',1)+1,gpsResponse.indexOf("\n"));
}
/********************************************************************************************
* Read RF Signal Quality
* Reply is of form CSQ: XX,YY where XX is 99 without a signal
*/
String SimModem::readSignal(){
   String signalResponse = readWaitResponse(AT_CSQ,100,"CSQ: ");
   return signalResponse.substring(0,signalResponse.indexOf(","));
}
String SimModem::readVerify(){
  return readResponse(ATI,0);
}
String SimModem::readIMEI(){
  return imei;
}

/*
 * Time string is in quotes if available
 * Date occurs before comma ","
 * Date format is YY/MM/DD
 */
int SimModem::readClock(int format, String &clockString){
  if (status == MODEM_STATUS_ON){
    String dateTimeResponse = readWaitResponse(AT_TIME,100,"CCLK:");
  
    int startIndex = dateTimeResponse.indexOf("\"");
    int endIndex = dateTimeResponse.indexOf("\"",startIndex+1);
    if (format == 0){
      dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf("-",startIndex));
    }
    else if (format == 1){
      dateTimeResponse = dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf(",",startIndex));
      dateTimeResponse.replace("/","");
    }
    else {
      dateTimeResponse = dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf("-",startIndex));
      dateTimeResponse.replace("/","");
      dateTimeResponse.replace(",","_");
      dateTimeResponse.replace(":","");
    }
    clockString = dateTimeResponse;
    return 0;
  }
  else{
    return -1;
  }
}

/*
 * Time string is in quotes if available
 * Date occurs before comma ","
 * Date format is YY/MM/DD
 */
String SimModem::readClock(int format){
  if (status == MODEM_STATUS_ON){
    String dateTimeResponse = readWaitResponse(AT_TIME,100,"CCLK:");
  
    int startIndex = dateTimeResponse.indexOf("\"");
    int endIndex = dateTimeResponse.indexOf("\"",startIndex+1);
    if (format == 0){
      return dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf("-",startIndex));
    }
    else if (format == 1){
      dateTimeResponse = dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf(",",startIndex));
      dateTimeResponse.replace("/","");
      return dateTimeResponse;
    }
    else {
      dateTimeResponse = dateTimeResponse.substring(startIndex+1, dateTimeResponse.indexOf("-",startIndex));
      dateTimeResponse.replace("/","");
      dateTimeResponse.replace(",","_");
      dateTimeResponse.replace(":","");
      return dateTimeResponse;
    }
  }
  else{
    return "none";
  }
}

/******************************************************************************
 * Read Active IP address
 * 
 * Response has 4 configurations
 * Search for any active configuration independent of channel
 *
 * Return:   0 - OK
 *          -1 - ERROR
 *****************************************************************************/
int SimModem::readIP(String &ipAddress){
  ipAddress = readResponse(AT_NET_ACT,0);
  int ipAddressIndex = ipAddress.indexOf("1,\"");
  if (ipAddressIndex >= 0){
    ipAddress.substring(ipAddressIndex+3,ipAddressIndex+15);
    return 0;
  }
  else{
    return -1;
  }
}

/*
 * Response is channel of NTP data channel
 */
String SimModem::readClockID(){
  //return readResponse(AT_NTP_ID,0);
  return readResponse(AT_FTP_TYP,0);
}
/*
 * Response is channel of NTP data channel
 */
String SimModem::readRFCfg(){
  return readResponse(AT_RF_CFGQ,0);
}
/*
 * Response is channel of NTP data channel
 */
String SimModem::readIPPing(){
  checkStatus();
  if (status != MODEM_STATUS_ON){
    Serial.println(status);
    return ("No internet connection");
  }
  String pingResponse = readResponse(AT_IP_PING,3000);
  int ipAddressIndex = pingResponse.indexOf(",");  
  ipAddressIndex = pingResponse.indexOf(",", ipAddressIndex+1);
  int pingTimeIndex = pingResponse.indexOf(",",ipAddressIndex);
  return ("IP Address: " + pingResponse.substring(ipAddressIndex, pingTimeIndex)
          + " Response Time = " + pingResponse.substring(pingTimeIndex, pingResponse.length()));
  
}

/*
 * Turn off the command echo function
 */
 
int SimModem::enableIP(){
  for (int i = 0; i<5; i++){
    String string = readWaitResponse(AT_NET_0ON,2000,"PDP:");
    
    if (string.indexOf("Active") >= 0){
      return 0;
    }
    else if (string.indexOf("ERROR") >= 0){
      return 1;
    }
    delay(5000);
  }
  return -1;
}

/*
 * Turn off the command echo function
 */
String SimModem::echoOff(){
  return readResponse(ATE_OFF,0);
}

/*
 * FTP directory read operation
 */
void SimModem::ftpFileName(int type){
  switch(type){
    case 0:
      readResponse(AT_FTP_GET_NAM,0);
    case 1:
      readResponse(AT_FTP_GET_NAM,0);
  }
}

/*
 * FTP directory read operation
 */
String SimModem::ftpList(){

  String responseString;
  // Check if the modem has internet connection
  if (true){
    Serial.println("Requesting FTP list");
    responseString = readWaitResponse(AT_FTP_LST,10000,"FTPLIST:");

    // Check if response was data ready
    if (responseString.indexOf("1,1") >= 0){
      Serial.println("Reading FTP list");
      // Response will indicate number of bytes ready to read
      // Value may be 0, requires command to be repeated
      responseString = readWaitResponse(AT_FTP_LRD,10000,"FTPLIST: 2,");
      while(responseString.indexOf("0") == 0){
        responseString = readWaitResponse(AT_FTP_LRD,10000,"FTPLIST: 2,");
      }
      Serial.print(readBurst(AT_FTP_LRD,30000,"FTPLIST:"));
    }
    Serial.println("Waiting for disconnect, FTP");
    readWaitResponse(AT_FTP_EXT,3000,"FTPLIST: 1,80");
    return "Done";
  }
   
  return "No Network";
}

/*
 * File read operation for small files
 */
int SimModem::ftpGet(String &fileString){
  
  String responseString = readWaitResponse(AT_FTP_GET,5000,"FTPGET:");

  if (responseString.indexOf("1,1") >= 0){
    fileString.concat(readBurst(AT_FTP_GRD,5000,"FTPGET:"));
    readWaitResponse(AT_FTP_EXT,1000,"FTPGET: 1,80");
    return 0;
  }
  else return -1;
}
/*********************************************************************
 *  PUT operation for small files
 *  
 *  Input: dataFile (position may not be at start of file)
 *  Input: option (selects upload file name)
 *  
 *  Determines size of data to upload (from current file postion to end)
 *  Determines number of transfers at largest transfer size of 1360
 *  Determines size of last transfer less than maximum
 *  
 *  Output: 0 = successful transfer, received OK
 */
int SimModem::ftpPut(File dataFile, int option){
  
  // Check that modem is present, powered on, connected to network
  int modem = startSession();
  if(modem < 0){
    return -62;
  }
  
  String responseString;
  char temp;
  int dataSize = dataFile.size() - dataFile.position();
  int numPuts = dataSize / MODEM_BUFFER;
  int lastPutSize = dataSize % MODEM_BUFFER;
  String lastPutString = AT_FTP_PWR + String(lastPutSize);

  String timeString;
  readClock(2, timeString);
  String putFileName = AT_FTP_PUT_NM1 + String("\"") + imei + timeString + "_" + dataFile.name() + String("\"");
  readWaitResponse(putFileName,3000,"OK");
  readResponse(AT_FTP_PUT_QNM,100);                                     // ERROR: Delay needed? or verifies set name

  responseString = readWaitResponse(AT_FTP_PUT,3000,"FTPPUT:");         // Start FTP session

  while (responseString.indexOf("1,0") >= 0){                           // Wait for +FTPPUT: 1,1,xxxx (default xxxx = 1360)
    responseString = readWaitResponse(AT_FTP_PUT,3000,"FTPPUT:");
  }

  if (responseString.indexOf("1,1") >= 0){                              // Device ready for data
    for (int i = 0; i < numPuts; i++){                                  // Iterate over 1360 byte chunks
      
      responseString = readWaitResponse(AT_FTP_PWX,3000,"FTPPUT:");     // Wait for data ready acknowledgement
      if (responseString.indexOf("2,") < 0){                            // If transmission failed break loop
        Serial.println(responseString);
        Serial.println("Error");
        return -1;
      }
      for (int j = 0; j < MODEM_BUFFER; j++){
        temp = dataFile.read();
        SimModemSerial.print(temp);
      }
      delay(10);
      SimModemSerial.readBytesUntil(':',buffer, MODEM_BUFFER);
      SimModemSerial.flush();
    }
    if (lastPutSize){                                                    // Last cycle to write data less than maximum number of bytes
      responseString =  readWaitResponse(lastPutString,3000,"FTPPUT:");  // 
      if (responseString.indexOf("2,") < 0){                             // If transmission failed break loop
        Serial.println(responseString);
        Serial.println("Error");
        return -1;
      }
      for (int j = 0; j < lastPutSize; j++){                            // Iterate over the remaining bytes
          temp = dataFile.read();
          SimModemSerial.print(temp);
        }
        SimModemSerial.readBytesUntil(':',buffer, MODEM_BUFFER);
        SimModemSerial.flush();
    }
    responseString = readWaitResponse(AT_FTP_PDN,1000,"OK");
    //readResponse(AT_FTP_EXT,0);                                   // Disabled manual disconnect (MIGHT USE AGAIN)
    if (responseString){
      return 0;
    }   
  }  
  return -1;
}

int SimModem::ftpPut(String virtualFile, int option){
  
  // Check that modem is present, powered on, connected to network
  int modem = startSession();
  if(modem < 0){
    return -62;
  }
  
  String responseString;
  char temp;
  int dataSize = virtualFile.length();
  int numPuts = dataSize / MODEM_BUFFER;
  int lastPutSize = dataSize % MODEM_BUFFER;
  String lastPutString = AT_FTP_PWR + String(lastPutSize);

  String timeString;
  readClock(2, timeString);
  String putFileName;
  if (option == 2){
    putFileName = AT_FTP_PUT_NM1 + String("\"") + imei + timeString + ".txt" + String("\"");
  }
  else{
    putFileName = AT_FTP_PUT_NM1 + String("\"") + imei + timeString + ".csv" + String("\"");
  }
  readWaitResponse(putFileName,3000,"OK");
  readResponse(AT_FTP_PUT_QNM,100);                                     // ERROR: Delay needed? or verifies set name

  responseString = readWaitResponse(AT_FTP_PUT,3000,"FTPPUT:");         // Start FTP session

  while (responseString.indexOf("1,0") >= 0){                           // Wait for +FTPPUT: 1,1,xxxx (default xxxx = 1360)
    responseString = readWaitResponse(AT_FTP_PUT,3000,"FTPPUT:");
  }

  if (responseString.indexOf("1,1") >= 0){                              // Device ready for data
    for (int i = 0; i < numPuts; i++){                                  // Iterate over 1360 byte chunks
      responseString = readWaitResponse(AT_FTP_PWX,3000,"FTPPUT:");     // Wait for data ready acknowledgement
      if (responseString.indexOf("2,") < 0){                            // If transmission failed break loop
        Serial.println("Error");
        return -1;
      }
      for (int j = 0; j < MODEM_BUFFER; j++){
        temp = virtualFile[j+i*MODEM_BUFFER];
        SimModemSerial.print(temp);
      }
      delay(10);
      SimModemSerial.readBytesUntil(':',buffer, MODEM_BUFFER);
      SimModemSerial.flush();
    }
    if (lastPutSize){                                                   // Last cycle to write data less than maximum number of bytes
      responseString = readWaitResponse(lastPutString,3000,"FTPPUT:");  //
      if (responseString.indexOf("2,") < 0){                            // If transmission failed break loop
        Serial.println("Error");
        return -1;
      }
      for (int j = 0; j < lastPutSize; j++){
          temp = virtualFile[j+numPuts*MODEM_BUFFER];
          SimModemSerial.print(temp);
        }
        SimModemSerial.readBytesUntil(':',buffer, MODEM_BUFFER);
        SimModemSerial.flush();
    }
    responseString = readWaitResponse(AT_FTP_PDN,1000,"OK");
    // Do not attempt manual disconnect (CAUSES ERRORS)
    if (responseString){
      return 0;
    }   
  }  
  return -1;
}
/*
 *  Attach a string to an existing file
 */

int SimModem::ftpPut(String dataString){
  char temp;
  int dataSize = dataString.length()+1;
  String putString = AT_FTP_PWR + String(dataSize);
  String responseString;

  // Check that modem is present, powered on, connected to network
  int modem = startSession();
  if(modem < 0){
    return -62;
  }

  Serial.print("FTP Put Request Transmission :");
  Serial.println(readWaitResponse(AT_FTP_PUT,3000,"FTPPUT:"));
  responseString = readWaitResponse(putString,3000,"FTPPUT:");
  while (responseString.indexOf("1,0") >= 0){
    Serial.print("FTP server delay");
    responseString = readWaitResponse(putString,3000,"FTPPUT:");
  }
    
  for (int i = 0; i < dataSize; i++){
      temp = dataString[i];
      SimModemSerial.print(temp);
  }
  SimModemSerial.print('\n');
  SimModemSerial.readBytesUntil(':',buffer, MODEM_BUFFER);
  SimModemSerial.flush();
  
  readWaitResponse(AT_FTP_PDN,10000,"FTPPUT: 1,0");
  // Do not attempt manual disconnect (CAUSES ERRORS)
  return 0;
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::GPSOn(){
  return readResponse(AT_GPS_PWR,0);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::GPSOff(){
  return readResponse(AT_GPS_OFF,0);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::RFOn(){
  return readResponse(AT_RF_ON,0);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::RFOff(){
  return readResponse(AT_RF_OFF,0);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::disableIP(){
  readResponse(AT_NET_0OF,0);
  return readResponse(AT_NET_1OF,0);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::ftpCID(){
  return readResponse(AT_FTP_CID,100);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::ftpUsername(){
  Serial.println(readResponse(AT_FTP_UN_QRY,100));
  return readResponse(AT_FTP_UN,100);
}

/*
 * GPS Enabled
 * Simcom 7070G cannot operate GPS and modem at same time
 */
String SimModem::ftpPwd(){
  return readResponse(AT_FTP_PWD,100);
}

/*
 * FTP Server Set
 * Must append filename in quotes to end of AT command
 */
String SimModem::ftpServer(){
  Serial.println(readResponse(AT_FTP_SRV_QRY,100));
  return readResponse(AT_FTP_SRV,100);
}

int SimModem::ftpStatus(){
  Serial.println(readResponse(AT_FTP_SRV_QRY,100));
  Serial.println(readResponse(AT_FTP_UN_QRY,100));
  Serial.println(readResponse(AT_FTP_PW_QRY,100));
  Serial.println(readResponse(AT_FTP_GET_QNM,100));
  Serial.println(readResponse(AT_FTP_GET_QPT,100));
  Serial.println(readResponse(AT_FTP_PUT_QNM,100));
  Serial.println(readResponse(AT_FTP_PUT_QPT,100));
  Serial.println(readResponse(AT_FTP_PUT_QOP,100));  
  Serial.println(readResponse(AT_FTP_PUT_QOP,100));
  return 0;
}

/*
 * FTP File Name Set
 * Must append filename in quotes to end of AT command
 */
String SimModem::ftpFile(){
  Serial.println(readResponse(AT_FTP_GET_QNM,1000));
  return readResponse(AT_FTP_PUT_NM1 + String("\"") + "test.txt" + String("\""),0);
}

/******************************************************************************
 * MQTT Server Subscribe
 * 
 *  Send the subscribe command (requires initialization)
 *  Check response for error
 *  Append response to input pointer
 *
 *  ERROR:  0 - ok
 *          1 - error, connection exists
 *          2 - error, message empty
 *****************************************************************************/
int SimModem::mqttSub(String &message){
  message = readWaitResponse(AT_MQT_SUB, 10000,"SMSUB:");
  if (message.indexOf("ERROR") < 0){
    statusMQTT = MODEM_STATUS_MQTT_SUB;
    return 0;
  }
  return 1;
}

/*
 * MQTT Server Publish
 * 
 */
int SimModem::mqttPub(String message, int option){
  // Test variable for index of string
  char temp = 'b';
  if (option == 0){
    readResponse(AT_MQT_PUB1,0);
    SimModemSerial.print(temp);
  }
  else if (option == 1){
    String ATpubMessage = AT_MQT_PUBX + String(message.length()) + AT_MQT_PUBY;
    readResponse(ATpubMessage,0);
    for (int i = 0; i < message.length(); i++){
      temp = message[i];
      SimModemSerial.print(temp);
    }
  }
  return 0;
}

/*
 * MQTT File Name SetServer Unsubscribe
 * 
 */
int SimModem::mqttUnsub(){
  String response = readResponse(AT_MQT_UNS,0);
  if (response.indexOf("OK")){
    return 0;
  }
  return -1;
}

/******************************************************************************
 * MQTT Server Read
 * 
 *  Check status, configure is necessary
 *  Check response for error
 *  Append response to input pointer
 *
 *  ERROR:  0 - ok
 *          1 - error, connection exists
 *          2 - error, message empty
 *****************************************************************************/
int SimModem::mqttRead(String &message, int option){
  String response;
  if (statusMQTT == MODEM_STATUS_MQTT_UNKNOWN){
    Serial.println("unknown");
    response = readWaitResponse(AT_MQT_STA,1000,"SMSTATE:");  // Check the connection
    if (response.indexOf("1") >= 0){                          // 1 = connected
      statusMQTT = MODEM_STATUS_MQTT_CON;                     // Set status
    }
    // Device is not connected MQTT server
    else{
      Serial.println("not connect");
      // Attempt to write configuration
      if (startMQTT(option)){
        Serial.println("fail start");
        // Configuration failed, error returned
        statusMQTT = MODEM_STATUS_MQTT_UNKNOWN;
        return -1;
      }
    }
  }
  // Device might have been unknown and configured by previous
  if (statusMQTT == MODEM_STATUS_MQTT_CFG){
    Serial.println("connect");
    mqttConnect();
  }
  // Device might already be connected
  if (statusMQTT == MODEM_STATUS_MQTT_SUB){
    Serial.println("unsubscribe");
    mqttUnsub();
  }
  // Need to change to configuration topic
  readResponse(AT_MQT_TOP,0);
  delay(100);
  // Final step is to subscribe and read latest message
  mqttSub(message);
  Serial.println(message);
  //mqttUnsub();
  return 0;
}

/******************************************************************************
 * MQTT Server Publish
 * 
 *  Check status, configure is necessary
 *  Check response for error
 *  Append response to input pointer
 *
 *  ERROR:  0 - ok
 *          1 - error, connection exists
 *          2 - error, message empty
 *****************************************************************************/
int SimModem::mqttWrite(String &message, int option){
  String response;
  if (statusMQTT == MODEM_STATUS_MQTT_UNKNOWN){
    Serial.println("unknown");
    response = readWaitResponse(AT_MQT_STA,1000,"SMSTATE:");  // Check the connection
    if (response.indexOf("1") >= 0){                          // 1 = connected
      statusMQTT = MODEM_STATUS_MQTT_CON;                     // Set status
    }
    // Device is not connected MQTT server
    else{
      Serial.println("not connect");
      // Attempt to write configuration
      if (startMQTT(option)){
        Serial.println("fail start");
        // Configuration failed, error returned
        statusMQTT = MODEM_STATUS_MQTT_UNKNOWN;
        return -1;
      }
    }
  }
  // Device might have been unknown and configured by previous
  if (statusMQTT == MODEM_STATUS_MQTT_CFG){
    Serial.println("connect");
    mqttConnect();
  }
  // Device might already be connected
  if (statusMQTT == MODEM_STATUS_MQTT_SUB){
    Serial.println("unsubscribe");
    mqttUnsub();
  }
  // Change topic to device specific
  String topicID = AT_MQT_TOP_STAB;
  //topicID.concat("_" + imei.substring(12,15));
  Serial.println(topicID);
  Serial.println(readResponse(topicID,0));
  delay(100);
  // Final step is to subscribe and read latest message
  mqttPub(message,1);
  Serial.println(message);
  //mqttUnsub();
  return 0;
}

/*
 * Call for checking current software state, enables "threading" modem operations
 * 
 * Initial state: UNKNOWN
 *    Check if device is on (any query)
 * Device On:
 * Device Off:
 * 
 */

int SimModem::checkStatus(){
  String responseString = "";

  // Determine if the device is ON or OFF
  if (status == MODEM_STATUS_UNKNOWN || status == MODEM_STATUS_OFF){
    Serial.println("Checking status");
    responseString = readResponse(ATE_OFF,0);   // Send basic query, wait for response
    if (responseString == ""){
      status = MODEM_STATUS_OFF;
      return status;
    }
    status = MODEM_STATUS_ON;
  }

  // If the device is ON then check status of different modules
  if (status == MODEM_STATUS_ON){
    return status;
  }
  else {
    return (int)status;
  }
}

/*
 *  checkFTP
 */
int SimModem::checkFTP(){
  if (status <= -3){
    if ((millis() - timer) > 10000){
      status = -1;
      return -2;
    }
    else{
      return -3;
    }
  }
  else if (status == -2){
    if ((millis() - timer) > 35000){
      String responseString = readResponse(ATE_OFF,0);
      if (responseString != "") {
        status = 0;
        return 0;
      }
      return -1;
    }
    return -2;
  }
  else {
    return (int)status;
  }
}

/******************************************************************************
 * SSL Functions
 *****************************************************************************/
int SimModem::sslFileDownload(File dataFile, int option){
  if (option == 0){
    Serial.println(readResponse(AT_SSL_FL1,0));
  }
  else if (option == 1){
    Serial.println(readResponse(AT_SSL_FL2,0));
  }
  else if (option == 2){
    Serial.println(readResponse(AT_SSL_FL3,0));
  }
  for (int j = 0; j < dataFile.size(); j++){
    char temp = dataFile.read();
    SimModemSerial.print(temp);
  }
  return 0;
}

/******************************************************************************
 * SSL File Download from code
 * Download the selected file from "secrets.h"
 *****************************************************************************/

int SimModem::sslFileDownload(int option){
  int fileSize = 0;
  String rootFile = SSL_ROOT;
  if (option == 0){
    Serial.println(readResponse(AT_SSL_CA_MOSQ,0));
    fileSize = SSL_ROOT_SIZE;
  }
  else if (option == 1){
    Serial.println(readResponse(AT_SSL_FL2,0));
    fileSize = SSL_CERT_SIZE;
  }
  else if (option == 2){
    Serial.println(readResponse(AT_SSL_FL3,0));
    fileSize = SSL_KEY_SIZE;
  }
  for (int j = 0; j < fileSize; j++){
    char temp = rootFile[j];
    SimModemSerial.print(temp);
  }
  return 0;
}

/******************************************************************************
 * SSL File Convert
 * Use the current SSL configuration to convert a certificate file
 *
 * Return: not implemented
 *****************************************************************************/
int SimModem::sslConvert(int option){
  if (option == 0){
    Serial.println(readResponse(AT_SSL_CV1,0));
  }
  else if (option == 1){
    Serial.println(readResponse(AT_SSL_CV2,0));
  }
  else if (option == 2){
    Serial.println(readResponse(AT_SSL_CV3,0));
  }
  return 0;
}

int SimModem::sslCipher(){
  Serial.println(readResponse(AT_SSL_CIP,0));
  return 0;
}

/******************************************************************************
 * SSL Configuration
 * Set the current SSL configuration SNI value
 *
 * Return: not implemented
 *****************************************************************************/
int SimModem::sslSni(String &message, int option){
  message = readResponse(AT_SSL_SNI_HIVE,0);
  return 0;
}

/******************************************************************************
 * SSL Configuration
 * Set the current SSL configuration index value
 *
 * Return: not implemented
 *****************************************************************************/
int SimModem::sslCtindex(){
  Serial.println(readResponse(AT_SSL_CTX,0));
  return 0;
}

/******************************************************************************
 * SSL Configuration
 * Set the current SSL configuration TLS version value
 *
 * Return: not implemented
 *****************************************************************************/
int SimModem::sslVersion(){
  Serial.println(readResponse(AT_SSL_VER,0));
  return 0;
}

/******************************************************************************
 * SSL Configuration
 * Set the current SSL configuration TLS version value
 *
 * Return: not implemented
 *****************************************************************************/
int SimModem::sslConfigure(int option){
  String responseString = "";
  if (option == 0){
    startCFS();                               // CFS init
    sslFileDownload(0);                       // CFS write file
    stopCFS();                                // CFS term
    startMQTT(2);                             // MQTT Configuration includes ?sni?
    readResponse(AT_SSL_VER,0);               // SSL version
    readResponse(AT_SSL_CIP,0);               // SSL ciphersuite
    
    sslConvert(0);                            // SSL convert
    readResponse(AT_SSL_CTX,0);               // SSL Configutation index (default 1)
    
  }
  else if (option == 1){
    startCFS();                               // CFS init
    readResponse(AT_SSL_VER,0);               // CFS write file
    readResponse(AT_SSL_VER,0);               // CFS write file
    readResponse(AT_SSL_VER,0);               // CFS write file
    stopCFS();                                // CFS term
    readResponse(AT_SSL_VER,0);               // SSL version
    readResponse(AT_SSL_VER,0);               // SSL ciphersuite
    readResponse(AT_SSL_VER,0);               // SSL sni
    readResponse(AT_SSL_VER,0);               // SSL Convert 
    readResponse(AT_SSL_VER,0);               // SSL Convert key file
  }
  return 0;
}

/*
 * HTTP Functions
 */
int SimModem::httpRead(){
  Serial.println(readResponse(AT_WWW_URL,0));
  Serial.println(readResponse(AT_WWW_BDY,0));
  Serial.println(readResponse(AT_WWW_HDR,0));
  Serial.println(readResponse(AT_WWW_CON,0));
  Serial.println(readResponse(AT_WWW_STA,0));
  Serial.println(readResponse(AT_WWW_HED_CLR,0));
  Serial.println(readResponse(AT_WWW_HED_AGT,0));
  Serial.println(readResponse(AT_WWW_HED_CAC,0));
  Serial.println(readResponse(AT_WWW_HED_CON,0));
  Serial.println(readResponse(AT_WWW_HED_ACC,0));
  Serial.println(readWaitResponse(AT_WWW_GET,3000,"+SHREQ:"));
  Serial.println(readBurst(AT_WWW_RD,1000,"url"));
  return 0;
}

int SimModem::httpSSL(){
  Serial.println(readResponse(AT_WWW_SSL,0));
  return 0;
}
/*
 * CFS Functions
 */
int SimModem::startCFS(){
  Serial.println(readResponse(AT_CFS_INI,0));
  return 0;
}

int SimModem::stopCFS(){
  Serial.println(readResponse(AT_CFS_TRM,0));
  return 0;
}
