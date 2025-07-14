#include "web_server.h"
#include "config.h"
#include "wifi_scanner.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Arduino.h>

void startConfigMode() {
  configMode = true;
  
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

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:18px}.btn{display:block;padding:20px;margin:15px 0;background:#007cba;color:white;text-decoration:none;text-align:center;border-radius:5px;font-size:18px}</style></head>";
  html += "<body><h1>Bike " + String(config.bikeId) + " - Controle</h1>";
  html += "<a href='/config' class='btn'>1) Configurações</a>";
  html += "<a href='/wifi' class='btn'>2) Ver WiFi Detectados</a>";
  html += "<a href='/dados' class='btn'>3) Ver Dados Gravados</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
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

void handleSave() {
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

void handleWifi() {
  scanWiFiNetworks();
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;font-size:16px}table{width:100%;border-collapse:collapse}th,td{border:1px solid #ddd;padding:12px;text-align:left}th{background:#f2f2f2}.strong{color:green}.weak{color:red}</style>";
  html += "<script>setTimeout(function(){location.reload()},5000)</script></head>";
  html += "<body><h1>WiFi Detectados (Bike " + String(config.bikeId) + ")</h1>";
  html += "<p>Atualizando a cada 5 segundos...</p>";
  html += "<table><tr><th>SSID</th><th>RSSI</th><th>Canal</th></tr>";
  for (int i = 0; i < networkCount; i++) {
    String cssClass = networks[i].rssi > -60 ? "strong" : (networks[i].rssi < -80 ? "weak" : "");
    html += "<tr class='" + cssClass + "'><td>" + String(networks[i].ssid) + "</td><td>" + String(networks[i].rssi) + " dBm</td><td>" + String(networks[i].channel) + "</td></tr>";
  }
  html += "</table><br><a href='/'>Voltar</a></body></html>";
  server.send(200, "text/html", html);
}

void handleDados() {
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
      File file = LittleFS.open(dir.fileName().c_str(), "r");
      if (file) {
        String content = file.readString();
        file.close();
        html += "<code>" + content + "</code>";
      }
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