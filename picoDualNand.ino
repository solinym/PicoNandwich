#include "pico/bootrom.h"
#include <EEPROM.h>

const int CE_NAND_A   = 3;
const int CE_NAND_B   = 4;
const int SYNC_BUTTON = 2;
const int LED_PIN     = LED_BUILTIN;
const int LED_EXT     = 1;
const int BEEP_PIN    = 0;

bool nandASelected = true;
bool lastButtonState = HIGH;
const int EEPROM_SIZE = 256; 
const int ADDR_STATE  = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("--- PicoNandwich v1.0 | By: solinym ---");
  pinMode(CE_NAND_A, OUTPUT);
  pinMode(CE_NAND_B, OUTPUT);
  pinMode(SYNC_BUTTON, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);
  pinMode(LED_EXT, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  uint8_t savedState = EEPROM.read(ADDR_STATE);
  
  if (savedState == 0) { //NAND 1 Save State
    nandASelected = true;
    Serial.println("SAVE STATE: Loaded NAND 1");
  } else if (savedState == 1) { //NAND 2 Save State
    nandASelected = false;
    Serial.println("SAVE STATE: Loaded NAND 2");
  } else { //Default to NAND 1 if flash is blank.
    Serial.println("SAVE STATE: Flash is blank, defaulting to NAND 1.");
    nandASelected = true;
    updateStorage(0);
  }
  applyNandLogic(false);
}

const unsigned long HOLD_TIME = 750;
unsigned long pressStart = 0;
bool holding = false;
const unsigned long BOOTSEL_HOLD = 8000;

//Main Loop | Handles sync button logic
void loop() {
  bool button = digitalRead(SYNC_BUTTON);

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); 
    if (command.length() > 0) {
      handleStringCommands(command);
    }
  }

  if (button == LOW && lastButtonState == HIGH) {
    pressStart = millis();
    holding = false;
  }

  if (button == LOW) {
    unsigned long held = millis() - pressStart;

    if (held > BOOTSEL_HOLD) {
      Serial.println("SYNC_BTN: Entering BOOTSEL/flashing mode. REMEMBER: YOU CAN NOT RE-ENTER BOOTSEL IF PICO DOESNT HAVE PicoNandwich INSTALLED OR IF YOU CAN ACCESS THE PHYSICAL BUTTON ON THE PICO. DO NOT UNPLUG YOUR CONSOLE.");
      blinkLEDTime(3, 500);
      beepTime(1, 2000, 500);
      reset_usb_boot(0, 0);
    }
  }

  if (button == LOW && !holding && millis() - pressStart > HOLD_TIME) {
    holding = true;
    Serial.println("SYNC_BTN: Switching NAND");
    toggleNand();
  }

  lastButtonState = button;
}

//Toggles between NAND 1/NAND 2 (will be deprecated with triple/quad nand release)
void toggleNand() {
  nandASelected = !nandASelected;
  Serial.println(nandASelected ? "NAND: Switched to NAND 1" : "NAND: Switched to NAND 2");

  applyNandLogic(true);
  updateStorage(nandASelected ? 0 : 1);
}

//Nand switching logic
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

//Update save state in Pico flash
void updateStorage(uint8_t val) {
  // Only write if the value has changed.
  if (EEPROM.read(ADDR_STATE) != val) {
    EEPROM.write(ADDR_STATE, val);
    if(EEPROM.commit()) {
       Serial.println("SAVE STATE: NAND state saved to flash.");
    } else {
       Serial.println("SAVE STATE: ERROR saving NAND state to flash!");
    }
  }
}


//Blink onboard/ROL LED (Number of blinks, Duration of blinks)
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

//Play beeps through a speaker/buzzer (Number of beeps, Duration of beeps, Delay between beeps)
void beepTime(int times, int duration, int del){
  for(int i=0; i<times; i++){
    tone(BEEP_PIN, 2000, duration);
    delay(del);
  }
}

//Handle Serial Commands (lol this is so bad)
void handleStringCommands(String cmd) {
  if (cmd == "switch_nand") {
    Serial.println("USB_DEBUG: Switching NAND...");
    toggleNand();
  } 
  else if (cmd == "bootsel") {
    Serial.println("USB_DEBUG: Resetting Pico to bootloader... (REMEMBER: YOU CAN NOT RE-ENTER BOOTSEL IF PICO DOESNT HAVE PicoNandwich INSTALLED OR IF YOU CAN ACCESS THE PHYSICAL BUTTON ON THE PICO. DO NOT UNPLUG YOUR CONSOLE.)");
    delay(500);
    reset_usb_boot(0, 0);
  } 
  else if (cmd == "status") {
    Serial.println("");
    Serial.println("--- PicoNandwich Status ---");
    Serial.println("Version: v1.0");
    Serial.print("Active NAND: ");
    Serial.println(nandASelected ? "1" : "2");
    Serial.print("CE 1 STATUS: ");
    Serial.println(digitalRead(CE_NAND_A) == LOW ? "LOW" : "HIGH");
    Serial.print("CE 2 STATUS: ");
    Serial.println(digitalRead(CE_NAND_B) == LOW ? "LOW" : "HIGH");
    Serial.println("CONSOLE POWER STATUS: N/A (Not Implemented)");
    Serial.println("");
  } 
  else if (cmd == "help") {
    Serial.println("");
    Serial.println("--- Available Commands ---");
    Serial.println("switch_nand - Switches between available NANDs.");
    Serial.println("status      - Shows active NAND and other useful information.");
    Serial.println("bootsel     - Reboots Pico for firmware update.");
    Serial.println("");
  } 
  else {
    Serial.print("USB_DEBUG: Unknown command: '");
    Serial.print(cmd);
    Serial.println("' - Type 'help' for list.");
  }
}