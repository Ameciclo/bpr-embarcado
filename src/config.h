#ifndef CONFIG_H
#define CONFIG_H

struct WiFiNetwork {
  char ssid[32];
  char bssid[18];
  int rssi;
  int channel;
  int encryption;
};

struct Config {
  int scanTimeActive = 1000;
  int scanTimeInactive = 1000;
  char baseSSID1[32] = "";
  char basePassword1[32] = "";
  char baseSSID2[32] = "";
  char basePassword2[32] = "";
  char baseSSID3[32] = "";
  char basePassword3[32] = "";
  char bikeId[10] = "sl01";
  bool isAtBase = false;
  char firebaseUrl[128] = "";
  char firebaseKey[64] = "";
};

struct ScanData {
  unsigned long timestamp;
  WiFiNetwork networks[10];
  int networkCount;
  float batteryLevel;
  bool isCharging;
};

extern Config config;
extern WiFiNetwork networks[30];
extern int networkCount;
extern ScanData dataBuffer[20];
extern int dataCount;
extern bool configMode;
extern bool timeSync;

void loadConfig();
void saveConfig();

#endif