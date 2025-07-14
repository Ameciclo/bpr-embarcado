#include "config.h"
#include <LittleFS.h>
#include <Arduino.h>

String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("Falha ao abrir arquivo: %s\n", path);
    return "";
  }
  String content = file.readString();
  file.close();
  return content;
}

void writeFile(const char* path, const String& content) {
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.printf("Falha ao criar arquivo: %s\n", path);
    return;
  }
  file.print(content);
  file.close();
}

void loadConfig() {
  strcpy(config.bikeId, "sl01");
  config.scanTimeActive = 5000;
  config.scanTimeInactive = 30000;
  strcpy(config.baseSSID1, "");
  strcpy(config.basePassword1, "");
  strcpy(config.baseSSID2, "");
  strcpy(config.basePassword2, "");
  strcpy(config.baseSSID3, "");
  strcpy(config.basePassword3, "");
  strcpy(config.firebaseUrl, "");
  strcpy(config.firebaseKey, "");
  
  if (!LittleFS.begin()) {
    Serial.println("Sistema de arquivos não disponível - usando configuração padrão");
    return;
  }
  
  Serial.println("Carregando configurações dos arquivos...");
  
  String bikeId = readFile("/bike.txt");
  if (bikeId.length() > 0) {
    bikeId.trim();
    bikeId.toCharArray(config.bikeId, 10);
    Serial.printf("Bike ID carregado: %s\n", config.bikeId);
  }
  
  String timing = readFile("/timing.txt");
  if (timing.length() > 0) {
    int idx = timing.indexOf('\n');
    if (idx > 0) {
      config.scanTimeActive = timing.substring(0, idx).toInt();
      config.scanTimeInactive = timing.substring(idx+1).toInt();
      Serial.printf("Timing carregado: %d/%d ms\n", config.scanTimeActive, config.scanTimeInactive);
    }
  }
  
  String bases = readFile("/bases.txt");
  if (bases.length() > 0) {
    Serial.println("Carregando bases WiFi do arquivo...");
    int pos = 0;
    int idx = bases.indexOf('\n');
    if (idx > 0) {
      bases.substring(pos, idx).toCharArray(config.baseSSID1, 32);
      pos = idx + 1;
      idx = bases.indexOf('\n', pos);
      if (idx > 0) {
        bases.substring(pos, idx).toCharArray(config.basePassword1, 32);
        pos = idx + 1;
        idx = bases.indexOf('\n', pos);
        if (idx > 0) {
          bases.substring(pos, idx).toCharArray(config.baseSSID2, 32);
          pos = idx + 1;
          idx = bases.indexOf('\n', pos);
          if (idx > 0) {
            bases.substring(pos, idx).toCharArray(config.basePassword2, 32);
            pos = idx + 1;
            idx = bases.indexOf('\n', pos);
            if (idx > 0) {
              bases.substring(pos, idx).toCharArray(config.baseSSID3, 32);
              pos = idx + 1;
              bases.substring(pos).toCharArray(config.basePassword3, 32);
            }
          }
        }
      }
    }
  }
  
  Serial.printf("Base 1: %s\n", config.baseSSID1);
  Serial.printf("Base 2: %s\n", config.baseSSID2);
  Serial.printf("Base 3: %s\n", config.baseSSID3);
  
  String firebase = readFile("/firebase.txt");
  if (firebase.length() > 0) {
    int idx = firebase.indexOf('\n');
    if (idx > 0) {
      firebase.substring(0, idx).toCharArray(config.firebaseUrl, 128);
      firebase.substring(idx+1).toCharArray(config.firebaseKey, 64);
      Serial.println("Firebase carregado do arquivo");
    }
  }
  
  Serial.printf("Firebase: %s\n", strlen(config.firebaseUrl) > 0 ? "Configurado" : "Não configurado");
  Serial.println("Configurações carregadas");
}

void saveConfig() {
  if (!LittleFS.begin()) {
    Serial.println("Falha ao montar sistema de arquivos");
    return;
  }
  
  writeFile("/bike.txt", String(config.bikeId));
  
  String timing = String(config.scanTimeActive) + "\n" + String(config.scanTimeInactive);
  writeFile("/timing.txt", timing);
  
  String bases = String(config.baseSSID1) + "\n" + 
                String(config.basePassword1) + "\n" + 
                String(config.baseSSID2) + "\n" + 
                String(config.basePassword2) + "\n" + 
                String(config.baseSSID3) + "\n" + 
                String(config.basePassword3);
  writeFile("/bases.txt", bases);
  
  String firebase = String(config.firebaseUrl) + "\n" + String(config.firebaseKey);
  writeFile("/firebase.txt", firebase);
  
  Serial.println("Configurações salvas");
}