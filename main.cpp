// Modbus bridge device V3
// Copyright 2020 by miq1@gmx.de

#include <Arduino.h>
#include "Blinker.h"
#include "Buttoner.h"
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "TelnetLog.h"
#include "Logging.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

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

SSD1306AsciiWire oled;

TelnetLog tl(23, 4);

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  delay(5000);
  Serial.println("_OK_\n");

  WiFi.begin("FritzGandhi", "MahatmaNetzwerk");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  rot.start(BL_FLICK);
  gruen.start(BL_SLOW);

  Wire.begin();
  Wire.setClock(400000L);

  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(System5x7);
  oled.setScrollMode(SCROLL_MODE_AUTO);
  oled.clear();

  oled.printf("012345678901234567890123456789");
  delay(3000);
  oled.clear();
  oled.setFont(fixed_bold10x15);
  oled.printf("012345678901234567890123456789");
  delay(3000);
  oled.clear();
  oled.setFont(Callibri10);
  oled.printf("012345678901234567890123456789");
  delay(3000);
  oled.clear();
  oled.setFont(System5x7);
  tl.begin("Bridge-Test");
  LOGDEVICE = &tl;
  MBUlogLvl = LOG_LEVEL_VERBOSE;
}

void loop() {
  static uint32_t dTimer = millis();
  static bool dOn = false;
  char buffer[64];
  rot.update();
  gruen.update();
  tl.update();

  if (ButtonA.update() > 0) {
    if (!dOn) oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
    dOn = true;
    dTimer = millis();
    switch (ButtonA.getEvent()) {
      case BE_CLICK:
        snprintf(buffer, 64, "A clicked.(%d)", ButtonA.qSize());
        break;
      case BE_DOUBLECLICK:
        snprintf(buffer, 64, "A doubly clicked.(%d)", ButtonA.qSize());
        break;
      case BE_PRESS:
        snprintf(buffer, 64, "A held down.(%d)", ButtonA.qSize());
        oled.setContrast(0);
        break;
      default:
        snprintf(buffer, 64, "Huh? A?(%d)", ButtonA.qSize());
        break;
    }
    oled.println();
    oled.print(buffer);
    LOG_D("Button %s\n", buffer);
    HEXDUMP_V("Buffer", (uint8_t *)buffer, strlen(buffer));
  }
  
  if (ButtonB.update() > 0) {
    if (!dOn) oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
    dOn = true;
    dTimer = millis();
    switch (ButtonB.getEvent()) {
      case BE_CLICK:
        snprintf(buffer, 64, "B clicked.(%d)", ButtonB.qSize());
        break;
      case BE_DOUBLECLICK:
        snprintf(buffer, 64, "B doubly clicked.(%d)", ButtonB.qSize());
        break;
      case BE_PRESS:
        snprintf(buffer, 64, "B held down.(%d)", ButtonB.qSize());
        oled.setContrast(255);
        break;
      default:
        snprintf(buffer, 64, "Huh? B?(%d)", ButtonB.qSize());
        break;
    }
    oled.println();
    oled.print(buffer);
    tl.println(buffer);
  }

  if (dOn && millis() - dTimer > 5000) {
    oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
    dOn = false;
  }
}

