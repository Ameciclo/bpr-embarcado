#include "led_control.h"
#include "config.h"
#include <Arduino.h>

void updateLED() {
  unsigned long now = millis();
  
  if (configMode) {
    // Modo AP: 3 piscadas rápidas + pausa longa
    unsigned long intervals[] = {100, 100, 100, 100, 100, 100, 1000}; // ON-OFF-ON-OFF-ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep % 2 == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 7;
    }
  } else if (config.isAtBase) {
    // Conectado na base: 1 piscada lenta + pausa longa
    unsigned long intervals[] = {500, 500, 1500}; // ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 3;
    }
  } else {
    // Coletando dados: 2 piscadas rápidas + pausa média
    unsigned long intervals[] = {200, 200, 200, 200, 800}; // ON-OFF-ON-OFF-PAUSA
    if (now - lastLedBlink > intervals[ledStep]) {
      ledState = (ledStep % 2 == 0) ? 1 : 0;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
      lastLedBlink = now;
      ledStep = (ledStep + 1) % 5;
    }
  }
}