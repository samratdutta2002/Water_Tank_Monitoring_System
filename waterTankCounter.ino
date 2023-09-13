#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "PinChangeInterrupt.h"

#define longPressTime 3000
#define resetEEPROMTime 5000
#define EEPROM_Offset 0
#define T1 20 // timer 1 in minutes
#define T2 30 // timer 2 in minues
#define T3 40 // timer 3 in minutes
#define pumpWatt 1000.0 // watt of your pump
#define wattSec 3600000.0

#define B1 2
#define B2 3
#define B3 4
#define B4 5
#define pump 12

LiquidCrystal_I2C lcd(0x3F, 16, 2);
volatile bool B1P, B2P, B3P, B4P, pumpSwitch;
bool B1LP, B2LP, B3LP, B4LP, BLflag, resetEEPROM;
byte pumpTimeLeft;
int sec;
unsigned long timer, pumpTimer, totalTimeSec;
String unit;

void initialiseButtons() {
  B1P = false;
  B2P = false;
  B3P = false;
  B4P = false;
  B1LP = false;
  B2LP = false;
  B3LP = false;
  B4LP = false;
}

void buttonOne() {
  B1P = true;
}
void buttonTwo() {
  B2P = true;
}
void buttonThree() {
  B3P = true;
}
void buttonFour() {
  B4P = true;
}

void resetMemory() {  // unit counter reset
  EEPROM.write(EEPROM_Offset + 0, 0);
  EEPROM.write(EEPROM_Offset + 1, 0);
  EEPROM.write(EEPROM_Offset + 2, 0);
  EEPROM.write(EEPROM_Offset + 3, 0);
  EEPROM.write(EEPROM_Offset + 4, 0);
  EEPROM.write(EEPROM_Offset + 5, 0);
  EEPROM.write(EEPROM_Offset + 6, 0);
  EEPROM.write(EEPROM_Offset + 7, 0);
  EEPROM.write(EEPROM_Offset + 8, 0);
  EEPROM.write(EEPROM_Offset + 9, 0);
}

void initialisePump() {
  pumpTimeLeft = EEPROM.read(EEPROM_Offset + 0);
  sec = EEPROM.read(EEPROM_Offset + 1);
  if (sec > 0 || pumpTimeLeft > 0) {
    pumpTimer = millis();
    pumpSwitch = true;
  } else pumpTimer = 0;
}

void updateTotalTime() {  // called every second , add 2, 3, 4, 5, 6, 7, 8, 9
  byte add = EEPROM_Offset + 2, t;
  while (true) {
    if (EEPROM.read(add) >= 255) {
      EEPROM.write(add, 0);
      add = add + 1;
      continue;
    }
    t = EEPROM.read(add);
    EEPROM.write(add, t + 1);
    break;
  }
}

void getPumpUpTime() {
  totalTimeSec = (EEPROM.read(EEPROM_Offset + 3) * 255) + (EEPROM.read(EEPROM_Offset + 4) * 255) + (EEPROM.read(EEPROM_Offset + 5) * 255) + EEPROM.read(EEPROM_Offset + 2);
  unit = String(((((double)(totalTimeSec)) * pumpWatt) / wattSec), 4);
  //Serial.print("\nMotor has been runing for total: ");
  //Serial.print(totalTimeSec);
  //Serial.println(" seconds");
  //Serial.println("Total unit: " + unit);
}


void monitorPump() {
  if (sec > 0 || pumpTimeLeft > 0) {

    if ((millis() - pumpTimer) >= 1000) {
      sec--;  // 1 second elapsed
      updateTotalTime();
      if (pumpTimeLeft == 0 && sec <= 10) {
        BLflag = !BLflag;
        if (BLflag) lcd.backlight();
        else lcd.noBacklight();
      }
      EEPROM.write(EEPROM_Offset + 1, (byte)((sec <= -1) ? 59 : sec));
      pumpTimer = millis();
      if (sec <= -1) {  // 1 minute elapsed
        sec = 59;
        pumpTimeLeft--;
        EEPROM.write(EEPROM_Offset + 0, pumpTimeLeft);
      }
      getPumpUpTime();
      updateLCD();
    }
  } else {
    if (pumpSwitch) {
      pumpSwitch = false;
      getPumpUpTime();
      lcd.backlight();
      updateLCD();
    }
    pumpTimer = 0;
    sec = 0;
  }
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P: ");
  lcd.print((pumpSwitch) ? "On" : "Off");
  lcd.setCursor(8, 0);
  lcd.print("T: ");
  lcd.print(pumpTimeLeft);
  lcd.print(":");
  lcd.print(sec);
  lcd.setCursor(0, 1);
  lcd.print("Unit: " + unit);
}

void setup() {

  //Serial.begin(9600);

  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);
  pinMode(B4, INPUT_PULLUP);
  pinMode(pump, OUTPUT);

  lcd.init();
  lcd.backlight();

  initialiseButtons();
  initialisePump();

  attachInterrupt(0, buttonOne, FALLING);
  attachInterrupt(1, buttonTwo, FALLING);
  attachPCINT(digitalPinToPCINT(B3), buttonThree, FALLING);
  attachPCINT(digitalPinToPCINT(B4), buttonFour, FALLING);
  //lcd.noBacklight();
  getPumpUpTime();
  updateLCD();
}
void loop() {


  monitorPump();

  if (pumpSwitch) digitalWrite(pump, HIGH);
  else digitalWrite(pump, LOW);

  if (B4P) {
    B4P = false;
    timer = millis();
    resetEEPROM = false;
    B4LP = false;
    while (!digitalRead(B4)) {
      if (pumpSwitch && (millis() - timer >= longPressTime)) {
        B4LP = true;
        break;
      }
      if (!pumpSwitch && (millis() - timer >= resetEEPROMTime)) {
        resetEEPROM = true;
        break;
      }
    }

    if (resetEEPROM) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Reset EEPROM");
      lcd.setCursor(0, 1);
      lcd.print("Yes");
      lcd.setCursor(14, 1);
      lcd.print("No");
      while (!digitalRead(B4))
        ;
      while (true) {
        if (B1P) {
          B1P = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Deleting Memory!");
          resetMemory();
          delay(500);
          lcd.setCursor(3, 1);
          lcd.print("Successful");
          delay(1000);
          initialiseButtons();
          getPumpUpTime();
          updateLCD();
          break;
        }
        if (B3P) {
          B3P = false;
          getPumpUpTime();
          updateLCD();
          delay(200);
          initialiseButtons();
          break;
        }
      }
    }

    if (B4LP && pumpSwitch) {
      //Serial.println("Push Button Long Pressed!");
      pumpTimeLeft = 0;
      EEPROM.write(EEPROM_Offset + 0, 0);
      EEPROM.write(EEPROM_Offset + 1, 0);
      sec = 0;
      getPumpUpTime();
      updateLCD();
      monitorPump();
      if (pumpSwitch) digitalWrite(pump, HIGH);
      else digitalWrite(pump, LOW);
      while (!digitalRead(B4))
        ;
    }
  }


  if (B1P) {
    B1P = false;
    //Serial.println("Button 1 Pressed");

    if (!pumpSwitch) {
      pumpTimeLeft = T1 - 1;
      sec = 59;
      EEPROM.write(EEPROM_Offset + 0, pumpTimeLeft);
      EEPROM.write(EEPROM_Offset + 1, (byte)(sec));
      pumpSwitch = true;
      pumpTimer = millis();
    }
    getPumpUpTime();
    updateLCD();
  }

  if (B2P) {
    B2P = false;
    //Serial.println("Button 2 Pressed");

    if (!pumpSwitch) {
      pumpTimeLeft = T2 - 1;
      sec = 59;
      EEPROM.write(EEPROM_Offset + 0, pumpTimeLeft);
      EEPROM.write(EEPROM_Offset + 1, (byte)(sec));
      pumpSwitch = true;
      pumpTimer = millis();
    }
    getPumpUpTime();
    updateLCD();
  }

  if (B3P) {
    B3P = false;
    //Serial.println("Button 3 Pressed");

    if (!pumpSwitch) {
      pumpTimeLeft = T3 - 1;
      sec = 59;
      EEPROM.write(EEPROM_Offset + 0, pumpTimeLeft);
      EEPROM.write(EEPROM_Offset + 1, (byte)(sec));
      pumpSwitch = true;
      pumpTimer = millis();
    }
    getPumpUpTime();
    updateLCD();
  }
}