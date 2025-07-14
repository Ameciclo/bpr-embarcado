#include "wifi_scanner.h"
#include "status_tracker.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Arduino.h>

String getBasePassword(String ssid) {
  if (ssid == String(config.baseSSID1))
    return String(config.basePassword1);
  if (ssid == String(config.baseSSID2))
    return String(config.basePassword2);
  if (ssid == String(config.baseSSID3))
    return String(config.basePassword3);
  return "";
}

void scanWiFiNetworks() {
  int n = WiFi.scanNetworks();
  networkCount = 0;

  for (int i = 0; i < n && networkCount < 30; i++) {
    WiFi.SSID(i).toCharArray(networks[networkCount].ssid, 32);
    WiFi.BSSIDstr(i).toCharArray(networks[networkCount].bssid, 18);
    networks[networkCount].rssi = WiFi.RSSI(i);
    networks[networkCount].channel = WiFi.channel(i);
    networks[networkCount].encryption = WiFi.encryptionType(i);
    networkCount++;
  }
}

bool checkAtBase() {
  for (int i = 0; i < networkCount; i++) {
    if (strlen(config.baseSSID1) > 0 && strcmp(networks[i].ssid, config.baseSSID1) == 0) {
      Serial.printf("Base1 encontrada: %s (RSSI: %d)\n", networks[i].ssid, networks[i].rssi);
      if (networks[i].rssi > -80) return true;
    }
    if (strlen(config.baseSSID2) > 0 && strcmp(networks[i].ssid, config.baseSSID2) == 0) {
      Serial.printf("Base2 encontrada: %s (RSSI: %d)\n", networks[i].ssid, networks[i].rssi);
      if (networks[i].rssi > -80) return true;
    }
    if (strlen(config.baseSSID3) > 0 && strcmp(networks[i].ssid, config.baseSSID3) == 0) {
      Serial.printf("Base3 encontrada: %s (RSSI: %d)\n", networks[i].ssid, networks[i].rssi);
      if (networks[i].rssi > -80) return true;
    }
  }
  return false;
}

bool connectToBase() {
  for (int i = 0; i < networkCount; i++) {
    String basePass = getBasePassword(String(networks[i].ssid));
    if (basePass != "") {
      Serial.printf("Conectando à base: %s\n", networks[i].ssid);
      WiFi.begin(networks[i].ssid, basePass.c_str());

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Conectado à base %s!\n", networks[i].ssid);
        Serial.printf("IP obtido: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        trackConnection(networks[i].ssid, WiFi.localIP().toString().c_str(), true);
        return true;
      } else {
        Serial.printf("Falha ao conectar em %s\n", networks[i].ssid);
      }
    }
  }
  return false;
}

void storeData() {
  String filename = "/scan_" + String(millis()) + ".json";
  
  // Formato sem bateria: [timestamp,realTime,[[ssid,bssid,rssi,channel]]]
  String data = "[" + String(millis());
  data += ",0"; // realTime (será calculado no upload)
  data += ",[";
  
  int maxNets = min(networkCount, 5);
  for (int i = 0; i < maxNets; i++) {
    if (i > 0) data += ",";
    data += "[\"" + String(networks[i].ssid) + "\",\"";
    data += String(networks[i].bssid) + "\",";
    data += String(networks[i].rssi) + ",";
    data += String(networks[i].channel) + "]";
  }
  data += "]]";
  
  File file = LittleFS.open(filename.c_str(), "w");
  if (file) {
    file.print(data);
    file.close();
    dataCount++;
  }
  
  // Track battery separately
  trackBattery(getBatteryLevel());
}

float getBatteryLevel() {
  int adcValue = analogRead(A0);
  float voltage = (adcValue / 1024.0) * 4.2;
  float percentage = ((voltage - 3.0) / 1.2) * 100.0;
  return max(0.0f, min(100.0f, percentage));
}