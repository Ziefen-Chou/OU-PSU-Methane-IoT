#define WIFI_ID       "nanophotonics"
#define WIFI_PWD      "123456789"

//#define IP_PING       "AT+SNPING4=\"ftp.weng.oucreate.com\",1,16,1000"
// #define IP_PING       "AT+SNPING4=\"ftp.caps.ou.edu\",1,16,1000"
#define IP_PING       "AT+SNPING4=\"www,google.com\",1,16,1000"
//#define FTP_SERVER  "AT+FTPSERV=\"ftp.weng.oucreate.com\""
//#define FTP_UN      "AT+FTPUN=\"capstone@weng.oucreate.com\""
//#define FTP_PWD     "AT+FTPPW=\"OUcapstone22\""

#define FTP_SERVER                "AT+FTPSERV=\"ftp.caps.ou.edu\""
#define FTP_UN                    "AT+FTPUN=\"anonymous\""
#define FTP_PWD                   "AT+FTPPW=\"\""

// #define MQTT_SERVER               "AT+SMCONF=\"URL\",b97b659315cf4f0cafd48b90e3421aa6.s2.eu.hivemq.cloud,8883"
// #define MQTT_SERVER               "AT+SMCONF=\"URL\",broker.hivemq.com,1883"
#define MQTT_SERVER               "AT+SMCONF=\"URL\",gust.caps.ou.edu,1883"  // Non-encrypted
#define MQTT_SERVER_GUST_SSL      "AT+SMCONF=\"URL\",gust.caps.ou.edu,8883"
#define MQTT_SERVER_TEST_BASIC    "AT+SMCONF=\"URL\",test.mosquitto.org,1883"
#define MQTT_SERVER_TEST_SSL      "AT+SMCONF=\"URL\",test.mosquitto.org,8883"
// #define MQTT_UN                   "AT+SMCONF=\"USERNAME\",psunec"
#define MQTT_UN                   "AT+SMCONF=\"USERNAME\",msense_device"

#define MQTT_SSLFILECALL          "AT+SMSSL=2,\"ca.crt\",\"myclient.crt\"" 

#define MQTT_UN_ANON              "AT+SMCONF=\"USERNAME\",\"\""
// #define MQTT_PWD                  "AT+SMCONF=\"PASSWORD\",Psusnec06"
#define MQTT_PWD                  "AT+SMCONF=\"PASSWORD\",wgzs7igsw56s"

#define MQTT_PWD_ANON             "AT+SMCONF=\"PASSWORD\",\"\""
// #define MQTT_ID                   "AT+SMCONF=\"CLIENTID\",aimnet002S"
// #define MQTT_ID                   "AT+SMCONF=\"CLIENTID\",aimnet002S25"
#define MQTT_ID                   "AT+SMCONF=\"CLIENTID\",ZifanTestStability1"
#define MQTT_ID_App_Encry         "AT+SMCONF=\"CLIENTID\",ZifanAppEncrypt"


#define SSL_ROOT                  "-----BEGIN CERTIFICATE-----MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQELBQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwGA1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAUBgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hvby5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UEBhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTESMBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVpdHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPgUZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FWTe3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEAs06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxoE6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNTMFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wKsMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJEm/XriWr/Cq4h/JfB7NTsezVslgkBaoU=-----END CERTIFICATE-----"
#define SSL_ROOT_SIZE             1452
#define SSL_CERT_SIZE             1500
#define SSL_KEY_SIZE              1500
