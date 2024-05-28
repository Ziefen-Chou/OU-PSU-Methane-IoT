This is the Arduino IDE code for the Adafruit Feather.

-------------------------------------------------------------------------------------------------------------------------------------
Added by PSU - Encryption and energy saving:
-------------------------------------------------------------------------------------------------------------------------------------
(1) Application-Layer Encryption:
    "AppEncrypt" function which is defined in "DataLogger.cpp"
    Using method: encrypted_message = logger.AppEncrypt(String_to_be_encrypted);
    Current function defects: It is not very stable, so can't run continuously for a long time (In debugging).

(2) Sleep of modem for energy saving
    "sleep" function which is defined in "SimModem.cpp"
    Using method: modem.sleep(1); ----- Enter sleeping mode
                  modem.sleep(0); ----- Exit sleeping mode


-------------------------------------------------------------------------------------------------------------------------------------
Added by PSU - Some operations on the SD:
-------------------------------------------------------------------------------------------------------------------------------------
All of below functions are defined in "DataLogger.cpp"
(1) Create new csv file:
    Using method: logger.fileNewName();
    (Note: The newly created file is directly overwritten by the global variable, so the collected date after creating new file will be stored in the new file)

(2) Add date to csv file:
    Using method: logger.fileAddCSV(collected_data, FILE_TYPE_DATA);

(3) Check the size of file:
    a) Check current file (global variable) size: logger.fileCheckSize();
    b) Check specified file size: logger.SpeFileCheckSize(file_you_want_to_check);

(4) Delete specified file
    Using method: logger.fileRemove_Specified(1, file_you_want_to_delete);

(5) Read data from specified file by lines
    Using method: logger.SpeFileReadByLine(FILE_TYPE_DATA, 1, 10, the_file_you_want_to_read);
