#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <LittleFS.h>
#include <EEPROM.h>


struct WiFiNetwork
{
  char ssid[32];
  char bssid[18];
  int rssi;
  int channel;
  int encryption;
};

struct Config
{
  int scanTimeActive = 5000;
  int scanTimeInactive = 30000;
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

struct ScanData
{
  unsigned long timestamp;
  WiFiNetwork networks[10];
  int networkCount;
  float batteryLevel;
  bool isCharging;
};

WiFiNetwork networks[30];
int networkCount = 0;
Config config;
ESP8266WebServer server(80);
bool configMode = false;

ScanData dataBuffer[20];
int dataCount = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);
bool timeSync = false;
unsigned long lastLedBlink = 0;
int ledState = LOW;
int ledPattern = 0;
int ledStep = 0;

void scanWiFiNetworks();
void updateLED();
void printStoredNetworks();
void loadConfig();
void saveConfig();
void startConfigMode();
void handleRoot();
void handleConfig();
void handleSave();
void handleDados();
String readFile(const char* path);
void writeFile(const char* path, const String& content);

void handleWifi();
bool checkAtBase();
void storeData();
void uploadData();
bool connectToBase();
void syncTime();
String getBasePassword(String ssid);
float getBatteryLevel();
void showMenu();
void handleSerialMenu();

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED off (invertido no ESP8266)
  delay(2000); // Tempo para pressionar botão FLASH
  
  if (!LittleFS.begin()) {
    Serial.println("Falha ao montar sistema de arquivos. Formatando...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("Falha ao montar sistema de arquivos mesmo após formatação");
    }
  }
  
  loadConfig();

  // Verificar se deve entrar em modo configuração
  pinMode(0, INPUT_PULLUP); // Configurar botão FLASH
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
  
  if (forceConfig || nearBase)
  {
    startConfigMode();
  }
  else
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    Serial.println("Iniciando scanner WiFi...");
    Serial.println("=========================");
  }
}

void loop()
{
  updateLED(); // Atualizar LED sempre
  
  if (configMode)
  {
    server.handleClient();
    return;
  }

  // Verificar se há comando no serial
  if (Serial.available()) {
    char cmd = Serial.peek();
    if (cmd == 'm' || cmd == 'M') {
      Serial.read(); // consumir o caractere
      Serial.println("\n=== MENU ATIVADO ===");
      showMenu();
      
      // Loop do menu
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
      Serial.read(); // limpar buffer
    }
  }

  scanWiFiNetworks();
  storeData();

  config.isAtBase = checkAtBase();

  if (config.isAtBase && dataCount > 0)
  {
    if (connectToBase())
    {
      syncTime();
      uploadData();
    }
  }

  int scanDelay = config.isAtBase ? config.scanTimeInactive : config.scanTimeActive;
  printStoredNetworks();

  Serial.printf("Status: %s | Buffer: %d | Próximo: %ds | Digite 'm' para menu\n",
                config.isAtBase ? "BASE" : "MOVIMENTO", dataCount, scanDelay / 1000);
  delay(scanDelay);
}

void startConfigMode()
{
  configMode = true;
  
  // Se botão FLASH pressionado, cria AP próprio
  if (digitalRead(0) == LOW) {
    WiFi.mode(WIFI_AP);
    String apName = "Bike-" + String(config.bikeId);
    WiFi.softAP(apName.c_str(), "12345678");
    Serial.println("=== MODO CONFIGURAÇÃO ATIVO ===");
    Serial.println("WiFi: " + apName);
    Serial.println("Senha: 12345678");
    Serial.println("Acesse: http://192.168.4.1");
    Serial.println("================================");
  } else {
    // Conecta à base WiFi para configuração
    if (connectToBase()) {
      Serial.println("=== MODO CONFIGURAÇÃO ATIVO ===");
      Serial.println("IP: " + WiFi.localIP().toString());
      Serial.println("Acesse: http://" + WiFi.localIP().toString());
      Serial.println("================================");
    }
  }

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/wifi", handleWifi);
  server.on("/dados", handleDados);
  server.begin();
}

void handleRoot()
{
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:18px}.btn{display:block;padding:20px;margin:15px 0;background:#007cba;color:white;text-decoration:none;text-align:center;border-radius:5px;font-size:18px}</style></head>";
  html += "<body><h1>Bike " + String(config.bikeId) + " - Controle</h1>";
  html += "<a href='/config' class='btn'>1) Configurações</a>";
  html += "<a href='/wifi' class='btn'>2) Ver WiFi Detectados</a>";
  html += "<a href='/dados' class='btn'>3) Ver Dados Gravados</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig()
{
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:18px}input,button{padding:15px;font-size:16px;margin:10px 0;width:100%;box-sizing:border-box}h3{margin-top:30px}</style></head>";
  html += "<body><h1>Configurações - Bike " + String(config.bikeId) + "</h1>";
  html += "<a href='/' style='display:block;padding:10px;background:#666;color:white;text-decoration:none;text-align:center;margin:10px 0'>Voltar</a>";
  html += "<form method='post' action='/save'>";
  html += "ID da Bicicleta: <input name='bike' value='" + String(config.bikeId) + "'><br>";
  html += "Tempo Scan Ativo (ms): <input name='active' value='" + String(config.scanTimeActive) + "'><br>";
  html += "Tempo Scan Inativo (ms): <input name='inactive' value='" + String(config.scanTimeInactive) + "'><br>";
  html += "<h3>Base 1:</h3>SSID: <input name='ssid1' value='" + String(config.baseSSID1) + "'><br>";
  html += "Senha: <input name='pass1' value='" + String(config.basePassword1) + "'><br>";
  html += "<h3>Base 2:</h3>SSID: <input name='ssid2' value='" + String(config.baseSSID2) + "'><br>";
  html += "Senha: <input name='pass2' value='" + String(config.basePassword2) + "'><br>";
  html += "<h3>Base 3:</h3>SSID: <input name='ssid3' value='" + String(config.baseSSID3) + "'><br>";
  html += "Senha: <input name='pass3' value='" + String(config.basePassword3) + "'><br>";
  html += "<button type='submit'>Salvar</button></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave()
{
  server.arg("bike").toCharArray(config.bikeId, 10);
  config.scanTimeActive = server.arg("active").toInt();
  config.scanTimeInactive = server.arg("inactive").toInt();
  server.arg("ssid1").toCharArray(config.baseSSID1, 32);
  server.arg("pass1").toCharArray(config.basePassword1, 32);
  server.arg("ssid2").toCharArray(config.baseSSID2, 32);
  server.arg("pass2").toCharArray(config.basePassword2, 32);
  server.arg("ssid3").toCharArray(config.baseSSID3, 32);
  server.arg("pass3").toCharArray(config.basePassword3, 32);

  saveConfig();
  server.send(200, "text/html", "<html><body><h1>Salvo! Reiniciando...</h1></body></html>");
  delay(2000);
  ESP.restart();
}

void handleWifi()
{
  scanWiFiNetworks();
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:16px}table{width:100%;border-collapse:collapse}th,td{border:1px solid #ddd;padding:12px;text-align:left}th{background:#f2f2f2}.strong{color:green}.weak{color:red}</style>";
  html += "<script>setTimeout(function(){location.reload()},5000)</script></head>";
  html += "<body><h1>WiFi Detectados (Bike " + String(config.bikeId) + ")</h1>";
  html += "<p>Atualizando a cada 5 segundos...</p>";
  html += "<table><tr><th>SSID</th><th>RSSI</th><th>Canal</th></tr>";
  for (int i = 0; i < networkCount; i++)
  {
    String cssClass = networks[i].rssi > -60 ? "strong" : (networks[i].rssi < -80 ? "weak" : "");
    html += "<tr class='" + cssClass + "'><td>" + String(networks[i].ssid) + "</td><td>" + String(networks[i].rssi) + " dBm</td><td>" + String(networks[i].channel) + "</td></tr>";
  }
  html += "</table><br><a href='/'>Voltar</a></body></html>";
  server.send(200, "text/html", html);
}

void handleDados()
{
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:16px}.arquivo{border:1px solid #ddd;padding:10px;margin:10px 0;background:#f9f9f9}</style></head>";
  html += "<body><h1>Dados Gravados - Bike " + String(config.bikeId) + "</h1>";
  html += "<a href='/' style='display:block;padding:10px;background:#666;color:white;text-decoration:none;text-align:center;margin:10px 0;width:100px'>Voltar</a>";
  
  Dir dir = LittleFS.openDir("/");
  int count = 0;
  while (dir.next()) {
    if (dir.fileName().startsWith("scan_")) {
      count++;
      html += "<div class='arquivo'>";
      html += "<strong>" + dir.fileName() + "</strong> (" + String(dir.fileSize()) + " bytes)<br>";
      String content = readFile(dir.fileName().c_str());
      html += "<code>" + content + "</code>";
      html += "</div>";
      if (count >= 10) {
        html += "<p><em>Mostrando apenas os primeiros 10 arquivos...</em></p>";
        break;
      }
    }
  }
  
  if (count == 0) {
    html += "<p>Nenhum arquivo de dados encontrado.</p>";
  }
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

bool checkAtBase()
{
  for (int i = 0; i < networkCount; i++)
  {
    // Debug: mostrar comparações
    if (strlen(config.baseSSID1) > 0 && strcmp(networks[i].ssid, config.baseSSID1) == 0) {
      Serial.printf("Base1 encontrada: %s (RSSI: %d)\n", networks[i].ssid, networks[i].rssi);
      if (networks[i].rssi > -80) return true; // Ajustado para -80
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

void loadConfig()
{
  // Configurações padrão (fallback) - SEM DADOS SENSÍVEIS
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
  
  // Carregar ID da bicicleta
  String bikeId = readFile("/bike.txt");
  if (bikeId.length() > 0) {
    bikeId.trim();
    bikeId.toCharArray(config.bikeId, 10);
    Serial.printf("Bike ID carregado: %s\n", config.bikeId);
  }
  
  // Carregar tempos de scan
  String timing = readFile("/timing.txt");
  if (timing.length() > 0) {
    int idx = timing.indexOf('\n');
    if (idx > 0) {
      config.scanTimeActive = timing.substring(0, idx).toInt();
      config.scanTimeInactive = timing.substring(idx+1).toInt();
      Serial.printf("Timing carregado: %d/%d ms\n", config.scanTimeActive, config.scanTimeInactive);
    }
  }
  
  // Carregar configurações das bases
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
  
  // Carregar configurações do Firebase
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

void saveConfig()
{
  if (!LittleFS.begin()) {
    Serial.println("Falha ao montar sistema de arquivos");
    return;
  }
  
  // Salvar ID da bicicleta
  writeFile("/bike.txt", String(config.bikeId));
  
  // Salvar tempos de scan
  String timing = String(config.scanTimeActive) + "\n" + String(config.scanTimeInactive);
  writeFile("/timing.txt", timing);
  
  // Salvar configurações das bases
  String bases = String(config.baseSSID1) + "\n" + 
                String(config.basePassword1) + "\n" + 
                String(config.baseSSID2) + "\n" + 
                String(config.basePassword2) + "\n" + 
                String(config.baseSSID3) + "\n" + 
                String(config.basePassword3);
  writeFile("/bases.txt", bases);
  
  // Salvar configurações do Firebase
  String firebase = String(config.firebaseUrl) + "\n" + String(config.firebaseKey);
  writeFile("/firebase.txt", firebase);
  
  Serial.println("Configurações salvas");
}

void scanWiFiNetworks()
{
  int n = WiFi.scanNetworks();
  networkCount = 0;

  for (int i = 0; i < n && networkCount < 30; i++)
  {
    WiFi.SSID(i).toCharArray(networks[networkCount].ssid, 32);
    WiFi.BSSIDstr(i).toCharArray(networks[networkCount].bssid, 18);
    networks[networkCount].rssi = WiFi.RSSI(i);
    networks[networkCount].channel = WiFi.channel(i);
    networks[networkCount].encryption = WiFi.encryptionType(i);
    networkCount++;
  }
}

void storeData()
{
  // Criar arquivo com timestamp
  String filename = "/scan_" + String(millis()) + ".json";
  
  // Formato compacto: [timestamp,realTime,battery,charging,[[ssid,bssid,rssi,channel]]]
  String data = "[" + String(millis());
  data += ",0"; // realTime (será calculado no upload)
  data += "," + String((int)getBatteryLevel());
  data += "," + String(analogRead(A0) > 900 ? "true" : "false");
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
  
  writeFile(filename.c_str(), data);
  dataCount++;
}

bool connectToBase()
{
  // Tentar conectar em qualquer uma das 3 bases
  for (int i = 0; i < networkCount; i++)
  {
    String basePass = getBasePassword(String(networks[i].ssid));
    if (basePass != "")
    {
      Serial.printf("Conectando à base: %s\n", networks[i].ssid);
      WiFi.begin(networks[i].ssid, basePass.c_str());

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20)
      {
        delay(500);
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.printf("Conectado à base %s!\n", networks[i].ssid);
        Serial.printf("IP obtido: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        return true;
      } else {
        Serial.printf("Falha ao conectar em %s\n", networks[i].ssid);
      }
    }
  }
  return false;
}

String getBasePassword(String ssid)
{
  if (ssid == String(config.baseSSID1))
    return String(config.basePassword1);
  if (ssid == String(config.baseSSID2))
    return String(config.basePassword2);
  if (ssid == String(config.baseSSID3))
    return String(config.basePassword3);
  return "";
}

void syncTime()
{
  if (!timeSync)
  {
    timeClient.begin();
    if (timeClient.update())
    {
      timeSync = true;
      Serial.println("Horário sincronizado");
    }
  }
}

void uploadData()
{
  if (strlen(config.firebaseUrl) == 0 || dataCount == 0)
    return;

  Serial.println("=== UPLOAD FIREBASE ===");
  
  // Usar timestamp único para cada upload
  unsigned long timestamp = timeSync ? timeClient.getEpochTime() : millis();
  
  // Payload com dados reais dos scans
  String payload = "{\"bike\":\"" + String(config.bikeId) + "\"";
  payload += ",\"timestamp\":" + String(timestamp);
  payload += ",\"battery\":" + String((int)getBatteryLevel());
  payload += ",\"networks\":[";
  
  // Adicionar até 5 redes mais fortes
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
  
  // Usar timestamp como chave única
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
      
      // Limpar arquivos locais após upload bem-sucedido
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

float getBatteryLevel()
{
  // Lê o valor do ADC (A0) - assumindo divisor de tensão para bateria
  int adcValue = analogRead(A0);
  // Converte para voltagem (ESP8266 ADC: 0-1024 = 0-1V, com divisor pode ser 0-4.2V)
  float voltage = (adcValue / 1024.0) * 4.2;
  // Converte para porcentagem (3.0V = 0%, 4.2V = 100%)
  float percentage = ((voltage - 3.0) / 1.2) * 100.0;
  return max(0.0f, min(100.0f, percentage));
}

void updateLED()
{
  unsigned long now = millis();
  
  if (configMode) {
    // Modo AP: 3 piscadas rápidas + pausa longa
    int intervals[] = {100, 100, 100, 100, 100, 100, 1000}; // ON-OFF-ON-OFF-ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep % 2 == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 7;
    }
  } else if (config.isAtBase) {
    // Conectado na base: 1 piscada lenta + pausa longa
    int intervals[] = {500, 500, 1500}; // ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 3;
    }
  } else {
    // Coletando dados: 2 piscadas rápidas + pausa média
    int intervals[] = {200, 200, 200, 200, 800}; // ON-OFF-ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep % 2 == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 5;
    }
  }
}

void printStoredNetworks()
{
  float battery = getBatteryLevel();
  Serial.printf("=== Bike %s - %d redes - Bat: %.1f%% ===\n", config.bikeId, networkCount, battery);
}

void showMenu()
{
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

void handleSerialMenu()
{
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
            dataCount = 1; // Simular dados
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
              String content = readFile(dir.fileName().c_str());
              Serial.printf("Conteúdo: %s\n", content.c_str());
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
              String content = readFile(dir.fileName().c_str());
              Serial.print(content);
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