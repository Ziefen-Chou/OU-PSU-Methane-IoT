/*  Adafruit Feather Senseair K96 with Modbus Interface
 *  
 *  Creates an external interface and stores data from serial sensor devices
 *  Currently works with Adalogger SD card, FeatherWing WiFi, and Simcom 7080G modem
 *  System has optional data acquisition from a serial GPS device
 *  1. 
 *  2. Check for serial port input
 *  3. Data acquisition check
 *  3a. Check timer
 *  3b. Sensor data
 *  3c. GPS data
 *  3d. Clock data
 *  4. ONLY IF data acquired FTP check
 *  4a. Upload backup data file if timer check OR getting too large for transfer
 *  4b. Upload recent data file if timer check
 *  4c. Download settings file if timer check
 *  
 * Written by James D. Jeffers 2022/06/30
 * Updated 2023/10/23
 * Copyright (c) 2023 University of Oklahoma.  All right reserved.

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

#define SW_VER_NUM "Firmware v0.10.3"
#include "secrets.h"

#include "SystemControl.h"
#include "k96Modbus.h"
#include "SimModem.h"
#include "DataLogger.h"
#include "GPSSerial.h"
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>

#define INTERVAL_DATA_MAX 900000
#define INTERVAL_DATA_MIN 20000
#define INTERVAL_FTP_MAX 900000
#define INTERVAL_FTP_MIN 60000
#define INTERVAL_CFG_MAX 900000
#define INTERVAL_CFG_MIN 60000
#define INTERVAL_BACKUP_MAX 7200000
#define INTERVAL_BACKUP_MIN 600000
#define INTERVAL_LOG_MAX 3600000
#define INTERVAL_LOG_MIN 60000

#define SERIAL_BUFFER 1024

#define DEVICE_ENABLED 0
#define DEVICE_DISABLED 1

#define MODE_CFG_FTP 0
#define MODE_CFG_MQTT 1

#define MODE_DATA MODE_CFG_FTP
#define MODE_CTRL MODE_CFG_MQTT


String dataString = "test_April-2024_#";
int returnValue = 0;  // Generic integer for catching return values
String returnString = "";

int intervalData = 1000;  // Read data from sensor and GPS every 30 seconds
int timerLastRead = 0;    // Millisecond timer value for last data read

int intervalFile = DEVICE_ENABLED;  // Status variable to enable/disable code options

int intervalUpdate = 0;     // Write most recent data to FTP server every 3 minutes
int timerLastDataFile = 0;  // Millisecond timer value for last data upload
int countLastDataFile = 0;

int intervalCfg = 0;  // Read timing configuration by FTP every 3 minutes
int timerLastCfg = 0;

int intervalBackup = 0;  // Backup the data file every 15 minutes
int LastFileUpload = 0;
int lastFTPByteBackup = 0;

int firstLog = DEVICE_DISABLED;
int intervalLog = 0;     // Update the system event log every 60 minutes
int timerLastLog = 0;    // Timer value in milliseconds for last log FTP
int lastFTPByteLog = 0;  // Number of bytes current log succussful FTP

SystemControl control;            // Enables external DC power supplies for fan, pump, etc.
int powerSave = DEVICE_DISABLED;  // Track the ability to turn off the modem bewteen uploads
bool powerFan = true;             // Enable signal for hardware buck converter
bool powerPump = true;            // Enable signal for hardware buck converter

SimModem modem;
int statusModem = DEVICE_ENABLED;  // Tracks current state of modem (0 = OK)
int statusFTP = -1;                // Tracks the number of consectutive FTP errors (0 = OK)
bool logModemTime = true;
bool logModemPower = true;

k96Modbus k96;                      // Serial port device using Modbus protocol
int statusSensor = DEVICE_ENABLED;  // System will always try to communicate with a sensor

DataLogger logger;              // Physical or virtual (saves data in RAM without card)
int statusSD = DEVICE_ENABLED;  // Maximum 1024 files, filename length =< 8
bool fileSizeLimit = true;      // Number of bytes before a new file is created
bool dataSizeLimit = false;     // Number of bytes stored locally before modem is activated

GPSSerial gpsSerial;
int statusGPS = DEVICE_DISABLED;
int statusClock = 0;

// Serial port parser
char serialBuffer[SERIAL_BUFFER] = "";

// PSU_add
unsigned long previousMillis = 0;    // Stores the last time the calculation was made
const long interval_sleep = 300000;  // Interval at which to perform the calculation (5 minutes = 300000 milliseconds)

long count = 0;

/*********************************************************************************************
 *  Setup and Initializtion
 *  
 *  a. Serial port for debug
 *  1. SD card (can store text before/after other devices) 
 *  2. Modem for real-time data transfer
 *  3. SenseAir K96 Sensor (serial port with Modbus)
 *  
 ********************************************************************************************/

void setup() {

  Serial.begin(115200);  // Initialize serial port for USB connections
  delay(3000);
  Serial.println("OU Nananophotonics Lab: Project AIMNet");

  /* Device 1: SD card 1111111111111111111111111111111111111111111111111111111111*/
  if (statusSD == DEVICE_ENABLED) {
    Serial.print("SD Card ");
    statusSD = logger.init();
    if (statusSD == 0) {
      Serial.println("Initialized");
      logger.fileRemove(1);
      logger.logNewName();
      logger.fileAddCSV("**********************************************", 2);
      logger.fileAddCSV(SW_VER_NUM, 2);

      // Check for settings file on SD
      if (!readStatus()) {
        logger.fileAddCSV(("Default Settings: " + settingsString()), FILE_TYPE_LOG);
        writeStatus();
      } else {
        logger.fileAddCSV(("SD Card Settings: " + settingsString()), FILE_TYPE_LOG);
      }

    } else if (statusSD > 0) {
      Serial.println("Not Present, Virtual Device Enabled");
      logger.fileAddCSV("**********************************************", 2);
      logger.fileAddCSV(SW_VER_NUM, FILE_TYPE_LOG);
      logger.fileAddCSV(("Default Settings: " + settingsString()), FILE_TYPE_LOG);
      logger.fileAddCSV(("Memory: Virtual SD card"), FILE_TYPE_LOG);
    } else {
      Serial.println("Failed");
    }
  }

  /* Device 2: Modem 2222222222222222222222222222222222222222222222222222222222222*/
  if (statusModem == DEVICE_ENABLED) {
    Serial.print("Modem ");
    statusModem = modem.init();
    if (statusModem) {
      logger.fileAddCSV("Modem not present", FILE_TYPE_LOG);
      Serial.println(" not present");
    } else {
      modem.readClock(0, returnString);
      logger.fileAddCSV((returnString + ": Modem IMEI " + modem.readIMEI()), FILE_TYPE_LOG);
      Serial.println(" initialized");
    }
  }

  /* Device 3: Chemical sensor 333333333333333333333333333333333333333333333333333*/
  statusSensor = k96.init();
  if (!statusSensor) {
    Serial.println("Sensor initialized");
    modem.readClock(0, returnString);
    logger.fileAddCSV((returnString + ": K96 Sensor Initialized"), FILE_TYPE_LOG);
  } else {
    logger.fileAddCSV((returnString + ": K96 Sensor Failed"), FILE_TYPE_LOG);
    Serial.println("Sensor failed");
  }

  /* Device 4: GPS sensor   */
  if (statusGPS == DEVICE_ENABLED) {
    control.enablePower(POWER_GPS);  // Added for board 3 support
    statusGPS = gpsSerial.init();
    if (!statusGPS) {
      Serial.println("GPS Module initialized");
    } else {
      Serial.println("GPS Module failed");
    }
  }

  // Create a data acquisition file based on the date
  if (statusSD >= 0) {
    logger.fileNewName();
    modem.readClock(0, returnString);
    logger.fileAddCSV((returnString + ": Data acquisition file = " + logger.fileNameString()), FILE_TYPE_LOG);
  }

  // Completed startup, if internet connection available store system log
  if (firstLog && !statusModem) {
    Serial.println("Log File Upload Started");
    if (statusSD == FILE_REAL) {
      File tempFile = logger.fileOpen(FILE_TYPE_LOG);
      int currentFTP = modem.ftpPut(tempFile, FILE_TYPE_LOG);
      modem.readClock(0, returnString);
      logger.fileAddCSV((returnString + ": Log File Uploaded Bytes: " + String(tempFile.size())), FILE_TYPE_LOG);
      if (!currentFTP) {
        logger.fileAddCSV((returnString + ": Log File Uploaded Bytes: " + String(tempFile.size())), FILE_TYPE_LOG);
        lastFTPByteLog = tempFile.size();
      } else {
        logger.fileAddCSV((returnString + ": Log File Upload Failed Code: " + String(currentFTP)), FILE_TYPE_LOG);
      }
      tempFile.close();
    } else if (statusSD == FILE_VIRTUAL) {
      int currentFTP = modem.ftpPut(logger.fileRead(FILE_TYPE_LOG), FILE_TYPE_LOG);
      modem.readClock(0, returnString);
      if (!currentFTP) {
        logger.fileAddCSV((returnString + ": Log File Upload (Bytes) = " + String(logger.fileSize(FILE_TYPE_LOG))), FILE_TYPE_LOG);
        lastFTPByteLog = logger.fileSize(FILE_TYPE_LOG);
      } else {
        logger.fileAddCSV((returnString + ": Log File Upload Failed (Code) = " + String(currentFTP)), FILE_TYPE_LOG);
      }
    }
  }

  /* Device 5 and 6: System pump and system fan */
  if (powerFan) {
    control.enablePower(POWER_FAN);
  }
  if (powerPump) {
    control.enablePower(POWER_PUMP);
  }
}

/******************************************************************************************************
 *  Main program loop
 *  1. Check serial port and handle I/O
 *  2. Loop timing, read sensor data
 *  3. Loop timing, Update timing, upload data
 *  4. Loop timing, file timing, upload data
 ******************************************************************************************************/

void loop() {

  delay(300);
  // Check the serial port for user input, single letter codes

  if (Serial.available()) {

    String serialCommand = "";
    int serialData = 0;
    int serialDataSize = Serial.readBytesUntil('\n', serialBuffer, SERIAL_BUFFER);
    serialCommand = String(serialBuffer);
    serialCommand = serialCommand.substring(0, serialDataSize);

    if (serialCommand == "status") {
      Serial.println("System Status");
      Serial.print("Modem = ");
      Serial.println(statusModem);
      Serial.print("SD Card = ");
      Serial.println(statusSD);
      Serial.print("GPS Sensor = ");
      Serial.println(statusGPS);
      Serial.print("Sensor = ");
      Serial.println(statusSensor);
    } else if (serialCommand == "d") {
      Serial.println(dataString);
    } else if (serialCommand.startsWith("d=")) {
      intervalData = serialCommand.substring(2, serialDataSize).toInt();
      Serial.print("New data rate:");
      Serial.println(intervalData);
    } else if (serialCommand.startsWith("power save")) {
      powerSave = 1;
      Serial.print("Power Save Mode enabled");
    } else if (serialCommand.startsWith("power use")) {
      powerSave = 0;
      Serial.print("Power Save Mode disabled");
    }

    //********************************************************************************
    // Power System commands
    else if (serialCommand == "fan on") {
      control.enablePower(0);
      Serial.println("Fan enabled");
    } else if (serialCommand == "pump on") {
      control.enablePower(1);
      Serial.println("Pump enabled");
    } else if (serialCommand == "gps on") {
      control.enablePower(2);
      Serial.println("GPS enabled");
    } else if (serialCommand == "fan off") {
      control.disablePower(0);
      Serial.println("Fan disabled");
    } else if (serialCommand == "pump off") {
      control.disablePower(1);
      Serial.println("Pump disabled");
    } else if (serialCommand == "gps off") {
      control.disablePower(2);
      Serial.println("GPS disabled");
    } else if (serialCommand == "read aux") {
      Serial.print("Read auxilliary input (max 1024): ");
      Serial.println(control.readAux());
    }

    //********************************************************************************
    // SD Card commands
    else if (serialCommand == "filenew") {
      logger.fileNewName();
    } else if (serialCommand == "filesize") {
      Serial.println(logger.fileCheckSize());
    } else if (serialCommand == "fileadd") {
      Serial.println(logger.fileAddCSV("abc", 0));
    } else if (serialCommand == "filename") {
      Serial.println(logger.fileNameString());  // Data file
    } else if (serialCommand == "file data") {
      Serial.println("Data File: ");
      // logger.fileDump(FILE_TYPE_DATA);                 // Data file
      Serial.println(logger.fileReadByLine(FILE_TYPE_DATA, 1, 10));
    } else if (serialCommand == "file log") {
      Serial.println("Log File: ");
      logger.fileDump(FILE_TYPE_LOG);  // Log file
    } else if (serialCommand == "file status") {
      Serial.println("Status File: ");
      logger.fileDump(FILE_TYPE_STATUS);  // Log file
    } else if (serialCommand == "file status load") {
      Serial.println("Status File: ");
      readStatus();  // Log file
    } else if (serialCommand == "file cert") {
      Serial.println("Certificate File: ");
      logger.fileDump(FILE_TYPE_CRT);  // Log file
    } else if (serialCommand == "file pem") {
      Serial.println("PEM File: ");
      logger.fileDump(FILE_TYPE_PEM);  // Log file
    } else if (serialCommand == "file key") {
      Serial.println("PEM File: ");
      logger.fileDump(FILE_TYPE_KEY);  // Log file
    } else if (serialCommand == "r") {
      logger.fileRemove(1);
    } else if (serialCommand == "r_spe") {  //delete specified file
      logger.fileRemove_Specified(1, "D1.CSV");
    } else if (serialCommand == "ls") {
      Serial.println("SD Card Directory ");
      File root;
      root = SD.open("/");
      logger.fileDir(root, 4);
      root.close();
    } else if (serialCommand.indexOf("!") >= 0) {
      Serial.println("SD Card Delete All ");
      logger.fileRemoveAll();
    }

    //*********************************************************************************
    // Modem Commands
    else if (serialCommand.startsWith("upload=")) {
      intervalUpdate = serialCommand.substring(7, serialDataSize).toInt();
      Serial.print("New FTP upload rate: ");
      Serial.println(intervalUpdate);
      logger.fileAddCSV((returnString + ": FTP Upload Delay = " + String(intervalUpdate)), 2);
    }

    else if (serialCommand == "e") {
      Serial.print("Simcom 7070G Device Echo Off = ");
      Serial.println(modem.echoOff());
    } else if (serialCommand == "imei") {
      Serial.print("Simcom 7070G Device IMEI = ");
      Serial.println(modem.readIMEI());
    }

    else if (serialCommand == "q") {
      Serial.print("Simcom 7070G 4G Signal = ");
      Serial.println(modem.readSignal());
    } else if (serialCommand == "g") {
      Serial.print("Simcom 7070G GPS Signal = ");
      Serial.println(modem.readGPS());
    } else if (serialCommand == "h") {
      Serial.print("Simcom 7070G RF Config = ");
      Serial.println(modem.readRFCfg());
    } else if (serialCommand == "clock") {
      Serial.print("Simcom 7070G Date and Time = ");
      modem.readClock(0, returnString);
      Serial.println(returnString);
    } else if (serialCommand == "clock reset") {
      Serial.print("Simcom 7070G Date and Time = ");
      Serial.println(modem.startNTP());
    }

    else if (serialCommand == "init") {
      Serial.print("Simcom 7070G Reset ");  //here
      statusModem = modem.init();
    }

    else if (serialCommand == "power") {
      Serial.println("Simcom 7070G Power Toggle");
      modem.powerToggle();
    }

    else if (serialCommand == "ip") {
      Serial.print("Simcom 7070G IP Address = ");
      returnValue = modem.readIP(returnString);
      if (!returnValue) {
        Serial.println(returnString);
      } else {
        Serial.println("No Connection");
      }
    } else if (serialCommand == "ping") {
      Serial.print("Simcom 7070G IP Ping = ");
      Serial.println(modem.readIPPing());
    }

    else if (serialCommand == "modem gps off") {
      Serial.print("Simcom 7070G GPS Disabled = ");
      Serial.println(modem.GPSOff());
    } else if (serialCommand == "modem gps on") {
      Serial.print("Simcom 7070G GPS Enabled = ");
      Serial.println(modem.GPSOn());
    } else if (serialCommand == "modem RF off") {
      Serial.print("Simcom 7070G Disabled = ");
      Serial.println(modem.RFOff());
    } else if (serialCommand == "modem RF on") {
      Serial.print("Simcom 7070G Enabled = ");
      Serial.println(modem.RFOn());
    } else if (serialCommand == "cfs init") {
      Serial.print("Simcom 7070G File System = ");
      Serial.println(modem.startCFS());
    } else if (serialCommand == "cfs term") {
      Serial.print("Simcom 7070G File System Closed = ");
      Serial.println(modem.stopCFS());
    } else if (serialCommand == "exit") {
      Serial.print("Simcom 7070G Device Exit ");
      logger.fileRemoveAll();
      while (1)
        ;
    }
    //********************************************************************************
    // Network Commands
    else if (serialCommand == "net off") {
      Serial.println("Simcom 7070G Network Deactivated = ");
      Serial.println(modem.disableIP());
    } else if (serialCommand == "net on") {
      Serial.println("Simcom 7070G Network Activated = ");
      Serial.println(modem.enableIP());
    } else if (serialCommand == "ssl pem w") {
      Serial.println("Simcom 7070G Network SSL PEM = ");
      File tempFile = logger.fileOpen(FILE_TYPE_PEM);
      modem.sslFileDownload(tempFile, 0);
      tempFile.close();
    } else if (serialCommand == "ssl crt w") {
      Serial.println("Simcom 7070G Network SSL CRT = ");
      File tempFile = logger.fileOpen(FILE_TYPE_CRT);
      modem.sslFileDownload(tempFile, 0);
      tempFile.close();
    } else if (serialCommand == "ssl key w") {
      Serial.println("Simcom 7070G Network SSL key = ");
      File tempFile = logger.fileOpen(FILE_TYPE_KEY);
      modem.sslFileDownload(tempFile, 0);
      tempFile.close();
    } else if (serialCommand == "ssl pem test") {
      Serial.println("Simcom 7070G Network SSL Test file = ");
      Serial.println(modem.sslFileDownload(0));
    } else if (serialCommand == "ssl version") {
      Serial.println("Simcom 7070G Network SSL Type = ");
      modem.sslVersion();
    } else if (serialCommand == "ssl cipher") {
      Serial.println("Simcom 7070G Network SSL Cipher Suite = ");
      modem.sslCipher();
    } else if (serialCommand == "ssl ctindex") {
      Serial.println("Simcom 7070G Network SSL Cipher Suite = ");
      modem.sslCtindex();
    } else if (serialCommand == "ssl convert1") {
      Serial.println("Simcom 7070G Network SSL Key file = ");
      Serial.println(modem.sslConvert(0));
    } else if (serialCommand == "ssl convert2") {
      Serial.println("Simcom 7070G Network SSL PEM file = ");
      Serial.println(modem.sslConvert(1));
    } else if (serialCommand == "ssl convert3") {
      Serial.println("Simcom 7070G Network SSL CRT file = ");
      Serial.println(modem.sslConvert(2));
    } else if (serialCommand == "ssl configure") {
      Serial.println("Simcom 7070G Network SSL Configuration = ");
      Serial.println(modem.sslConfigure(0));
    }

    //********************************************************************************
    // // HTTP Commands
    // else if (serialCommand == "http read"){
    //   Serial.println("Simcom 7070G HTTP Read ");
    //   Serial.println(modem.httpRead());
    // }
    // else if (serialCommand == "http ssl"){
    //   Serial.println("Simcom 7070G HTTP Read ");
    //   Serial.println(modem.httpSSL());
    // }

    //********************************************************************************
    // FTP Commands
    else if (serialCommand == "ftp start") {
      Serial.println("Simcom 7070G FTP Reset ");
      Serial.println(modem.startFTP());
    } else if (serialCommand == "ftp un") {
      Serial.print("Simcom 7070G FTP Set Username = ");
      Serial.println(modem.ftpUsername());
    } else if (serialCommand == "ftp id") {
      Serial.print("Simcom 7070G FTP Set CID = ");
      Serial.println(modem.ftpCID());
    } else if (serialCommand == "ftp pwd") {
      Serial.print("Simcom 7070G FTP Set Password = ");
      Serial.println(modem.ftpPwd());
    } else if (serialCommand == "ftp server") {
      Serial.print("Simcom 7070G FTP Set Server = ");
      Serial.println(modem.ftpCID());
    } else if (serialCommand == "ftp status") {
      Serial.print("Simcom 7070G FTP Status:");
      Serial.println(modem.ftpStatus());
    }

    else if (serialCommand == "ftp ls") {
      Serial.print("Simcom 7070G FTP List = ");
      Serial.println(modem.ftpList());
    } else if (serialCommand == "ftp update") {
      Serial.print("Simcom 7070G FTP Get = ");
      String json;
      if (updateConfig(json) >= 0) {
        Serial.print("File downloaded");
        Serial.println(settingsString());
      } else {
        Serial.println("FTP Download Failed");
      }
    } else if (serialCommand == "ftp log") {
      Serial.print("Simcom 7070G FTP Put File = ");
      Serial.println(logger.fileRead(FILE_TYPE_LOG));
      File tempFile = logger.fileOpen(FILE_TYPE_LOG);
      Serial.println(modem.ftpPut(tempFile, FILE_TYPE_LOG));
      tempFile.close();
    } else if (serialCommand == "ftp log string") {
      Serial.print("Simcom 7070G FTP Put File = ");
      String temp = logger.fileRead(FILE_TYPE_LOG);
      Serial.println(modem.ftpPut(temp, FILE_TYPE_LOG));
    } else if (serialCommand == "s") {
      Serial.print("Simcom 7070G FTP Put File = ");
      File tempFile = logger.fileOpen(FILE_TYPE_DATA);
      Serial.println(modem.ftpPut(tempFile, FILE_TYPE_DATA));
      tempFile.close();
    } else if (serialCommand == "S") {
      Serial.print("Simcom 7070G FTP Put Append Data = ");
      Serial.println(modem.ftpPut(dataString));
    }

    //*********************************************************************************
    // MQTT Commands
    else if (serialCommand == "mqtt start 1") {
      Serial.print("Simcom 7070G MQTT Start = ");
      modem.startMQTT(1);  // 0 = HiveMQ server.  // here set the MQTT - gust
      modem.mqttStatus();
    } else if (serialCommand == "mqtt start 4") {
      Serial.print("Simcom 7070G MQTT Start = ");
      modem.startMQTT(4);  // 0 = HiveMQ server.  // here set the MQTT - gust
      modem.mqttStatus();
    }
    // else if (serialCommand == "mqtt start 1"){
    //   Serial.print("Simcom 7070G MQTT Start = ");
    //   modem.startMQTT(1); // 1 = Unencrypted test server (mosquitto.org)
    //   modem.mqttStatus();
    // }
    // else if (serialCommand == "mqtt start 2"){
    //   Serial.print("Simcom 7070G MQTT Start = ");
    //   modem.startMQTT(2);
    //   modem.mqttStatus();
    // }
    else if (serialCommand == "mqtt status") {
      Serial.print("Simcom 7070G MQTT Status = ");
      modem.mqttStatus();
    } else if (serialCommand == "mqtt connect") {
      Serial.print("Simcom 7070G MQTT Connect = ");
      modem.mqttConnect();  //有控制等待时间的变量
      modem.mqttStatus();
    } else if (serialCommand == "mqtt disconnect") {
      Serial.print("Simcom 7070G MQTT Disonnect = ");
      modem.mqttDisconnect();
    } else if (serialCommand == "mqtt pub") {
      Serial.print("Simcom 7070G MQTT Publish \"a\" ");
      modem.mqttPub("a", 0);
    }
    // else if (serialCommand == "mqtt pub 0"){
    //   Serial.print("Simcom 7070G MQTT Publish \"Test\" ");
    //   modem.mqttPub("Test",1);
    // }

    // Workworkworkworkworkworkworkwork. -----------
    // else if (serialCommand == "mqtt pub 1"){
    //   Serial.print("Simcom 7070G MQTT Publish Data = ");   // here! send message
    //   modem.mqttPub(dataString,1);   // dataString: "test_psu_Jan-30-2024"
    // }

    // loop loop loop loop loop.
    else if (serialCommand == "mqtt pub 1") {
      int nums = 0;
      while (1) {
        unsigned long currentMillis = millis();
        // if (currentMillis - previousMillis >= interval_sleep) {
        //     // Save the last time the calculation was made
        //     previousMillis = currentMillis;
        //     nums += 1;
        //     String numstr = String(nums);

        //     String publish_content = dataString + numstr;
        //     // Perform your task here
        //     Serial.print("Simcom 7070G MQTT Publish Data = \n");   // here! send message
        //     modem.mqttPub(publish_content,1);   // dataString: "test_psu_Jan-30-2024"
        // }
        if (currentMillis - previousMillis >= interval_sleep) {
          // 保存上一次操作的时间
          previousMillis = currentMillis;
          nums += 1;
          String numstr = String(nums);

          String publish_content = dataString + numstr;
          // 循环10次执行任务
          for (int i = 0; i < 10; i++) {
            Serial.print("Simcom 7070G MQTT Publish Data = \n");
            modem.mqttPub(publish_content, 1);  // dataString: "test_psu_Jan-30-2024"

            // 如果不是最后一次循环，则暂停5秒
            if (i < 9) {
              delay(5000);  // 暂停5秒
            }
          }
        }
      }

    }

    // else if (serialCommand == "mqtt pub 2"){
    //   Serial.print("Simcom 7070G MQTT Publish Config = ");
    //   modem.mqttPub(settingsString(),1);
    // }
    else if (serialCommand == "mqtt sub") {
      Serial.print("Simcom 7070G MQTT Subscribe = \n");
      String json;
      modem.mqttSub(json);
      Serial.println(json);
    } else if (serialCommand == "mqtt unsub") {
      Serial.print("Simcom 7070G MQTT Unsubscribe = ");
      modem.mqttUnsub();
    } else if (serialCommand == "mqtt read") {
      Serial.print("Simcom 7070G MQTT Read = ");
      modem.mqttRead(dataString, 1);
    }

    //*********************************************************************************
    // GPS Commands
    // else if (serialCommand == "gps"){
    //   Serial.print("GPS Serial Value = ");
    //   Serial.println(gpsSerial.readResponse());
    // }
    // else if (serialCommand == "gps reset"){
    //   Serial.print("GPS Serial Value = ");
    //   statusGPS = gpsSerial.init();
    //   Serial.println(statusGPS);
    // }
    // else if (serialCommand == "gps raw"){
    //   Serial.print("GPS Serial Value = ");
    //   Serial.println(gpsSerial.readRaw());
    // }
    // else if (serialCommand == "gps time"){
    //   //gps.encode(modem.readGPS());
    //   Serial.print("GPS Serial Time = ");
    //   Serial.println(gpsSerial.time());
    // }
    // else {
    //   Serial.print("Unknown Command: ");
    //   Serial.println(serialCommand);
    // }
  }

  /*
 * Data processing loop - create comma separated value string
 * 
 * 1 - Date
 * 2 - Time
 * 3 - CH4 concentration
 * 4 - CO2 concentration
 * 5 - H2O concentration
 */

  //  Serial.print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~new test~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

  //   // 0. check modem's status
  //   // statusModem = modem.checkStatus();  // if statusModem == 0: it is on
  //   statusModem = modem.init();  // if statusModem == 0: it is on
  //   Serial.print("statusModem: ");
  //   Serial.print(statusModem);

  //   int restart_time = 0;
  //   while(statusModem != 0){       //  if not on, restart it
  //     Serial.print("not start, try to start --" + String(restart_time)+"\n");
  //     restart_time++;
  //     statusModem = modem.init();
  //     delay(3000);
  //   }

  //   Serial.print(modem.enableIP());
  //   Serial.print("Simcom 7070G IP Address = \n");
  //   returnValue = modem.readIP(returnString);
  //   if(!returnValue){
  //     Serial.println(returnString);
  //   }
  //   else{
  //     Serial.println("No Connection");
  //   }
  //   // Serial.print("modem starts up");


  //   // 1. check mqtt status
  //       modem.mqttStatus();
  //       modem.startMQTT(3);
  //       modem.mqttConnect();
  //       delay(1000);

  //   // 2. publish data
  //       // Serial.print("try to send message");
  //       // modem.mqttPub("Hello, I am MQTT",1);
  //       dataString = "test_April-2024_#" + String(count);
  //       // returnValue = modem.mqttWrite(dataString, 1);

  //       for(int i=0; i<10;i++){
  //         String pub_content = dataString+ "_";
  //         pub_content = pub_content + String(i);
  //         returnValue = modem.mqttWrite(pub_content, 1);
  //         delay(5000);
  //       }
  //       count++;

  //       Serial.print("rest for a while");
  //       modem.powerToggle();

  //       delay(300000);  // 20s   here we want to rest for 5min  5*60 * 1000


  Serial.print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Combine-SD-Round-Test~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    // create new file
        logger.fileNewName();

    // 2. collect data, put into SD -- 5 minutes
        Serial.print("Save data to SD card");
        // modem.mqttPub("Hello, I am MQTT",1);
        dataString = "test_April-2024_#" + String(count);
        // returnValue = modem.mqttWrite(dataString, 1);
        // sample/5s, 5 mintues, 12*5=60 samples
        for(int i=0; i<60;i++){
          String pub_content = dataString+ "_";
          pub_content = pub_content + String(i);
          Serial.println(logger.fileAddCSV(pub_content, 0));
          delay(5000);
        }
        count++;
    // start the modem and network
        Serial.println(modem.enableIP());
        statusModem = modem.init();
        int restart_time = 0;
        while(statusModem != 0){       //  if not on, restart it
          Serial.print("not start, try to start --" + String(restart_time)+"\n");
          restart_time++;
          statusModem = modem.init();
          delay(10000);
        }
        Serial.print(modem.enableIP());
        Serial.print("Simcom 7070G IP Address = \n");
        returnValue = modem.readIP(returnString);
        if(!returnValue){
          Serial.println(returnString);
        }
        else{
          Serial.println("No Connection");
        }
    // Serial.print("modem starts up");
    // 1. check mqtt status
        modem.mqttStatus();
        modem.startMQTT(3);
        modem.mqttConnect();
        delay(1000);
        Serial.print("Start to transmit data");
        int init_line = 1;
        int end_line = 10;
        for(int i=0; i<6;i++){
          String TransData = logger.fileReadByLine(FILE_TYPE_DATA, init_line, end_line);
          returnValue = modem.mqttWrite(TransData, 1);
          init_line += 10;
          end_line += 10;
          delay(9000);
        }
        // modem.powerToggle();
        //  delete now file
        Serial.println(modem.disableIP());
        logger.fileRemove(1);


  // Serial.print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Combine-SD-Round-AppEncrypt-Test~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  // // create new file
  // logger.fileNewName();

  // // 2. collect data, put into SD -- 5 minutes
  // Serial.print("Save data to SD card");
  // // modem.mqttPub("Hello, I am MQTT",1);
  // dataString = "test_April-2024_#" + String(count);
  // // returnValue = modem.mqttWrite(dataString, 1);
  // // sample/5s, 5 mintues, 12*5=60 samples
  // for (int i = 0; i < 20; i++) {
  //   String pub_content = dataString + "_";
  //   pub_content = pub_content + String(i);
  //   Serial.println(logger.fileAddCSV(pub_content, 0));
  //   delay(2000);
  // }
  // count++;

  // statusModem = modem.init();
  // int restart_time = 0;
  // while (statusModem != 0) {  //  if not on, restart it
  //   Serial.print("not start, try to start --" + String(restart_time) + "\n");
  //   restart_time++;
  //   statusModem = modem.init();
  //   delay(5000);
  // }
  // Serial.print(modem.enableIP());
  // Serial.print("Simcom 7070G IP Address = \n");
  // returnValue = modem.readIP(returnString);
  // if (!returnValue) {
  //   Serial.println(returnString);
  // } else {
  //   Serial.println("No Connection");
  // }
  // // Serial.print("modem starts up");
  // // config the encryption
  // uint8_t key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
  // uint8_t iv[12] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B };
  // // Define a structure for the encrypted data and its length
  // String pub_content;
  // GCM<AES128> gcm;
  // gcm.setKey(key, sizeof(key));
  // gcm.setIV(iv, sizeof(iv));
  // // Buffer for ciphertext
  // // 1. check mqtt status
  // modem.mqttStatus();
  // modem.startMQTT(4);
  // modem.mqttConnect();
  // delay(1000);
  // Serial.print("Start to transmit data");
  // int init_line = 1;
  // int end_line = 10;
  // for (int i = 0; i < 6; i++) {
  //   String TransData = logger.fileReadByLine(FILE_TYPE_DATA, init_line, end_line);
  //   int dataSize = TransData.length();
  //   uint8_t *plaintext = new uint8_t[dataSize + 1];
  //   TransData.getBytes(plaintext, dataSize + 1);  // Ensures null-termination
  //   // Proceed with encryption, ensuring not to include the null terminator
  //   uint8_t ciphertext[64];
  //   // gcm.encrypt(ciphertext, plaintext, dataSize);
  //   gcm.encrypt(ciphertext, plaintext, sizeof(plaintext) - 1); // Subtract 1 to exclude the null terminator from encryption
  //   // Assuming `encrypt` modifies a global or passed variable for length,
  //   // or you know the length from the size of `plaintext`
  //   size_t len = sizeof(plaintext) - 1 + 16;

  //   gcm.computeTag(ciphertext + sizeof(plaintext) - 1, 16);  // Append the tag
  //   // Convert binary data to a hexadecimal string for transmission
  //   char hexString[128];  // Make sure this is large enough
  //   for (size_t j = 0; j < len; j++) {
  //     sprintf(hexString + (j * 2), "%02x", ciphertext[j]);
  //   }
  //   Serial.print(hexString);
  //   Serial.print("transmit to mqtt");
  //   pub_content = String(hexString);
  //   returnValue = modem.mqttWrite(pub_content, 1);
  //   init_line += 10;
  //   end_line += 10;
  //   delay(9000);
  // }
  // modem.powerToggle();
  // //  delete now file
  // logger.fileRemove(1);

  // Serial.print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Test-Encryption-Package~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  // statusModem = modem.init();
  // modem.mqttStatus();
  // modem.startMQTT(4);
  // modem.mqttConnect();
  // delay(5000);
  // uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  // uint8_t iv[12] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B};
  // // Define a structure for the encrypted data and its length
  // String pub_content;
  // GCM<AES128> gcm;
  // gcm.setKey(key, sizeof(key));
  // gcm.setIV(iv, sizeof(iv));
  // // Buffer for ciphertext
  // uint8_t plaintext[] = "Test-Application-Encryption";
  // uint8_t ciphertext[64];

  // gcm.encrypt(ciphertext, plaintext, sizeof(plaintext) - 1); // Subtract 1 to exclude the null terminator from encryption

  // // Assuming `encrypt` modifies a global or passed variable for length,
  // // or you know the length from the size of `plaintext`
  // size_t len = sizeof(plaintext) - 1 + 16;

  // gcm.computeTag(ciphertext + sizeof(plaintext) - 1, 16); // Append the tag

  // // Convert binary data to a hexadecimal string for transmission
  // char hexString[128]; // Make sure this is large enough
  // for (size_t i = 0; i < len; i++) {
  //   sprintf(hexString + (i * 2), "%02x", ciphertext[i]);
  // }

  // Serial.print(hexString);
  // pub_content = String(hexString);
  // returnValue = modem.mqttWrite(pub_content, 1);
  // delay(5000);
  // modem.mqttDisconnect();



    // Serial.print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Test-Remote Control by MQTT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    if (intervalCfg && (millis()-timerLastCfg) > intervalCfg){
          // MQTT publish data and receive commands
          Serial.println("Try to subscribe mqtt data\n");
          modem.startMQTT(0); 
          modem.mqttConnect(); 
          returnValue = modem.mqttRead(returnString, 1);
          delay(10000);
          modem.mqttDisconnect(); 
          // String command = "intervalData=1000 ;intervalMQTT= 200 ;intervalCfg=3;intervalLog=50;intervalBackup=60;";
          
          // // Parse the command
          parseCommand(returnString);
          
          // // Output the results
          Serial.print("intervalData = ");
          Serial.println(intervalData);
          Serial.print("intervalUpdate = ");
          Serial.println(intervalUpdate);
          Serial.print("intervalCfg = ");
          Serial.println(intervalCfg);
          Serial.print("intervalLog = ");
          Serial.println(intervalLog);
          Serial.print("intervalBackup = ");
          Serial.println(intervalBackup);
    }
    // end Test-Remote Control by MQTT





  //   if ((millis()-timerLastRead) > intervalData){

  //     timerLastRead = millis();
  //     int option = (intervalUpdate != 0);
  //     dataString = ',' + String(millis()/1000) + ',';
  //     int test = k96.readCSVString(dataString);                              // Sensor data
  //     if (statusGPS == DEVICE_ENABLED){
  //       dataString += gpsSerial.readResponse();
  //     }
  //     if (statusModem == DEVICE_ENABLED && logModemTime){
  //       modem.readClock(0, returnString);
  //       dataString.concat(returnString);
  //     }
  //     if (statusModem == DEVICE_ENABLED && logModemPower){
  //       dataString += "," + modem.readSignal();
  //     }

  // //SDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSD
  // // Store the data on local SD card (can be disabled)
  // // Check file size for disk management, change filename if necessary
  // // Display external error message using opCode
  //     if(intervalFile == DEVICE_ENABLED){
  //       statusSD = logger.fileAddCSV(dataString, 0);
  //       fileSizeLimit = (logger.fileCheckSize() > FILE_MAX_SIZE);
  //       dataSizeLimit = ((logger.fileCheckSize() - lastFTPByteBackup) > DATA_MAX_SIZE) || fileSizeLimit;
  //       if (statusSD > 0){
  //         control.blinkCode(3);
  //       }
  //       else if (test != 0){
  //         control.blinkCode(2);
  //       }
  //       else {
  //         control.blinkCode(1);
  //       }
  //     }
  //     else{
  //       if (test != 0){
  //         control.blinkCode(1);
  //       }
  //       else if (statusSD > 0){
  //         control.blinkCode(2);
  //       }
  //     }

  //     // MQTT
  //     //returnValue = modem.mqttRead(returnString, 1);
  //     returnValue = modem.mqttWrite(dataString, 1);

  //     //FTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTPFTP
  //     // Update the data file on the FTP server
  //     if (intervalUpdate && (millis()-timerLastDataFile) > intervalUpdate){
  //       if (statusSD == 0){
  //         File tempFile = logger.fileOpen(1);
  //         returnValue = modem.ftpPut(tempFile,1);
  //         tempFile.close();
  //       }
  //       else {
  //         returnValue = modem.ftpPut(logger.fileRead(FILE_TYPE_BACKUP),FILE_TYPE_DATA);
  //       }
  //       if (!returnValue){
  //         logger.fileAddCSV((modem.readClock(0)+": File upload (bytes) = " + String(logger.fileSize(FILE_TYPE_BACKUP))),FILE_TYPE_LOG);
  //         logger.fileRemove(1);
  //         timerLastDataFile = millis();
  //         statusFTP = 0;
  //       }
  //       // Failed FTP, increment the consecutive failure counter
  //       else{
  //         statusFTP += 1;
  //         logger.fileAddCSV((modem.readClock(0)+": File UPLOAD ERROR data upload"),2);
  //       }

  //     }

  //       // LOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOGLOG
  //       // Update the system log, append only the bytes since the last successful FTP transfer
  //       else if (intervalLog && (millis()-timerLastLog) > intervalLog){
  //         Serial.println("Log File Upload Started");
  //         int fileSize = 0;

  //         if (statusSD == 0){                           // SD card present, upload file
  //           File tempFile = logger.fileOpen(FILE_TYPE_LOG);
  //           tempFile.seek(lastFTPByteLog);
  //           returnValue = modem.ftpPut(tempFile,FILE_TYPE_LOG);
  //           fileSize = tempFile.size();
  //           tempFile.close();
  //         }
  //         else{                                         // SD card virtual, upload string
  //           returnValue = modem.ftpPut(logger.fileRead(FILE_TYPE_LOG),FILE_TYPE_LOG);
  //         }
  //         // Successful FTP, record in log, set number bytes, reset timer
  //         if (!returnValue){
  //           Serial.println("Log File Upload Completed");
  //           logger.fileAddCSV((modem.readClock(0)+": Log upload (bytes) = " + String(logger.fileSize(FILE_TYPE_LOG))),FILE_TYPE_LOG);
  //           timerLastLog = millis();
  //           statusFTP = 0;
  //           lastFTPByteLog = fileSize;
  //         }
  //         else{
  //           Serial.println("Log File Upload Failed");
  //           logger.fileAddCSV((modem.readClock(0)+": Log file upload failed"),2);
  //           statusFTP += 1;
  //         }

  //       }
  //       //CONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIGCONFIG
  //       // Not updating the data file
  //       else if (intervalCfg && (millis()-timerLastCfg) > intervalCfg){
  //         Serial.println("Configuration File Download Started");
  //         String json;
  //         if (MODE_CTRL == MODE_CFG_FTP){
  //           returnValue = modem.ftpGet(json);
  //         }
  //         else {
  //           returnValue = modem.mqttRead(json, 1);
  //         }
  //         // Server connection test, success => 0
  //         if (returnValue == 0){
  //           returnValue = updateConfig(json);
  //           if (returnValue < -1){
  //             Serial.println("Configuration File Download Failed");
  //             logger.fileAddCSV((modem.readClock(0)+": Invalid settings"),2);
  //             timerLastCfg = millis();
  //             statusFTP += 1;
  //           }
  //           else if (returnValue == -1){
  //             Serial.println("Configuration File Download Invalid");
  //             logger.fileAddCSV((modem.readClock(0)+": Config Download: Invalid file"),2);
  //             timerLastCfg = millis();
  //           }
  //           else if (returnValue >0){
  //             Serial.println("Configuration File Download Completed");
  //             logger.fileAddCSV((modem.readClock(0)+": Settings Updated = " + settingsString()),2);
  //             timerLastCfg = millis();
  //             statusFTP = 0;
  //           }
  //           else{
  //             statusFTP = 0;
  //             timerLastCfg = millis();
  //           }
  //         }
  //         // Server connection test, failure => -1
  //         else{
  //           Serial.println("Configuration File Download Failed");
  //           logger.fileAddCSV((modem.readClock(0)+": Configuration update failed"),2);
  //           timerLastCfg = millis();
  //           statusFTP += 1;
  //         }
  //       }
  //       //BACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUPBACKUP
  //       // Upload the dated CSV file
  //       else if (((intervalBackup && (millis()-LastFileUpload) > intervalBackup)) || dataSizeLimit){

  //         File tempFile = logger.fileOpen(FILE_TYPE_DATA);
  //         tempFile.seek(lastFTPByteBackup);
  //         returnValue = modem.ftpPut(tempFile,FILE_TYPE_DATA);

  //         // Check if file was uploaded correctly
  //         if (!returnValue){
  //           logger.fileAddCSV((modem.readClock(0)+": Backup file updated"),2);
  //           LastFileUpload = millis();
  //           statusFTP = 0;
  //           lastFTPByteBackup = tempFile.size();
  //           if (fileSizeLimit){
  //             logger.fileNewName();
  //             logger.fileAddCSV((modem.readClock(0)+": File size limit. New file"),2);
  //             lastFTPByteBackup = 0;
  //           }
  //           countLastDataFile++;
  //           writeStatus();
  //         }
  //         // File was not updated, do not increment counter but create new file
  //         else {
  //           logger.fileAddCSV((modem.readClock(0)+": File backup failed"),2);
  //           statusFTP += 1;
  //         }

  //         tempFile.close();
  //       }
  //       //ERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERROR
  //       // After 5 consecutive failures reset the power to the modem
  //       if (statusFTP >= 5){
  //         logger.fileAddCSV((modem.readClock(0)+": FTP ERRORS, Modem reset"),2);
  //         Serial.println("Modem reset");
  //         modem.disableIP();
  //         delay(1000);
  //         statusFTP = 0;
  //         modem.powerToggle();
  //       }
  //       //POWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWERPOWER
  //       // If sufficient delay until next modem action, disable network connection
  //       //if (powerSave && intervalUpdate >= 45000){
  //       if (powerSave && intervalUpdate >= 45000){
  //         modem.disableIP();
  //       }
  //   }
}

/* *********************************************************************************************** 
 *   updateConfig()
 * 
 *  Uses the modems FTP Get function to download a file: "config.json"
 *  Currently searches for tags for three variables
 *  intervalData, intervalDelay, intervalBackup
 *  
 *  Return: error if file is not found, or tag is not present setting is not updated
 *  1  : file found, settings updated
 *  0  : file found, no change
 *  -1 : file not found
 *  -2 : file found, incorrect format
 ***********************************************************************************************/

int updateConfig(String json) {

  String result;
  int found;
  int num;
  int change = 0;

  // Empty string indicates file not opened
  if (json.length() == 0) {
    return -2;
  }
  found = json.indexOf("intervalData=");
  if (found >= 0) {
    String result = json.substring(found + 13, json.indexOf(";", found + 13));
    num = result.toInt();
    if (num != intervalData) {
      if (num && num > INTERVAL_DATA_MAX && num < INTERVAL_DATA_MIN) return -2;
      intervalData = num;
      change = 1;
    }
  }
  found = json.indexOf("intervalFTP=");
  if (found >= 0) {
    String result = json.substring(found + 12, json.indexOf(";", found + 12));
    num = result.toInt();
    if (num != intervalUpdate) {
      if (num && num > INTERVAL_FTP_MAX && num < INTERVAL_FTP_MIN) return -2;
      intervalUpdate = num;
      change = 1;
    }
  }
  found = json.indexOf("intervalCfg=");
  if (found >= 0) {
    String result = json.substring(found + 12, json.indexOf(";", found + 12));
    num = result.toInt();
    if (num != intervalCfg) {
      if (num && num > INTERVAL_CFG_MAX && num < INTERVAL_CFG_MIN) return -2;
      intervalCfg = num;
      change = 1;
    }
  }
  found = json.indexOf("intervalLog=");
  if (found >= 0) {
    String result = json.substring(found + 12, json.indexOf(";", found + 12));
    num = result.toInt();
    if (num != intervalLog) {
      if (num && num > INTERVAL_LOG_MAX && num < INTERVAL_LOG_MIN) return -2;
      intervalLog = num;
      change = 1;
    }
  }
  found = json.indexOf("intervalBackup=");
  if (found >= 0) {
    String result = json.substring(found + 15, json.indexOf(";", found + 15));
    num = result.toInt();
    if (num != intervalBackup) {
      if (num > INTERVAL_BACKUP_MAX && num < INTERVAL_BACKUP_MIN) return -2;
      intervalBackup = num;
      change = 1;
    }
    return change;
  }
  return -1;
}

/******************************************************************************
 * Parse the status file
 *
 * Search input string for variable tags
 * Only update found elements, ignore missing elements

 * Error: Invalid values for 
 *****************************************************************************/

int readStatus() {
  String json;
  File statusFile = logger.fileOpen(FILE_TYPE_LOG);

  while (statusFile.available()) {
    Serial.print(String(statusFile.read()));
    //json.concat(statusFile.read());
  }
  statusFile = logger.fileOpen(FILE_TYPE_STATUS);

  while (statusFile.available()) {
    Serial.print(statusFile.read());
    //json.concat(statusFile.read());
  }

  int found = json.indexOf(STATUS_TAG_FILE);
  String result = json.substring(found + 14, json.indexOf(";", found + 14));
  countLastDataFile = result.toInt();
  statusFile.close();

  Serial.println(json);

  return updateConfig(json);
}

int writeStatus() {
  String json = STATUS_TAG_FILE + String(countLastDataFile) + ";";
  json.concat(settingsString());
  logger.fileAddCSV(json, FILE_TYPE_STATUS);
  return 0;
}

/*
 * Puts the system settings in a printable string
 */

String settingsString() {
  return "intervalData=" + String(intervalData) + ";intervalFTP=" + String(intervalUpdate) + ";intervalCfg=" + String(intervalCfg) + ";intervalLog=" + String(intervalLog) + ";intervalBackup=" + String(intervalBackup);
}

/*
 * Function to parse the configuration command
 */

void parseCommand(String command) {
    // Map keys to variable addresses
    struct Config {
        const char* key;
        int* variable;
    };
    
    extern int intervalData;     // Ensure these variables are declared elsewhere
    extern int intervalUpdate;
    extern int intervalCfg;
    extern int intervalLog;
    extern int intervalBackup;

    Config configMap[] = {
        {"intervalData", &intervalData},
        {"intervalUpdate", &intervalUpdate},
        {"intervalCfg", &intervalCfg},
        {"intervalLog", &intervalLog},
        {"intervalBackup", &intervalBackup},
    };
    
    const int configMapSize = sizeof(configMap) / sizeof(Config);

    // Split the command by semicolons
    int start = 0;
    while (start < command.length()) {
        int end = command.indexOf(';', start);
        if (end == -1) {
            end = command.length();
        }
        
        String item = command.substring(start, end);
        start = end + 1;

        // Split each item by the equals sign
        int pos = item.indexOf('=');
        if (pos != -1) {
            String key = item.substring(0, pos);
            key.trim(); // Correct usage of trim

            String valueStr = item.substring(pos + 1);
            valueStr.trim(); // Correct usage of trim

            int value = valueStr.toInt();

            // Set the corresponding variable
            for (int i = 0; i < configMapSize; i++) {
                if (key == configMap[i].key) {
                    *configMap[i].variable = value;
                    break;
                }
            }
        }
    }
}
