/**
 * (C) Copyright 2020 Chess Clock Arduino by Antonio Marin Neto
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Neto Marin
* E-mail: contato@netomarin.dev
*/

#include <TM1637Display.h>
#include <EEPROM.h>

// Define the connections pins:
#define CLK_1 9
#define DIO_1 8
#define CLK_2 7
#define DIO_2 6

// Define button and led pins
#define LED_1 13
#define BTN_1 12
#define LED_2 11
#define BTN_2 10
#define BTN_SET 5

// Create display object of type TM1637Display:
TM1637Display displays[] = { TM1637Display(CLK_1, DIO_1), TM1637Display(CLK_2, DIO_2)};

// Create array that turns all segments on:
const uint8_t data[] = {0xff, 0xff, 0xff, 0xff};

// Create array that turns all segments off:
const uint8_t blank[] = {0x00, 0x00, 0x00, 0x00};

// Config mode leds
const uint8_t configMode[] = {SEG_G, SEG_G, SEG_G, SEG_G};

/**
 * Timer constants and settings
 */
int timer[2][2];
#define MIN_INDEX 0
#define SEC_INDEX 1

// Tell it where to store your config data in EEPROM
#define EEPROM_START 0
const char PROGRAM_NAME[] = {'c','h','e','s','s','c','l' 'o','c','k'};

// Structure to save game time
struct StoreStruct {
  unsigned int m;
  unsigned int s;  
} gameTime = {
  // The default values
  1, /** minutes */
  0 /** seconds */
};

/**
 * Game status
 */
#define READY 0
#define RUNNING 1
#define PAUSED 2
#define FINISHED 3
#define CONFIG 4

/**
 * Flow control variables
 */
unsigned long pressedSince;
#define TURN_P1 0
#define TURN_P2 1
int turn;

int gameStatus;
unsigned long setPressed;
long p1Started;
long p2Started;
unsigned long turnStarted = 0;

/**
 * Decrementing player time
 */
void countdown() {
  if (timer[turn][MIN_INDEX] == 0 && timer[turn][SEC_INDEX] == 0) {
    //timer finished
    gameFinished();
  } else if (timer[turn][SEC_INDEX] > 0) {
    timer[turn][SEC_INDEX]--;
  } else {
    if (timer[turn][MIN_INDEX] > 0) {
      timer[turn][MIN_INDEX]--;
    }
    timer[turn][SEC_INDEX] = 59;
  }
}

/**
 * A player has run out of time
 */
void gameFinished() {
  // TO DO: Play a sound
  gameStatus = FINISHED;
  blinkConfigMode();
  displays[turn].setSegments(configMode);
}

/**
 * After P2 pushes the button, change turn to P1
 */
void p1Turn() {
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_1, HIGH);
  gameStatus = RUNNING;
  turn = TURN_P1;
  turnStarted = millis();
}

/**
 * After P1 pushes the button, change turn to P2
 */
void p2Turn() {
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_1, LOW);
  gameStatus = RUNNING;
  turn = TURN_P2;
  turnStarted = millis();
}

/**
 * Define game time before (re)start
 */
void setGameTimer() {
  for(int i = 0; i < 2; i++) {
    timer[i][MIN_INDEX] = gameTime.m;
    timer[i][SEC_INDEX] = gameTime.s;
  }
}

/**
 * Update display with correct time
 */
void updateDisplays(bool setup) {
  if (setup) {
    // Clear the display:
    for (int i = 0; i < 2; i++) {
      displays[i].setBrightness(7);
      displays[i].clear();
    }
  }
  
  // Starting displays with time
  for (int i = 0; i < 2; i++) {
    displays[i].showNumberDecEx(timer[i][MIN_INDEX], 0b11100000, true, 2, 0);
    displays[i].showNumberDec(timer[i][SEC_INDEX], true, 2, 2);
  }
}

/**
 * Preparing config mode data and status
 */
void enterConfigMode() {
  gameStatus = CONFIG;
  setPressed = 0;
  turnStarted = millis();
  blinkConfigMode();  
}

/**
 * Blink ---- on displays to indicate is entering in config mode
 */
void blinkConfigMode() {
  for (int i = 0; i < 2; i++) {
    displays[0].clear();
    displays[1].clear();
    delay(500);
    displays[0].setSegments(configMode);
    displays[1].setSegments(configMode);
    delay(500); 
  }
}

/**
 * Load preivously saved game time. If not found, save a default value.
 */
void loadSavedTime() {
  //checking for memory update
  for (int i = 0; i < sizeof(PROGRAM_NAME); i++) {
    if (PROGRAM_NAME[i] != EEPROM[i]) {
      // Chess clock data not saved. Saving default values.
      saveTime();
      return;
    }
  }

  // If reaches here, there is data saved. Let's read!
  EEPROM.get(EEPROM_START + sizeof(PROGRAM_NAME), gameTime);    
}

/**
 * Save game time to EEPROM
 */
void saveTime() {
  EEPROM.put(EEPROM_START, PROGRAM_NAME);
  EEPROM.put(sizeof(PROGRAM_NAME), gameTime);
}

void setup() {
  //Starting Serial for debug
  Serial.begin(9600);
  
  // Define pins
  pinMode(BTN_SET, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  // Starting both leds ON
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);

  // Loading store game time
  loadSavedTime();

  // Game status
  setPressed = 0;
  gameStatus = READY;
  
  // Set timer
  setGameTimer();

  // Initializing displays with timer
  updateDisplays(true);
}

void loop() {
  // Reading buttons
  int btn_1_value = digitalRead(BTN_1);
  int btn_2_value = digitalRead(BTN_2);
  int btn_set_value = digitalRead(BTN_SET);
  delay(150);

  // Checking if still in config mode
  if (gameStatus == CONFIG) {
    if (btn_set_value == LOW) {      
      if(setPressed == 0) {
        // Defining when set was pressed again during config
        setPressed = millis();
        return;
      } else if(millis() - setPressed > 2000) {
        // Saving timer after presseing set for 2+ seconds
        saveTime();
        setGameTimer();
        gameStatus = READY;
        setPressed = 0;
        blinkConfigMode();
        return;
      }
    }

    // Dismissing config mode if it's idle for 5+ seconds
    if (millis() - turnStarted > 5000) {      
      loadSavedTime();
      updateDisplays(false);
      gameStatus = READY;
      setPressed = 0;
      blinkConfigMode();
      return;
    }

    // Increasing values
    if (btn_1_value == LOW) {
      gameTime.m++;
      if (gameTime.m > 99) {
        gameTime.m = 0;
      }
    }

    if (btn_2_value == LOW && gameStatus == CONFIG) {
      gameTime.s++;
      if (gameTime.s > 59) {
        gameTime.s = 0;
      }
    }

    // Printing setup time
    displays[0].showNumberDecEx(gameTime.m, 0b11100000, true, 2, 0);
    displays[0].showNumberDec(gameTime.s, true, 2, 2);
    turnStarted = millis();
  } else {
    // Not in config mode
    if (btn_set_value == LOW && (gameStatus == PAUSED || gameStatus == READY)) {
      if (setPressed == 0) {
        // First time pressing SET
        setPressed = millis();
        return;
      } else if (millis() - setPressed >= 2000){
        // After 2 seconds, entering config mode.
        enterConfigMode();
        return;
      }
    } else if (btn_1_value == LOW && btn_2_value == LOW) {
      // Both buttons pressed. Stop both clocks.
      digitalWrite(LED_2, HIGH);
      digitalWrite(LED_1, HIGH);
      gameStatus = PAUSED;
    } else if (btn_1_value == LOW && (turn == TURN_P1 || gameStatus == READY || gameStatus == PAUSED)) {
      p2Turn();    
    } else if (btn_2_value == LOW && (turn == TURN_P2 || gameStatus == READY || gameStatus == PAUSED)) {
      p1Turn();
    }
  
    if (gameStatus == RUNNING && (millis() - turnStarted) >= 1000) {
      // 1 second passed
      countdown();
      turnStarted = millis();
    }

    updateDisplays(false);
  }
}
