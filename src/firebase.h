#ifndef FIREBASE_H
#define FIREBASE_H

#include <NTPClient.h>
#include <WiFiUdp.h>

extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

void syncTime();
void uploadData();

#endif