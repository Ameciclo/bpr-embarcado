#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void startConfigMode();
void handleRoot();
void handleConfig();
void handleSave();
void handleWifi();
void handleDados();

#endif