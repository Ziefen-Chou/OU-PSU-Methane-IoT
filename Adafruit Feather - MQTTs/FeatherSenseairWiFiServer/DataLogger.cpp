/*  DataLogger.cpp - Library for SD card operations
 *  
 * Copyright (c) 2022 University of Oklahoma.  All right reserved.

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
#include "DataLogger.h"

DataLogger::DataLogger() {
}

int DataLogger::init() {
  // see if the card is present and can be initialized:
  if (SD.begin(SD_CS)) {
    status = 0;
    File root;
    root = SD.open("/");
    logFileCount = fileCount(root, LOG_FILE_NAME);
    dataFileCount = fileCount(root, DATA_FILE_NAME);
    root.close();
  } else {
    // Virtual files for log and data
    backupData = "";
    backupLog = "";
    status = 1;
  }
  return status;
}

/*
 *  Transmit file data over serial port
 */

int DataLogger::fileDump(int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    String temp;
    if (option == FILE_TYPE_DATA) {
      temp = fileName;
    } else if (option == FILE_TYPE_BACKUP) {
      temp = TEMP_DATA;
    } else if (option == FILE_TYPE_LOG) {
      temp = logName;
    } else if (option == FILE_TYPE_STATUS) {
      temp = STATUS_DATA;
    } else if (option == FILE_TYPE_CRT) {
      temp = CRT_FILE_NAME;
    } else if (option == FILE_TYPE_PEM) {
      temp = PEM_FILE_NAME;
    } else if (option == FILE_TYPE_KEY) {
      temp = KEY_FILE_NAME;
    }
    File dataFile = SD.open(temp);

    // if the file is available, write to it:
    if (dataFile) {
      while (dataFile.available()) {
        Serial.write(dataFile.read());
      }
      dataFile.close();
      return 0;
    }
  }

  // if the file isn't open, pop up an error:
  else {
    switch (option) {
      case FILE_TYPE_DATA:
        Serial.print(backupData);
        break;
      case FILE_TYPE_LOG:
        Serial.print(backupLog);
        break;
      default:
        Serial.print(backupData);
    }
    return 1;
  }
  return -1;
}

/*
 * Delete a file on the SD card
 */

// int DataLogger::fileRemove(int option, String Filename){
int DataLogger::fileRemove(int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    // String file = Filename;
    String file = fileName;
    // if (option) file = TEMP_DATA;
    if (SD.exists(file)) {
      SD.remove(file);
      return 0;
    } else {
      return -1;
    }
  }
  return -1;
}

/*
 * Delete all files on SD card
 */
void DataLogger::fileRemoveAll() {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    File root;
    root = SD.open("/");
    while (true) {

      File entry = root.openNextFile();
      if (!entry) {
        // no more files
        break;
      }
      SD.remove(entry.name());
      entry.close();
    }
    root.close();
  }
}

/*
 * Check the size of the current data file
 */
int DataLogger::fileCheckSize() {
  if (!status) {
    File dataFile = SD.open(fileName);
    int fileSize = dataFile.size();
    dataFile.close();
    return fileSize;
  }
  return -1;
}

/*
 *  Add a single line of text to a new/existing file
 */
int DataLogger::fileAddCSV(String csvString, int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    File dataFile;

    // if the file is available, write to it, else log error
    if (option <= FILE_TYPE_DATA) {
      dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        dataFile.println(csvString);
        dataFile.close();
        return 0;
      }
    }

    if (option == FILE_TYPE_BACKUP) {
      dataFile = SD.open(fileName, FILE_WRITE);
      // if the file is available, write to it, else log error
      if (dataFile) {
        dataFile.println(csvString);
        dataFile.close();
        return 0;
      }
    } else if (option == FILE_TYPE_LOG) {
      dataFile = SD.open(logName, FILE_WRITE);
      // if the file is available, write to it, else log error
      if (dataFile) {
        dataFile.println(csvString);
        dataFile.close();
        return 0;
      }
    }
    // Status file, contains info on last network and data file operations
    else if (option == FILE_TYPE_STATUS) {
      SD.remove(STATUS_DATA);
      dataFile = SD.open(STATUS_DATA, FILE_WRITE);
      // if the file is available, write to it, else log error
      if (dataFile) {
        dataFile.println(csvString);
        dataFile.close();
      } else {
        status = 1;
      }
    }
  } else if (status == 1) {
    if (option <= FILE_TYPE_DATA) {
      backupData.concat(csvString);
      backupData.concat('\n');
      if (backupData.length() > 3000) {
        backupData = backupData.substring(backupData.indexOf('\n') + 1, backupData.length());
      }
    } else if (option == FILE_TYPE_LOG) {
      backupLog.concat(csvString);
      backupLog.concat('\n');
      if (backupLog.length() > 1000) {
        backupLog = backupLog.substring(backupLog.indexOf('\n') + 1, backupLog.length());
      }
    }
    return 1;
  }
  return -1;
}

/*
 *  List the files in the directory
 */

void DataLogger::fileDir(File dir, int numTabs) {

  if (!status) {
    while (true) {

      File entry = dir.openNextFile();
      if (!entry) {
        // no more files
        break;
      }
      for (uint8_t i = 0; i < numTabs; i++) {
        Serial.print('\t');
      }
      Serial.print(entry.name());
      if (entry.isDirectory()) {
        Serial.println("/");
        fileDir(entry, numTabs + 1);
      } else {
        // files have sizes, directories do not
        Serial.print("\t\t");
        Serial.println(entry.size(), DEC);
      }
      entry.close();
    }
  }
}

/*
 * The total number of files in the directory
 */
int DataLogger::fileCount(File dir) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    int numFiles = 0;
    while (true) {

      File entry = dir.openNextFile();
      if (!entry) {
        // no more files
        return numFiles;
      }
      numFiles += 1;
      entry.close();
    }
  }
  return -1;
}

/*
 * The number of files containing the string input: fileName
 */
int DataLogger::fileCount(File dir, String fileName) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  int numFiles = 0;
  while (true) {

    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      return numFiles;
    }
    if (String(entry.name()).indexOf(fileName) >= 0) {
      numFiles += 1;
    }
    entry.close();
  }
}

/*
 *  Create a new file on the SD card using the file type count
 *  File names are limited to 8 characters: dXXXXXXX.csv
 *  X is the file number 0000000 - 9999999
 */
void DataLogger::fileNewName() {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (dataFileCount >= 0) {
    fileName = DATA_FILE_NAME + String(dataFileCount) + ".csv";
  } else {
    fileName = String(DATA_FILE_NAME) + "x.csv";
  }

  while (SD.exists(fileName)) {
    dataFileCount++;
    fileName = DATA_FILE_NAME + String(dataFileCount) + ".csv";
  }
}

/*
 * opens one of the file objects associated with current data stream
 */
String DataLogger::fileNameString() {

  return fileName;
}

/*
 *  Create a new file on the SD card using the date string
 *  File names are limited to 8 characters: YYMMDD_x.csv
 *  x (maximum 62) = 0-9,A-Z,a-z
 */
void DataLogger::logNewName() {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File root;
  root = SD.open("/");
  logName = LOG_FILE_NAME + String(logFileCount) + ".txt";
  root.close();
}

/*
 * opens one of the file objects associated with current data stream
 * 
 * Input Option (type definitions)
 */
File DataLogger::fileOpen(int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    if (option == FILE_TYPE_BACKUP) {
      return SD.open(TEMP_DATA);
    } else if (option == FILE_TYPE_LOG) {
      return SD.open(logName);
    } else if (option == FILE_TYPE_STATUS) {
      return SD.open(STATUS_DATA);
    } else if (option == FILE_TYPE_CRT) {
      return SD.open(CRT_FILE_NAME);
    } else if (option == FILE_TYPE_PEM) {
      return SD.open(PEM_FILE_NAME);
    } else if (option == FILE_TYPE_KEY) {
      return SD.open(KEY_FILE_NAME);
    } else {
      return SD.open(fileName);
    }
  } else {
    File temp;
    temp.write("test");
    return temp;
  }
}

/*
 * Returns the string from the virtual file associated with option
 * 
 * Input Option (type definitions)
 */
String DataLogger::fileRead(int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  String temp;
  if (option == FILE_TYPE_BACKUP) {
    temp = backupData;
  } else if (option == FILE_TYPE_LOG) {
    temp = backupLog;
  } else if (option == FILE_TYPE_STATUS) {
    temp = backupData;
  } else if (option == FILE_TYPE_CRT) {
    temp = backupData;
  } else if (option == FILE_TYPE_PEM) {
    temp = backupData;
  } else if (option == FILE_TYPE_KEY) {
    temp = backupData;
  } else {
    temp = backupData;
  }
  return temp;
}

/*
 * opens one of the file objects associated with current data stream
 * 
 * Input Option (type definitions)
 */
int DataLogger::fileSize(int option) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  int size;
  if (option == FILE_TYPE_BACKUP) {
    size = backupData.length();
  } else if (option == FILE_TYPE_LOG) {
    size = backupLog.length();
  } else if (option == FILE_TYPE_STATUS) {
    size = backupData.length();
  } else if (option == FILE_TYPE_CRT) {
    size = backupData.length();
  } else if (option == FILE_TYPE_PEM) {
    size = backupData.length();
  } else if (option == FILE_TYPE_KEY) {
    size = backupData.length();
  } else {
    size = backupData.length();
  }
  return size;
}


// PSU-added code
String DataLogger::fileReadByLine(int option, int startLine, int endLine) {
  if (!status) {
    String temp;
    if (option == FILE_TYPE_DATA) {
      temp = fileName;
    } else if (option == FILE_TYPE_BACKUP) {
      temp = TEMP_DATA;
    } else if (option == FILE_TYPE_LOG) {
      temp = logName;
    } else if (option == FILE_TYPE_STATUS) {
      temp = STATUS_DATA;
    } else if (option == FILE_TYPE_CRT) {
      temp = CRT_FILE_NAME;
    } else if (option == FILE_TYPE_PEM) {
      temp = PEM_FILE_NAME;
    } else if (option == FILE_TYPE_KEY) {
      temp = KEY_FILE_NAME;
    }
    File dataFile = SD.open(temp);
    if (dataFile) {
      int currentLine = 1;
      String result = "";  // 初始化结果字符串
      while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        if (currentLine >= startLine && currentLine <= endLine) {
          result += line;  // 添加行到结果
          if (!line.endsWith("\n")) {
            result += '\n';  // 确保输出是完整的一行
          }
        }
        if (currentLine > endLine) {
          break;  // 如果已经读到endLine，结束循环
        }
        currentLine++;
      }
      dataFile.close();
      return result;  // 返回收集的行
    } else {
      return "Failed to open file.";  // 返回打开文件失败的信息
    }
  } else {
    return "Error: File is already open, cannot proceed.";  // 文件已经打开的错误信息
  }
}

int DataLogger::fileRemove_Specified(int option, String FILE) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (!status) {
    // String file = Filename;
    String file = FILE;
    // if (option) file = TEMP_DATA;
    if (SD.exists(file)) {
      SD.remove(file);
      return 0;
    } else {
      return -1;
    }
  }
  return -1;
}