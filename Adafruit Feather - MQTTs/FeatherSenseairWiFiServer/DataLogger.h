/* DataLogger.h - Library for SD card file management

  Functions for saving, deleting, or printing text files to and SD card.
  
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

#ifndef DataLogger_h
#define DataLogger_h

#define SD_CS             4
#define TEMP_DATA         "d0000000.csv"
#define STATUS_DATA       "status.txt"
#define STATUS_TAG_FILE   "lastFileIndex="
#define STATUS_TAG_TEST   "lastTestIndex="
#define FILE_MAX_SIZE     32000 
#define DATA_MAX_SIZE     32000

#define LOG_FILE_NAME     "L"
#define DATA_FILE_NAME    "D"

#define CRT_FILE_NAME    "client.crt"
#define PEM_FILE_NAME    "CA.pem"
#define KEY_FILE_NAME    "client.key"

#define FILE_TYPE_DATA      0
#define FILE_TYPE_BACKUP    1
#define FILE_TYPE_LOG       2
#define FILE_TYPE_STATUS    3
#define FILE_TYPE_CRT       4
#define FILE_TYPE_PEM       5
#define FILE_TYPE_KEY       6

#define FILE_REAL           0
#define FILE_VIRTUAL        1

#include <SPI.h>
#include <SD.h>
    
class DataLogger
{
  private:
    int status = -1;
    String fileName = "d0000000.csv";
    String logName =  "l0000000.txt";
    int logFileCount = -1;
    int dataFileCount = -1;

    String backupData = "";
    String backupLog = "";

  public:
    DataLogger();
    int init();
    int fileDump(int option);
    String fileReadByLine(int option, int startLine, int endLine);
    String SpefileReadByLine(String FileName, int startLine, int endLine);
    int fileRemove(int option);
    int fileRemove_Specified(int option, String FILE);
    void fileRemoveAll();
    int fileCheckSize();
    
    int fileAddCSV(String csvString, int option);
    
    void fileDir(File dir, int numTabs);
    
    int fileCount(File dir);
    int fileCount(File dir, String fileName);
    int DTSADD(String csvString, int option);

    String AppEncrypt(String OriginMessage);  // application-layer encryption
    
    void fileNewName();
    String fileNameString();
    void logNewName();
    File fileOpen(int option);

    String fileRead(int option);
    int fileSize(int option);
  
};

#endif
