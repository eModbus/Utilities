// Modbus bridge device V3
// Copyright 2020 by miq1@gmx.de

#include <Arduino.h>
#include "Blinker.h"
#include "Buttoner.h"

#define LED_R GPIO_NUM_4
#define LED_B GPIO_NUM_25
#define BUTTON_A GPIO_NUM_27
#define BUTTON_B GPIO_NUM_26

Blinker rot(LED_R);
Blinker gruen(LED_B);

Buttoner ButtonA(BUTTON_A, HIGH, false, 2);
Buttoner ButtonB(BUTTON_B);

const uint16_t BL_QUICKLY(0x2);
const uint16_t BL_SLOW(0x3000);
const uint16_t BL_FLICK(0xFFFC);

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  delay(5000);
  Serial.println("_OK_\n");

  rot.start(BL_FLICK);
  gruen.start(BL_SLOW);
}

void loop() {
  rot.update();
  gruen.update();

  if (ButtonA.update() > 0) {
    switch (ButtonA.getEvent()) {
      case BE_CLICK:
        Serial.printf("A clicked.(%d)\n", ButtonA.qSize());
        break;
      case BE_DOUBLECLICK:
        Serial.printf("A doubly clicked.(%d)\n", ButtonA.qSize());
        break;
      case BE_PRESS:
        Serial.printf("A held down.(%d)\n", ButtonA.qSize());
        break;
      default:
        Serial.printf("Huh? A?(%d)\n", ButtonA.qSize());
        break;
    }
  }
  
  if (ButtonB.update() > 0) {
    switch (ButtonB.getEvent()) {
      case BE_CLICK:
        Serial.printf("B clicked.(%d)\n", ButtonB.qSize());
        break;
      case BE_DOUBLECLICK:
        Serial.printf("B doubly clicked.(%d)\n", ButtonB.qSize());
        break;
      case BE_PRESS:
        Serial.printf("B held down.(%d)\n", ButtonB.qSize());
        break;
      default:
        Serial.printf("Huh? B?(%d)\n", ButtonB.qSize());
        break;
    }
  }
}
