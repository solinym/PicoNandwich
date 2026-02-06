/*
 * PicoNandwich v1.1
 * Dual-NAND Switching for Xbox 360 using Raspberry Pi Pico
 * Created by: solinym
 */

#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <EEPROM.h>
#include <Wire.h>

#define UART_ID uart0
#define BAUD_RATE 115200
#define XBOX_RX_PIN 17

// Hardware Pin Definitions
const int CE_NAND_A   = 3;
const int CE_NAND_B   = 4;
const int SYNC_BUTTON = 2;
const int LED_PIN     = LED_BUILTIN;
const int LED_EXT     = 1;
const int BEEP_PIN    = 0;

// Persistent Configuration Structure
struct Config {
  uint32_t magic;
  uint8_t activeNand;
  bool uartEnabled;
  bool powerSenseOn;
  float psAverage;
  float version;
};

Config sysSettings;
const uint32_t SETTINGS_MAGIC = 0x534F4C49;
bool nandASelected = true;
bool lastButtonState = HIGH;
const int EEPROM_SIZE = 256; 
const int ADDR_STATE  = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("--- PicoNandwich v1.1 | By: solinym ---");

  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  // UART Setup
  Serial1.setRX(XBOX_RX_PIN);
  Serial1.begin(115200);

  pinMode(CE_NAND_A, OUTPUT);
  pinMode(CE_NAND_B, OUTPUT);
  pinMode(SYNC_BUTTON, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);
  pinMode(LED_EXT, OUTPUT);

  // Initialize NAND state from flash
  nandASelected = (sysSettings.activeNand == 0);
  applyNandLogic(false);
}

// Button timing constants
const unsigned long HOLD_TIME = 750;
unsigned long pressStart = 0;
bool holding = false;
const unsigned long BOOTSEL_HOLD = 8000;

void loop() {
  bool button = digitalRead(SYNC_BUTTON);

  // Process USB Serial Commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); 
    if (command.length() > 0) {
      handleStringCommands(command);
    }
  }

  // Process SMC UART logs
  if (Serial1.available() && sysSettings.uartEnabled == true) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.print(c); 
    }
  }

  // Handle Button Press
  if (button == LOW && lastButtonState == HIGH) {
    pressStart = millis();
    holding = false;
  }

  // Handle long-press for BOOTSEL
  if (button == LOW) {
    unsigned long held = millis() - pressStart;
    if (held > BOOTSEL_HOLD) {
      Serial.println("[SYNC_BTN]: Entering BOOTSEL/flashing mode...");
      blinkLEDTime(3, 500);
      beepTime(1, 2000, 500);
      reset_usb_boot(0, 0);
    }
  }

  // Handle short-press for NAND toggle
  if (button == LOW && !holding && millis() - pressStart > HOLD_TIME) {
    holding = true;
    Serial.println("[SYNC_BTN]: Switching NAND");
    toggleNand();
  }

  lastButtonState = button;
}

// Logic to switch NAND banks
void toggleNand() {
  if(isConsoleOn() && sysSettings.powerSenseOn == true){
    Serial.println("[NAND]: Switch denied. Please power off your console first.");
    return;
  }
  nandASelected = !nandASelected;
  Serial.println(nandASelected ? "[NAND]: Switched to NAND 1" : "[NAND]: Switched to NAND 2");

  applyNandLogic(true);
  saveConfig();
}

// Pin level logic for Chip Enable (CE)
void applyNandLogic(bool notify) {
  if (nandASelected) {
    digitalWrite(CE_NAND_A, LOW);
    digitalWrite(CE_NAND_B, HIGH);
    if (notify) {
      blinkLEDTime(1, 50);
      beepTime(1, 50, 70);
    }
  } else {
    digitalWrite(CE_NAND_A, HIGH);
    digitalWrite(CE_NAND_B, LOW);
    if (notify) {
      blinkLEDTime(2, 50);
      beepTime(2, 50, 70);
    }
  }
}

// Write settings to EEPROM
void saveConfig() {
  sysSettings.magic = SETTINGS_MAGIC;
  sysSettings.activeNand = nandASelected ? 0 : 1;
  EEPROM.put(ADDR_STATE, sysSettings);
  EEPROM.commit();
}

// Load settings from EEPROM
void loadConfig() {
  EEPROM.get(ADDR_STATE, sysSettings);
  if (sysSettings.magic != SETTINGS_MAGIC || sysSettings.version < 1.1) {
    Serial.println("[SAVE STATE]: Flash is blank/old, initializing defaults.");
    sysSettings.magic = SETTINGS_MAGIC;
    sysSettings.activeNand = 0;
    sysSettings.uartEnabled = false;
    sysSettings.powerSenseOn = false;
    sysSettings.psAverage = 350.0;
    sysSettings.version = 1.1;
    saveConfig();
  }
}

// Feedback: LED Blinking
void blinkLEDTime(int times, int duration){
  for(int i=0; i<times; i++){
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_EXT, HIGH);
    delay(duration);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED_EXT, LOW);
    delay(duration);
  }
}

// Feedback: Buzzer Beeps
void beepTime(int times, int duration, int del){
  for(int i=0; i<times; i++){
    tone(BEEP_PIN, 2000, duration);
    delay(del);
  }
}

// Detect if console is powered via ADC on GP28
bool isConsoleOn() {
  int readings = 0;
  int count = 5; 
  for(int i = 0; i < count; i++) {
    readings += analogRead(28);
    delay(1); 
  }
  float average = readings / (float)count;
  return (average >= sysSettings.psAverage); 
}

// Serial Terminal Command Processor
void handleStringCommands(String cmd) {
  if (cmd == "switch_nand") {
    Serial.println("[USB_COM]: Switching NAND...");
    toggleNand();
  } 
  else if (cmd == "bootsel") {
    Serial.println("[USB_COM]: Entering Bootloader...");
    delay(500);
    reset_usb_boot(0, 0);
  } 
  else if (cmd == "status") {
    Serial.println("\n--- PicoNandwich Status ---");
    Serial.println("Version: v1.1");
    Serial.print("Active NAND: "); Serial.println(nandASelected ? "1" : "2");
    Serial.print("CE 1 STATUS: "); Serial.println(digitalRead(CE_NAND_A) == LOW ? "LOW" : "HIGH");
    Serial.print("CE 2 STATUS: "); Serial.println(digitalRead(CE_NAND_B) == LOW ? "LOW" : "HIGH");
    Serial.print("UART LOGGING: "); Serial.println(sysSettings.uartEnabled ? "ENABLED" : "DISABLED");
    Serial.print("POWER SENSING: "); Serial.println(sysSettings.powerSenseOn ? "ENABLED" : "DISABLED");
    Serial.print("PS THRESHOLD: "); Serial.println(sysSettings.psAverage);
    Serial.print("CONSOLE POWER: "); Serial.println(isConsoleOn() ? "ON" : "OFF");
    Serial.println("");
  } 
  else if (cmd == "help") {
    Serial.println("\n--- Available Commands ---");
    Serial.println("switch_nand   - Toggle NAND banks");
    Serial.println("uart_toggle   - Toggle SMC UART logging");
    Serial.println("ps_toggle     - Toggle Power Sensing safety");
    Serial.println("ps_set <val>  - Manual threshold set");
    Serial.println("ps_calibrate  - Auto-tune threshold");
    Serial.println("ps_monitor    - Live ADC monitoring");
    Serial.println("status        - System status");
    Serial.println("bootsel       - Update firmware\n");
  } 
  else if (cmd == "uart_toggle"){
    sysSettings.uartEnabled = !sysSettings.uartEnabled;
    saveConfig();
    Serial.println(sysSettings.uartEnabled ? "[USB_COM]: UART Enabled" : "[USB_COM]: UART Disabled");
  }
  else if (cmd == "ps_toggle"){
    sysSettings.powerSenseOn = !sysSettings.powerSenseOn;
    saveConfig();
    Serial.println(sysSettings.powerSenseOn ? "[USB_COM]: Power Sensing Enabled" : "[USB_DEBUG]: Power Sensing Disabled");
  }
  else if (cmd.startsWith("ps_set ")){
    sysSettings.psAverage = cmd.substring(7).toFloat();
    saveConfig();
    Serial.print("[USB_COM]: Power Sensing threshold set to: "); Serial.println(sysSettings.psAverage);
  }
  else if (cmd == "ps_calibrate"){
    Serial.println("[USB_COM]: Calibrating... Ensure console is ON.");
    long total = 0;
    for(int i=0; i<50; i++) {
        total += analogRead(28);
        delay(10);
    }
    float avg = total / 50.0;
    if(avg < 50) Serial.println("[USB_COM]: Voltage low. Check connections!");
    sysSettings.psAverage = avg * 0.8; 
    saveConfig();
    Serial.print("[USB_COM]: Calibrated! New threshold: "); Serial.println(sysSettings.psAverage);
  }
  else if (cmd == "ps_monitor") {
    Serial.println("[USB_COM]: Monitoring ADC (5 seconds)...");
    for(int i=0; i<20; i++) {
      Serial.print("ADC: "); Serial.println(analogRead(28));
      delay(250);
    }
  }
  else {
    Serial.print("[USB_COM]: Unknown command: '");
    Serial.print(cmd);
    Serial.println("' - Type 'help' for list.");
  }
}