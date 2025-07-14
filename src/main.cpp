#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "config.h"
#include "wifi_scanner.h"
#include "web_server.h"
#include "firebase.h"
#include "led_control.h"
#include "serial_menu.h"
#include "status_tracker.h"

// Global variables
Config config;
WiFiNetwork networks[30];
int networkCount = 0;
ScanData dataBuffer[20];
int dataCount = 0;
ESP8266WebServer server(80);
bool configMode = false;
bool timeSync = false;
unsigned long lastLedBlink = 0;
int ledState = LOW;
int ledStep = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  
  if (!LittleFS.begin()) {
    Serial.println("Falha ao montar sistema de arquivos. Formatando...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("Falha ao montar sistema de arquivos mesmo após formatação");
    }
  }
  
  loadConfig();

  pinMode(0, INPUT_PULLUP);
  delay(100);
  bool forceConfig = digitalRead(0) == LOW;
  bool nearBase = false;
  
  Serial.println("\n=== WIFI RANGE SCANNER ===");
  Serial.printf("Bicicleta: %s\n", config.bikeId);
  Serial.printf("Botão FLASH: %s\n", forceConfig ? "PRESSIONADO" : "LIVRE");
  
  if (forceConfig) {
    Serial.println("Botão FLASH detectado - Modo configuração");
  } else if (strlen(config.baseSSID1) > 0) {
    Serial.println("Verificando bases WiFi...");
    scanWiFiNetworks();
    nearBase = checkAtBase();
    if (nearBase) {
      Serial.println("Base WiFi detectada - Modo configuração");
    }
  }
  
  if (forceConfig || nearBase) {
    startConfigMode();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    Serial.println("Iniciando scanner WiFi...");
    Serial.println("=========================");
  }
}

void loop() {
  updateLED();
  
  if (configMode) {
    server.handleClient();
    return;
  }

  if (Serial.available()) {
    char cmd = Serial.peek();
    if (cmd == 'm' || cmd == 'M') {
      Serial.read();
      Serial.println("\n=== MENU ATIVADO ===");
      showMenu();
      
      while (true) {
        handleSerialMenu();
        if (Serial.available() && (Serial.peek() == 'q' || Serial.peek() == 'Q')) {
          Serial.read();
          Serial.println("\nVoltando ao modo normal...");
          break;
        }
        delay(100);
      }
    } else {
      Serial.read();
    }
  }

  scanWiFiNetworks();
  storeData();

  config.isAtBase = checkAtBase();

  if (config.isAtBase && dataCount > 0) {
    if (connectToBase()) {
      syncTime();
      uploadData();
    }
  }

  int scanDelay = config.isAtBase ? config.scanTimeInactive : config.scanTimeActive;
  float battery = getBatteryLevel();
  Serial.printf("=== Bike %s - %d redes - Bat: %.1f%% ===\n", config.bikeId, networkCount, battery);
  Serial.printf("Status: %s | Buffer: %d | Próximo: %ds | Digite 'm' para menu\n",
                config.isAtBase ? "BASE" : "MOVIMENTO", dataCount, scanDelay / 1000);
  delay(scanDelay);
}