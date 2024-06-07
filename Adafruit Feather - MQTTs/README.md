![image](https://github.com/Ziefen-Chou/OU-PSU-Methane-IoT/assets/59044637/4e579b71-c114-45b4-9cf7-5b2f09827a58)# OU-PSU-Methane-IoT
5/28/2024：
PSU：Add function of "Periodic deletion of obsolete documents (one week)";
You should pre-create a file name "DTS.TXT" in the SD card.

6/02/2024:
PSU: Add a function of remote control via MQTT server.
Step:
	1. Modified the subscribed topic in "SimModem.h"
	#define AT_MQT_SUB          "AT+SMSUB=\"aimnet/commands1\",1"
	2. Add function -- parseCommand(String command) in the FeatherSenseairWiFiServer.ino (the end of the file)
	3. Read MQTT commands and update parameters (see the codes).

Notice: the publish data in server-side should follow the following form:
"intervalData=xxx;intervalUpdate=xxx;intervalCfg=xxx;intervalLog=xxx;intervalBackup=xxx;"
