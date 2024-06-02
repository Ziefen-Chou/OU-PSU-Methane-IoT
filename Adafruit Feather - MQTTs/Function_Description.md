This is the description of the code function and how to use it.

-------------------------------------------------------------------------------------------------------------------------------------
About SD card operation
Note: 
(1) When you operating a specified file, you need to make sure that this file is opened, for the Arduino, you can only open one file, so if you want operate another file, you have to close now opening file;
(2) The Arduino not support the direct modifications for existing file, so you have no idea to change the content of existing file.
-------------------------------------------------------------------------------------------------------------------------------------
1) logger.fileAddCSV(String csvString, int option);
    Add the message to file (the file type is defined by "int option", you need to pre-open this type file by "logger.fileOpen(int option)" function);
    String csvString: the message you want to add;
    int option: which type of file you want to add (0 for sensor's data, 1 for backup file, 2 for log file, 3 for module's status).

2) logger.logNewName();
    Create a new log file (.txt).

3) logger.fileNewName();
    Create a new data file (.csv).

4) logger.fileOpen(int option);
    int option: 0 for sensor's data, 1 for backup file, 2 for log file, 3 for module's status.

5) logger.fileCheckSize();
    Check the current ".csv" file size.
   logger.SpeFileCheckSize(String FileName);
    Check the specified file's size.

6) logger.speFileAddCSV(String csvString, int option, String FileName);
    Add the message to specified file;
    String csvString: the message you want to add;
    int option: which type of file you want to add (0 for sensor's data, 1 for backup file, 2 for log file, 3 for module's status);
    String FileName: the specified file, it should be corresping to "int option", ".csv" suffix for 0 and ".txt" suffix for other.

7) logger.fileNameString();
    Returen current global variable file name (data .csv file).

8) logger.fileDump(int option);
    int option: 0 for sensor's data, 1 for backup file, 2 for log file, 3 for module's status, 4 for ".crt" file, 5 for ".pem" file, 6 for ".key" file.
    This function is used to read the data from specified type file (current global variable file name).

9) logger.SpeFileReadByLine(int option, int startLine, int endLine, String FileName);
    int option: which type of file you want to add (0 for sensor's data, 1 for backup file, 2 for log file, 3 for module's status);
    int startLine, int endLine: the starting and ending line;
    String FileName: the specified file, it should be corresping to "int option", ".csv" suffix for 0 and ".txt" suffix for other.

10) logger.fileRemove(1);
    Delete current global variable ".csv" file.

11) logger.fileRemove_Specified(1, String FILE);
    String FILE: the file you want to delete (".csv" file).

12) 





-------------------------------------------------------------------------------------------------------------------------------------
About communication module operation
-------------------------------------------------------------------------------------------------------------------------------------
1) modem.readClock(int format, String &clockString);
    int format: define the output form (0 for YY/MM/DD and time with separator, 1 for YY/MM/DD, other for YY/MM/DD and exact time);
                For example, 0 for 23/05/30,18:30:00-12;
                             1 for 230530;
                             other for 230530_183000.

2) modem.readSignal();
    Get the data signal strength.

3) modem.startNTP();
    Re-calibrate the time.

4) modem.init();
    Boot module.

5) modem.disableIP();
    Turn off the network.

6) modem.enableIP();
    Turn on the network.

7) 


-------------------------------------------------------------------------------------------------------------------------------------
About MQTT operation
-------------------------------------------------------------------------------------------------------------------------------------
1) modem.startMQTT(int option);
    int option: the configuration you want (like the server ip, client ID, username and password, etc.);
    For the configuration, you can add yours at the file "SimModem.cpp" according to the function defination "int SimModem::startMQTT(int option){}".

2) modem.mqttStatus();
    Get the status of MQTT server (like the connection).

3) modem.mqttWrite(String &message, int option);
    String &message: The data you want to transmit;
    int option: the configuration you want (like the server ip, client ID, username and password, etc.).

4) modem.mqttConnect();
    Try to connect the MQTT server according to the configuration from "modem.startMQTT(int option);".

5) modem.mqttSub(String &message);
    According to the configuration to subscribe the specified topic's message.

6) modem.mqttUnsub();
    Cancel the current subscription.

7) 





-------------------------------------------------------------------------------------------------------------------------------------About periodic delection: The "// delete expired files //" part at main loop
-------------------------------------------------------------------------------------------------------------------------------------
Basic idea: 
    1) There is a file named "DTS.TXT" which is used to store each data file's name and its time stamp;
    2) For each data file, when its size exxceeds the limitation, we will store its name and the last sampling time to the "DTS.TXT";
    3) Begin every loop, we will examine the current time and the oldest time stamp at "DTS.CSV";
    4) If the time difference of more than seven days, then iterate to find all filenames in the file ("DTS.TXT") whose timestamp is more than seven days from the current time;
    5) Delete these files and keep other file name and stamp which not satisfies the condition.