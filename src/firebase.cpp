#include "firebase.h"
#include "config.h"
#include "wifi_scanner.h"
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Arduino.h>

void syncTime() {
  if (!timeSync) {
    timeClient.begin();
    if (timeClient.update()) {
      timeSync = true;
      Serial.println("Horário sincronizado");
    }
  }
}

void uploadData() {
  if (strlen(config.firebaseUrl) == 0 || dataCount == 0)
    return;

  Serial.println("=== UPLOAD FIREBASE ===");
  
  unsigned long timestamp = timeSync ? timeClient.getEpochTime() : millis();
  
  String payload = "{\"bike\":\"" + String(config.bikeId) + "\"";
  payload += ",\"timestamp\":" + String(timestamp);
  payload += ",\"battery\":" + String((int)getBatteryLevel());
  payload += ",\"networks\":[";
  
  int maxNets = min(networkCount, 5);
  for (int i = 0; i < maxNets; i++) {
    if (i > 0) payload += ",";
    payload += "{\"ssid\":\"" + String(networks[i].ssid) + "\"";
    payload += ",\"rssi\":" + String(networks[i].rssi);
    payload += ",\"channel\":" + String(networks[i].channel) + "}";
  }
  payload += "]}";
  
  Serial.printf("Payload: %s\n", payload.c_str());
  
  WiFiClientSecure client;
  client.setInsecure();
  
  String url = String(config.firebaseUrl);
  url.replace("https://", "");
  url.replace("http://", "");
  int slashIndex = url.indexOf('/');
  String host = url.substring(0, slashIndex);
  
  String path = "/bikes/" + String(config.bikeId) + "/scans/" + String(timestamp) + ".json";
  
  Serial.printf("Host: %s\n", host.c_str());
  Serial.printf("Path: %s\n", path.c_str());
  
  if (client.connect(host.c_str(), 443)) {
    Serial.println("Conectado ao Firebase!");
    
    client.print("PUT " + path + " HTTP/1.1\r\n");
    client.print("Host: " + host + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(payload.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);
    
    delay(1000);
    String response = "";
    while (client.available()) {
      response += client.readString();
    }
    
    Serial.println("Resposta Firebase:");
    Serial.println(response);
    
    if (response.indexOf("200 OK") > 0) {
      Serial.println("Upload OK! Limpando arquivos locais...");
      
      Dir dir = LittleFS.openDir("/");
      while (dir.next()) {
        if (dir.fileName().startsWith("scan_")) {
          LittleFS.remove(dir.fileName());
          Serial.printf("Removido: %s\n", dir.fileName().c_str());
        }
      }
      dataCount = 0;
    } else {
      Serial.println("Erro no upload - mantendo arquivos locais");
    }
    
    client.stop();
  } else {
    Serial.println("Falha ao conectar no Firebase");
  }
  
  WiFi.disconnect();
}