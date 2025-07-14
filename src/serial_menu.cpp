#include "serial_menu.h"
#include "config.h"
#include "wifi_scanner.h"
#include "firebase.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

void showMenu() {
  Serial.println("\n=== MENU ===");
  Serial.println("1) Monitorar redes");
  Serial.println("2) Verificar conexao com base");
  Serial.println("3) Testar conexao Firebase");
  Serial.println("4) Mostrar configuracoes");
  Serial.println("5) Ver dados salvos");
  Serial.println("6) Transferir dados (copy/paste)");
  Serial.println("7) Ativar modo AP/Configuracao");
  Serial.println("q) Sair do menu");
  Serial.print("Escolha: ");
}

void handleSerialMenu() {
  if (Serial.available()) {
    char choice = Serial.read();
    Serial.println(choice);
    
    switch(choice) {
      case '1':
        Serial.println("\n=== REDES DETECTADAS ===");
        scanWiFiNetworks();
        for (int i = 0; i < networkCount; i++) {
          Serial.printf("%s | %d dBm | Canal %d\n", networks[i].ssid, networks[i].rssi, networks[i].channel);
        }
        showMenu();
        break;
        
      case '2':
        Serial.println("\n=== TESTE CONEXAO BASE ===");
        scanWiFiNetworks();
        if (checkAtBase()) {
          Serial.println("Base WiFi detectada!");
          if (connectToBase()) {
            Serial.println("Conectado com sucesso!");
            Serial.println("IP: " + WiFi.localIP().toString());
            WiFi.disconnect();
          } else {
            Serial.println("Falha na conexao");
          }
        } else {
          Serial.println("Nenhuma base WiFi detectada");
        }
        showMenu();
        break;
        
      case '3':
        Serial.println("\n=== TESTE FIREBASE ===");
        if (strlen(config.firebaseUrl) == 0) {
          Serial.println("Firebase nao configurado");
        } else {
          Serial.printf("URL: %s\n", config.firebaseUrl);
          Serial.printf("Key: %s...\n", String(config.firebaseKey).substring(0, 10).c_str());
          
          if (connectToBase()) {
            Serial.println("Conectado! Testando upload...");
            dataCount = 1;
            uploadData();
          } else {
            Serial.println("Falha ao conectar na base para teste");
          }
        }
        showMenu();
        break;
        
      case '4':
        Serial.println("\n=== CONFIGURACOES ===");
        Serial.printf("Bike ID: %s\n", config.bikeId);
        Serial.printf("Scan Ativo: %d ms\n", config.scanTimeActive);
        Serial.printf("Scan Inativo: %d ms\n", config.scanTimeInactive);
        Serial.printf("Base 1: '%s' / '%s'\n", config.baseSSID1, config.basePassword1);
        Serial.printf("Base 2: '%s' / '%s'\n", config.baseSSID2, config.basePassword2);
        Serial.printf("Base 3: '%s' / '%s'\n", config.baseSSID3, config.basePassword3);
        Serial.printf("Firebase URL: %s\n", config.firebaseUrl);
        Serial.printf("Firebase Key: %s...\n", String(config.firebaseKey).substring(0, 15).c_str());
        showMenu();
        break;
        
      case '5':
        Serial.println("\n=== DADOS SALVOS ===");
        {
          Dir dir = LittleFS.openDir("/");
          int count = 0;
          while (dir.next()) {
            if (dir.fileName().startsWith("scan_")) {
              count++;
              Serial.printf("Arquivo: %s (%d bytes)\n", dir.fileName().c_str(), dir.fileSize());
              File file = LittleFS.open(dir.fileName().c_str(), "r");
              if (file) {
                String content = file.readString();
                file.close();
                Serial.printf("Conteúdo: %s\n", content.c_str());
              }
              Serial.println("---");
              if (count >= 5) {
                Serial.println("(mostrando apenas os primeiros 5 arquivos)");
                break;
              }
            }
          }
          if (count == 0) {
            Serial.println("Nenhum arquivo de dados encontrado");
          }
        }
        showMenu();
        break;
        
      case '6':
        Serial.println("\n=== TRANSFERIR DADOS ===");
        Serial.println("Copie os dados abaixo e cole em um arquivo:");
        Serial.println("--- INICIO DOS DADOS ---");
        {
          Dir dir = LittleFS.openDir("/");
          Serial.println("{");
          Serial.println("\"bikeId\":\"" + String(config.bikeId) + "\",");
          Serial.println("\"exportTime\":" + String(millis()) + ",");
          Serial.println("\"scans\":[");
          
          bool first = true;
          while (dir.next()) {
            if (dir.fileName().startsWith("scan_")) {
              if (!first) Serial.println(",");
              File file = LittleFS.open(dir.fileName().c_str(), "r");
              if (file) {
                String content = file.readString();
                file.close();
                Serial.print(content);
              }
              first = false;
            }
          }
          Serial.println("");
          Serial.println("]}");
        }
        Serial.println("--- FIM DOS DADOS ---");
        Serial.println("Dados prontos para backup!");
        showMenu();
        break;
        
      case '7':
        Serial.println("\n=== ATIVANDO MODO AP ===");
        Serial.println("Reiniciando em modo configuracao...");
        delay(1000);
        ESP.restart();
        break;
        
      case 'q':
      case 'Q':
        Serial.println("Saindo do menu...");
        break;
        
      default:
        showMenu();
        break;
    }
  }
}