/*
    Mini Matrix Game (Bomberman)

    The classic Bomberman game is emulated on an 8x8 matrix, featuring an LCD Display that displays various game information,
    including player status, welcome and end game messages, and the number of lives remaining.
    The game comprises three elements: players (blinking slowly), bombs (blinking rapidly), and walls (non-blinking).
    A randomly generated map, with destructible walls (unbreakable borders), is created each time the game starts.
    Players navigate using a joystick, placing bombs that destroy walls in a plus pattern. The objective is to demolish
    as many walls as possible without succumbing to the blast. The game is timed, and the score is determined by
    the number of walls destroyed. Players have a set number of lives, which can be lost if caught in a bomb's blast
    radius during detonation. The logical game matrix is 16x16, but only an 8x8 portion is displayed, utilizing the
    fog of war concept. At the end of the game, the final score is showcased, and players are prompted if they have beaten
    the highscore. Each player can choose a three-character name, saved in the leaderboard along with their score.
    Highscores can be reset, setting all scores to 0 and player names to XXX. The game includes informative sections about
    the game and its creator, as well as a guide on how to play. Players can select from three difficulty levels (low, medium, high),
    affecting gameplay elements such as the number of lives, bomb explosion speed, map density, and points awarded for each destroyed wall.
    The game also features a settings menu, allowing players to adjust the LCD and matrix brightness, toggle sound on/off, and change their name.
    Sounds are played when navigating the menu, placing bombs, and detonating them. The game is controlled using a joystick, and the
    LCD contrast can be adjusted using a potentiometer.

    The circuit:

      Input:
        1 x Joystick - used for player movement - pins A0, A1, 13
      
      Output:
        1 x MAX7219 Led matrix driver - pins 10, 11, 12
        1 x 8x8 Led matrix controlled by the MAX7219 driver
        1 x 10uF capacitor to reduce power spikes when the matrix is fully on
        1 x 0.1uF capacitor to filter the noise on 5V
        1 x 10K resitor for the ISET pin on the matrix driver
        1 x LCD Display for displaying various info - pins 3, 4, 5, 6, 7, 8, 9
        1 x 50K Resistors to adjust LCD Contrast

    Menu structure:

    1. Start game
    2. Settings
      2.1. LCD brightness control
      2.2. Matrix brightness control
      2.3. Sounds - ON / OFF
      2.4. Player Name
    3. About
    4. Highscores
    5. Reset Highscores
    6. How to play
    7. Difficulty

    Created 24.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

const byte driverDin = 12;    // pin 12 is connected to the MAX7219 pin 1
const byte driverClock = 11;  // pin 11 is connected to the CLK pin 13
const byte driverLoad = 10;   // pin 10 is connected to LOAD pin 12

const byte joystickAxisX = A0;
const byte joystickAxisY = A1;
const byte joystickButton = 13;

// LCD pins
const byte lcdRs = 3;
const byte lcdEn = 8;
const byte lcdD4 = 7;
const byte lcdD5 = 6;
const byte lcdD6 = 5;
const byte lcdD7 = 4;
const byte lcdBacklight = 9;
const byte buzzerPin = 2;

const int bombTone = 700;

LiquidCrystal lcd(lcdRs, lcdEn, lcdD4, lcdD5, lcdD6, lcdD7);
byte lcdBrightness = 10;
byte previousLcdBrightness = 10;
byte lcdLineWidth = 16;
int lcdScrollInterval = 400;

LedControl lc = LedControl(driverDin, driverClock, driverLoad, 1);
const byte matrixSize = 16;
byte matrixBrightness = 2;
byte previousMatrixBrightness = 2;

const int playerBlinkInterval = 600;  // 0.6 seconds
unsigned long lastPlayerBlink = 0;
byte playerBlinkState = 1;
byte previousPlayerBlinkState = 1;

byte playerX = 3;
byte playerY = 3;

bool joyMoved = false;
int minJoyThreshold = 400;
int maxJoyThreshold = 600;

int xAxisValue = 0;
int yAxisValue = 0;

byte currentMovement = 0;
byte previousMovement = 0;

int bombTimer = 3000;
unsigned long bombPlacedTime;
bool bombPlaced = false;

byte bombX = 0;
byte bombY = 0;

byte bombBlinkState = 0;
int bombBlinkInterval = 100;  // 0.1 seconds
unsigned long lastBombBlink = 0;

byte debounceDelay = 100;
byte joyButtonState = 0;
byte joyButtonReading = 0;
unsigned long lastJoyPress = 0;

bool inGame = false;
byte menuPosition = 1;
byte previousMenuPosition = 0;

byte noLives = 3;
byte previousLives = 3;

unsigned long playTime = 0;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

bool inAbout = false;

String aboutString = " Bomberman by Bogdan Comardici - github.com/bogdancomardici";
byte aboutStringPos = 0;
byte aboutStringLen = 56;


bool inSettings = false;
byte settingsPosition = 1;
byte previousSettingsPosition = 0;

bool inSettingsInput = false;
byte settingsInput = 0;

bool inEndGameScreen = false;

bool inMenu = true;

bool soundOn = false;
bool previousSoundState = false;

char playerName[3] = "XXX";
char previousName[3] = "YYY";
byte namePosition = 0;

int score = 0;
byte difficulty = 1;

int highscores[5] = { 0, 0, 0, 0, 0 };
char highscoreNames[5][3] = { "AAA", "BBB", "CCC", "DDD", "EEE" };
bool inHighscores = false;
byte highscorePosition = 0;
byte previousHighscorePosition = 1;

byte lcdBrightnessMemoryAddr = 0;
byte matrixBrightnessMemoryAddr = sizeof(byte);
byte soundStateMemoryAddr = sizeof(byte) + matrixBrightnessMemoryAddr;
byte difficultyMemoryAddr = 2 * sizeof(byte) + soundStateMemoryAddr;
byte highScoresMemoryAddr = 3 * sizeof(char) + difficultyMemoryAddr;
byte highScoreNamesMemoryAddr = 5 * sizeof(int) + highScoresMemoryAddr;
byte playerNameMemoryAddr = 5 * 3 * sizeof(char) + highScoreNamesMemoryAddr;


bool inResetHighscores = false;
bool inHowTo = false;
bool inDifficulty = false;
byte previousDifficulty = 0;

const int visibleSize = 8;  // Size of the visible portion

byte mapMatrix[matrixSize][matrixSize] = {
  { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
  { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }
};

byte arrowDownChar[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
};

byte arrowUpAndDownChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
};

byte arrowUpChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte arrowLeftChar[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b01100,
  0b11100,
  0b01100,
  0b00100,
  0b00000
};

byte arrowRightChar[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00110,
  0b00111,
  0b00110,
  0b00100,
  0b00000,
};

byte heartChar[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

const uint64_t matrixImages[7] = {
  0x0066999d91660000,
  0xc3e766181927468c,
  0x18181800183c3c18,
  0x7e18183c7e7e7e00,
  0xc3e77e3c3c7ee7c3,
  0x1800183862663c00,
  0xfffffcfcf0f0c0c0
};
const int matrixImagesLen = sizeof(matrixImages) / 8;

void setup() {

  lc.shutdown(0, false);
  lc.clearDisplay(0);

  pinMode(joystickAxisX, INPUT);
  pinMode(joystickAxisY, INPUT);
  pinMode(joystickButton, INPUT_PULLUP);

  pinMode(driverDin, OUTPUT);
  pinMode(driverClock, OUTPUT);
  pinMode(driverLoad, OUTPUT);

  pinMode(buzzerPin, OUTPUT);

  pinMode(lcdRs, OUTPUT);
  pinMode(lcdEn, OUTPUT);
  pinMode(lcdD4, OUTPUT);
  pinMode(lcdD5, OUTPUT);
  pinMode(lcdD6, OUTPUT);
  pinMode(lcdD7, OUTPUT);
  pinMode(lcdBacklight, OUTPUT);

  // read data from eeprom;

  readIntArrayFromEEPROM(highScoresMemoryAddr, highscores, 5);
  readHighscoreNames(highScoreNamesMemoryAddr, highscoreNames, 5);
  EEPROM.get(lcdBrightnessMemoryAddr, lcdBrightness);
  EEPROM.get(matrixBrightnessMemoryAddr, matrixBrightness);
  EEPROM.get(soundStateMemoryAddr, soundOn);
  EEPROM.get(playerNameMemoryAddr, playerName);
  EEPROM.get(difficultyMemoryAddr, difficulty);
  // number of lives based on difficulty
  noLives = 4 - difficulty;
  // 1, 2, 3 seconds based on difficulty
  bombTimer = 4000 - 1000 * difficulty;
  // bomb blinks faster on higher difficulty
  bombBlinkInterval = 120 - 20 * difficulty;
  // set lcd and matrix brightness
  lc.setIntensity(0, matrixBrightness);
  analogWrite(lcdBacklight, lcdBrightness * 25);

  // initialize the LCD
  lcd.createChar(0, arrowDownChar);
  lcd.createChar(1, arrowUpAndDownChar);
  lcd.createChar(2, arrowUpChar);
  lcd.createChar(3, heartChar);
  lcd.createChar(4, arrowLeftChar);
  lcd.createChar(5, arrowRightChar);
  lcd.begin(16, 2);
  welcomeMessage();
  // analog input pin 3 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  randomSeed(analogRead(A3));

  generateMap();
}
void loop() {
  // read the joystick values

  analogWrite(lcdBacklight, lcdBrightness * 25);
  xAxisValue = analogRead(joystickAxisX);
  yAxisValue = analogRead(joystickAxisY);
  joyButtonReading = digitalRead(joystickButton);

  joyButtonState = debounceInput(joystickButton, &joyButtonReading, &lastJoyPress, debounceDelay);
  currentMovement = joyDirection(xAxisValue, yAxisValue, minJoyThreshold, maxJoyThreshold, &joyMoved);

  // keep the bomb state from getting stuck on
  if (!bombPlaced && (inEndGameScreen || inGame)) {
    bombBlinkState = false;
    noTone(buzzerPin);
  }

  // make sound on menu and settings change
  if (currentMovement != previousMovement && currentMovement != 4 && !inGame && soundOn) {
    tone(buzzerPin, bombTone, 60);
  }

  if (inGame) {
    handleGameLogic();
  } else if (inAbout) {
    printAbout();
  } else if (inSettings) {
    navigateSettings();
  } else if (inSettingsInput) {
    handleSettingsInput();
  } else if (inMenu) {
    navigateMenu();
  } else if (inEndGameScreen) {
    if (currentMovement != previousMovement) {
      inEndGameScreen = false;
      resetGame();
    }
    previousMovement = currentMovement;
  } else if (inHighscores) {
    navigateHighscores();
  } else if (inResetHighscores) {
    handleHighscoreReset();
  } else if (inHowTo) {
    handleHowTo();
  } else if (inDifficulty) {
    handleDifficultyChange();
  }
}
// generate map in a random manner
void generateMap() {
  for (int row = 1; row < matrixSize - 1; row++)
    for (int col = 1; col < matrixSize - 1; col++) {
      switch (difficulty) {
        case 1:
          mapMatrix[row][col] = random(0, 2);
          break;
        case 2:
          mapMatrix[row][col] = random(0, 3);
          break;
        case 3:
          mapMatrix[row][col] = random(0, 4);
          break;
        default:
          break;
      }
      // normalize values;
      if (mapMatrix[row][col] > 0)
        mapMatrix[row][col] = 1;
    }

  // make space for the player to start
  mapMatrix[3][3] = 0;
  mapMatrix[3][4] = 0;
  mapMatrix[4][3] = 0;
  mapMatrix[4][4] = 0;
}

// render the map keeping the player in the center
void renderMap() {
  for (int row = 0; row < visibleSize; row++)
    for (int col = 0; col < visibleSize; col++)
      if (playerX - 3 + row > 15 || playerY - 3 + col > 15 || playerX - 3 + row < 0 || playerY - 3 + col < 0) {
        lc.setLed(0, row, col, 0);
      } else
        lc.setLed(0, row, col, mapMatrix[playerX - 3 + row][playerY - 3 + col]);
}

void renderPlayer() {
  lc.setLed(0, 3, 3, playerBlinkState);  // render player in the center of the visible portion
}

void renderBomb() {
  // render the bomb only if it is within the visible portion
  if (bombX >= playerX - 3 && bombX < playerX + 5 && bombY >= playerY - 3 && bombY < playerY + 5)
    lc.setLed(0, bombX - playerX + 3, bombY - playerY + 3, bombBlinkState);
}

void placeBomb(byte xPosition, byte yPosition) {
  // ensure that the bomb is placed within the visible portion
  if (!bombPlaced && xPosition >= playerX - 3 && xPosition < playerX + 5 && yPosition >= playerY - 3 && yPosition < playerY + 5) {
    bombX = xPosition;
    bombY = yPosition;
    bombPlaced = true;
    bombPlacedTime = millis();
  }
}

// detonate the bomb and destroy the walls in a plus pattern
void detonateBomb() {
  if (bombPlaced) {
    mapMatrix[bombX][bombY] = 0;
    if (bombX - 1 >= 0) {
      if (mapMatrix[bombX - 1][bombY] == 1) {
        score += 10 + 5 * difficulty;
        mapMatrix[bombX - 1][bombY] = 0;
      }
    }

    if (bombX + 1 < matrixSize) {
      if (mapMatrix[bombX + 1][bombY] == 1) {
        score += 10 + 5 * difficulty;
        mapMatrix[bombX + 1][bombY] = 0;
      }
    }

    if (bombY - 1 >= 0) {
      if (mapMatrix[bombX][bombY - 1] == 1) {
        score += 10 + 5 * difficulty;
        mapMatrix[bombX][bombY - 1] = 0;
      }
    }

    if (bombY + 1 < matrixSize) {
      if (mapMatrix[bombX][bombY + 1] == 1) {
        score += 10 + 5 * difficulty;
        mapMatrix[bombX][bombY + 1] = 0;
      }
    }

    // check if player is in blast radius
    for (int blastRadius = -1; blastRadius <= 1; blastRadius++) {
      if (playerX == bombX + blastRadius && playerY == bombY) {
        noLives--;
        break;
      }
      if (playerY == bombY + blastRadius && playerX == bombX) {
        noLives--;
        break;
      }
    }
  }

  bombPlaced = false;
  bombX = 9;
  bombY = 9;
}

// move player based on joystick dirrection
void movePlayer(byte direction) {

  if (direction == 4)
    return;

  // move up
  if (direction == 0 && playerX > 0 && mapMatrix[playerX - 1][playerY] == 0) {
    playerX--;
    return;
  }

  // move down
  if (direction == 1 && playerX < matrixSize - 1 && mapMatrix[playerX + 1][playerY] == 0) {
    playerX++;
    return;
  }

  // move left
  if (direction == 2 && playerY > 0 && mapMatrix[playerX][playerY - 1] == 0) {
    playerY--;
    return;
  }

  // move right
  if (direction == 3 && playerY < matrixSize - 1 && mapMatrix[playerX][playerY + 1] == 0) {
    playerY++;
  }
}

byte joyDirection(int xAxisValue, int yAxisValue, int minThreshold, int maxThreshold, bool *joyMoved) {
  // return 0 for up cursorMovement
  if (yAxisValue < minThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 0;
  }

  // return 1 for down cursorMovement
  if (yAxisValue > maxThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 1;
  }

  // return 2 for left cursorMovement
  if (xAxisValue < minThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 2;
  }

  // return 3 for right cursorMovement
  if (xAxisValue > maxThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 3;
  }

  // return 4 if the joystick didn't move
  if (xAxisValue >= minThreshold && xAxisValue <= maxThreshold && yAxisValue <= maxThreshold && yAxisValue >= minThreshold) {
    *joyMoved = false;
    return 4;
  }
}

bool debounceInput(int pin, byte *lastReading, unsigned long *lastDebounceTime, unsigned long debounceDelay) {
  bool pressed = false;
  int reading = digitalRead(pin);

  if (reading != *lastReading) {
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      pressed = true;
    }
  }

  *lastReading = reading;
  return pressed;
}

void welcomeMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("## WELCOME TO ###");
  lcd.setCursor(0, 1);
  lcd.write("## BOMBERMAN! ##");
  delay(3000);
  lcd.clear();
}

void printMenu(byte menuOption) {
  switch (menuOption) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Start Game");
      lcd.write((uint8_t)5);
      lcd.write("    ");
      lcd.write((uint8_t)0);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Settings");
      lcd.write((uint8_t)5);
      lcd.write("      ");
      lcd.write((uint8_t)1);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("About");
      lcd.write((uint8_t)5);
      lcd.write("         ");
      lcd.write((uint8_t)1);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 4:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Highscores");
      lcd.write((uint8_t)5);
      lcd.write("    ");
      lcd.write((uint8_t)1);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 5:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Reset");
      lcd.write("          ");
      lcd.write((uint8_t)1);
      lcd.setCursor(0, 1);
      lcd.write("Highscores");
      lcd.write((uint8_t)5);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 6:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("How to play");
      lcd.write((uint8_t)5);
      lcd.write("   ");
      lcd.write((uint8_t)1);
      displayImage(matrixImages[menuOption - 1]);
      break;
    case 7:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Difficulty");
      lcd.write((uint8_t)5);
      lcd.write("    ");
      lcd.write((uint8_t)2);
      displayImage(matrixImages[menuOption - 1]);
      break;
    default:
      break;
  }
}

void printSettings(byte settingsOption) {

  switch (settingsOption) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back          ");
      lcd.write((uint8_t)0);
      lcd.setCursor(0, 1);
      lcd.write("LCD Brightness");
      lcd.write((uint8_t)5);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back   Matrix ");
      lcd.write((uint8_t)1);
      lcd.setCursor(0, 1);
      lcd.write("    Brightness");
      lcd.write((uint8_t)5);
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back          ");
      lcd.write((uint8_t)1);
      lcd.setCursor(0, 1);
      lcd.write("Sounds");
      lcd.write((uint8_t)5);
      break;
    case 4:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back          ");
      lcd.write((uint8_t)2);
      lcd.setCursor(0, 1);
      lcd.write("Player Name   ");
      lcd.write((uint8_t)5);
      break;
    default:
      break;
  }
}

void printSettingsInput(byte settingsOption) {
  switch (settingsOption) {
    case 1:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      lcd.write("Brightness: ");
      char lcdBrightnessChar[2];
      lcd.write(itoa(lcdBrightness, lcdBrightnessChar, 10));
      if (lcdBrightness == 1)
        lcd.write((uint8_t)2);
      else if (lcdBrightness == 10)
        lcd.write((uint8_t)0);
      else
        lcd.write((uint8_t)1);
      break;
    case 2:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      lcd.write("Brightness: ");
      char matrixBrightnessChar[2];
      lcd.write(itoa(matrixBrightness, matrixBrightnessChar, 10));
      if (matrixBrightness == 1)
        lcd.write((uint8_t)2);
      else if (matrixBrightness == 15)
        lcd.write((uint8_t)0);
      else
        lcd.write((uint8_t)1);
      break;
    case 3:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      lcd.write("Sound: ");
      if (soundOn) {
        lcd.write("ON");
        lcd.write((uint8_t)0);
      } else {
        lcd.write("OFF");
        lcd.write((uint8_t)2);
      }
      break;
    case 4:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.write("  ");
      for (byte i = 0; i < namePosition; i++) {
        lcd.write(" ");
      }
      lcd.write((uint8_t)0);
      lcd.setCursor(0, 1);
      lcd.write("Name:  ");
      for (byte i = 0; i < 3; i++) {
        lcd.write(playerName[i]);
      }
      lcd.write(" ");
      lcd.write((uint8_t)1);
      break;
    default:
      break;
  }
}
void menuActions(byte menuOption) {
  switch (menuOption) {
    case 1:
      inGame = true;
      inMenu = false;
      noLives = 4 - difficulty;
      bombTimer = 4000 - 1000 * difficulty;
      bombBlinkInterval = 120 - 20 * difficulty;
      renderMap();
      break;
    case 2:
      inSettings = true;
      inMenu = false;
      lcd.clear();
      printSettings(settingsPosition);
      break;
    case 3:
      inAbout = true;
      inMenu = false;
      break;
    case 4:
      readIntArrayFromEEPROM(highScoresMemoryAddr, highscores, 5);
      readHighscoreNames(highScoreNamesMemoryAddr, highscoreNames, 5);
      inHighscores = true;
      inMenu = false;
      printHighscore(0);
      break;
    case 5:
      inResetHighscores = true;
      inMenu = false;
      printResetHighscores();
      break;
    case 6:
      inMenu = false;
      inHowTo = true;
      printHowTo();
      break;
    case 7:
      inDifficulty = true;
      inMenu = false;
      printDifficulty(difficulty);
      break;
    default:
      break;
  }
}

void printGameStats(byte noLives, unsigned long playTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  char scoreChar[4];
  lcd.write(itoa(score, scoreChar, 10));
  lcd.setCursor(13, 0);
  for (int i = 0; i < noLives; i++)
    lcd.write((uint8_t)3);
  lcd.setCursor(0, 1);
  lcd.write("Time: ");
  char playTimeChar[4];
  lcd.write(itoa(playTime, playTimeChar, 10));
}

bool playerDead() {
  if (noLives == 0)
    return true;

  return false;
}

bool playerWin() {
  for (byte i = 0; i < matrixSize; i++)
    for (byte j = 0; j < matrixSize; j++)
      if (mapMatrix[i][j] == 1)
        return false;

  return true;
}
void printGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("## GAME OVER! ##");
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)4);
  lcd.write("EXIT   RESTART");
  lcd.write((uint8_t)5);
}
void printWin() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("### YOU WIN! ###");
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)4);
  lcd.write("EXIT   RESTART");
  lcd.write((uint8_t)5);
}

void resetGame() {
  inGame = false;
  inMenu = true;
  generateMap();
  lcd.clear();
  playerX = 3;
  playerY = 3;
  noLives = 4 - difficulty;
  bombTimer = 4000 - 1000 * difficulty;
  bombBlinkInterval = 120 - 20 * difficulty;
  playTime = 0;
  score = 0;
  printMenu(menuPosition);
}

void writeIntArrayIntoEEPROM(int address, int numbers[], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++) {
    EEPROM.write(addressIndex, numbers[i] >> 8);
    EEPROM.write(addressIndex + 1, numbers[i] & 0xFF);
    addressIndex += 2;
  }
}
void readIntArrayFromEEPROM(int address, int numbers[], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++) {
    numbers[i] = (EEPROM.read(addressIndex) << 8) + EEPROM.read(addressIndex + 1);
    addressIndex += 2;
  }
}
void writeHighscoreNames(int address, char names[][3], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++)
    for (int j = 0; j < 3; j++) {
      EEPROM.write(addressIndex, names[i][j]);
      addressIndex += 1;
    }
}
void readHighscoreNames(int address, char names[][3], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++)
    for (int j = 0; j < 3; j++) {
      names[i][j] = EEPROM.read(addressIndex);
      addressIndex += 1;
    }
}

void printHighscore(byte place) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)4);
  lcd.write("Back");
  lcd.setCursor(0, 1);
  char placeChar[1];
  char scoreChar[4];
  lcd.write(itoa(place + 1, placeChar, 10));
  lcd.write(". ");
  for (byte i = 0; i < 3; i++) {
    lcd.write(highscoreNames[place][i]);
  }
  lcd.write(" - ");
  lcd.write(itoa(highscores[place], scoreChar, 10));
  lcd.setCursor(15, 1);
  if (place == 4)
    lcd.write((uint8_t)2);
  else if (place == 0)
    lcd.write((uint8_t)0);
  else
    lcd.write((uint8_t)1);
}

int updateHighscore(int score, char playerName[3]) {

  int newPlace = 10;

  for (int i = 0; i < 5; i++) {
    if (score >= highscores[i]) {
      newPlace = i;
      break;
    }
  }

  if (newPlace != 10) {
    for (int i = 4; i > newPlace; i--) {
      highscores[i] = highscores[i - 1];
      strncpy(highscoreNames[i], highscoreNames[i - 1], 3);
    }
    highscores[newPlace] = score;
    for (int i = 0; i < 3; i++) {
      strncpy(highscoreNames[newPlace], playerName, 3);
    }
  }

  return newPlace;
}

void saveHighscores() {
  writeIntArrayIntoEEPROM(highScoresMemoryAddr, highscores, 5);
  writeHighscoreNames(highScoreNamesMemoryAddr, highscoreNames, 5);
}

void resetHighScores() {
  for (int i = 0; i < 5; i++) {
    highscores[i] = 0;
    strncpy(highscoreNames[i], "XXX", 3);
  }

  writeIntArrayIntoEEPROM(highScoresMemoryAddr, highscores, 5);
  writeHighscoreNames(highScoreNamesMemoryAddr, highscoreNames, 5);
}

void printResetHighscores() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("Are you sure?");
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)4);
  lcd.write("NO");
  lcd.write("         ");
  lcd.write("YES");
  lcd.write((uint8_t)5);
}

void printHowTo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)4);
  lcd.write("Back ");
  lcd.write("Move: ");
  lcd.write((uint8_t)4);
  lcd.write((uint8_t)1);
  lcd.write((uint8_t)5);
  lcd.setCursor(0, 1);
  lcd.write("Place Bomb:click");
}

void printDifficulty(byte difficulty) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)4);
  lcd.write("Back");
  lcd.setCursor(0, 1);
  lcd.write("Set: ");
  switch (difficulty) {
    case 1:
      lcd.write("low");
      lcd.write((uint8_t)2);
      break;
    case 2:
      lcd.write("med");
      lcd.write((uint8_t)1);
      break;
    case 3:
      lcd.write("high");
      lcd.write((uint8_t)0);
      break;
    default:
      break;
  }
}

void printScore() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("Score: ");
  char scoreChar[4];
  lcd.write(itoa(score, scoreChar, 10));
}

void printScoreBeat(int newPlace) {
  lcd.setCursor(0, 1);
  lcd.write("Score Beaten: ");
  char placeChar = '0';
  placeChar = placeChar + newPlace + 1;
  lcd.write(placeChar);
}

void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, bitRead(row, j));
    }
  }
}

void navigateSettings() {
  delay(1);
  if (currentMovement != previousMovement) {
    if (currentMovement == 2) {
      lcd.clear();
      printMenu(menuPosition);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inMenu = true;
    } else if (currentMovement == 3) {
      lcd.clear();
      printSettingsInput(settingsPosition);
      inSettingsInput = true;
      inSettings = false;
      inAbout = false;
    }
    if (currentMovement == 1 && settingsPosition < 4) {
      settingsPosition++;
    } else if (currentMovement == 0 && settingsPosition > 1) {
      settingsPosition--;
    }

    if (settingsPosition != previousSettingsPosition) {
      printSettings(settingsPosition);
      previousSettingsPosition = settingsPosition;
    }
    previousMovement = currentMovement;
  }
}

void printAbout() {
  if (currentMovement == 2) {
    inAbout = false;
    inMenu = true;
    lcd.clear();
    printMenu(menuPosition);
  } else {
    if (millis() - previousMillis > lcdScrollInterval) {
      previousMillis = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write(" ");  // to ignore scroll
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      for (byte i = aboutStringPos; i <= aboutStringPos + lcdLineWidth; i++)
        lcd.print(aboutString[i]);
      aboutStringPos++;
      if (aboutStringPos > aboutStringLen - lcdLineWidth)  // wrap string
        aboutStringPos = 0;
      lcd.scrollDisplayLeft();
    }
  }
}

void handleSettingsInput() {
  if (currentMovement == 2 && settingsPosition != 4) {
    inSettingsInput = false;
    inAbout = false;
    inSettings = true;
    inMenu = false;
    lcd.clear();
    printSettings(settingsPosition);
    EEPROM.put(lcdBrightnessMemoryAddr, lcdBrightness);
    EEPROM.put(matrixBrightnessMemoryAddr, matrixBrightness);
    EEPROM.put(soundStateMemoryAddr, soundOn);
    EEPROM.put(playerNameMemoryAddr, playerName);
  }
  if (currentMovement != previousMovement) {
    if (settingsPosition == 1) {
      if (currentMovement == 0 && lcdBrightness < 10) {
        lcdBrightness++;
      } else if (currentMovement == 1 && lcdBrightness > 1) {
        lcdBrightness--;
      }

      if (lcdBrightness != previousLcdBrightness) {
        printSettingsInput(settingsPosition);
        previousLcdBrightness = lcdBrightness;
        analogWrite(lcdBacklight, lcdBrightness * 25);
      }
    } else if (settingsPosition == 2) {
      if (currentMovement == 0 && matrixBrightness < 15) {
        matrixBrightness++;
      } else if (currentMovement == 1 && matrixBrightness > 1) {
        matrixBrightness--;
      }

      if (matrixBrightness != previousMatrixBrightness) {
        printSettingsInput(settingsPosition);
        previousMatrixBrightness = matrixBrightness;
        lc.setIntensity(0, matrixBrightness);
      }
    } else if (settingsPosition == 3) {
      if (currentMovement == 0 && soundOn == false) {
        soundOn = true;
      } else if (currentMovement == 1 && soundOn == true) {
        soundOn = false;
      }

      if (soundOn != previousSoundState) {
        printSettingsInput(settingsPosition);
        previousSoundState = soundOn;
      }
    } else if (settingsPosition == 4) {
      if (currentMovement == 2) {
        if (namePosition > 0) {
          namePosition--;
          printSettingsInput(settingsPosition);
        } else {
          // return to first setting
          settingsPosition = 1;
        }

      } else if (currentMovement == 3 && namePosition < 2) {
        namePosition++;
        printSettingsInput(settingsPosition);
      } else if (currentMovement == 0 && playerName[namePosition] < 'Z') {
        playerName[namePosition]++;
      } else if (currentMovement == 1 && playerName[namePosition] > 'A') {
        playerName[namePosition]--;
      }
      if (strcmp(playerName, previousName)) {
        printSettingsInput(settingsPosition);
        strncpy(previousName, playerName, 3);
      }
    }
    previousMovement = currentMovement;
  }
}

void navigateMenu() {
  delay(1);
  if (currentMovement != previousMovement) {
    if (currentMovement == 1 && menuPosition < 7) {
      menuPosition++;
    } else if (currentMovement == 0 && menuPosition > 1) {
      menuPosition--;
    }

    if (menuPosition != previousMenuPosition) {
      printMenu(menuPosition);
      previousMenuPosition = menuPosition;
    }
    previousMovement = currentMovement;
  }

  if (currentMovement == 3) {
    menuActions(menuPosition);
  }
}

void navigateHighscores() {
  delay(1);
  if (currentMovement != previousMovement) {
    if (currentMovement == 2) {
      lcd.clear();
      printMenu(menuPosition);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inHighscores = false;
      inMenu = true;
    } else if (currentMovement == 1 && highscorePosition < 4) {
      highscorePosition++;
    } else if (currentMovement == 0 && highscorePosition > 0) {
      highscorePosition--;
    }

    if (highscorePosition != previousHighscorePosition) {
      printHighscore(highscorePosition);
      previousHighscorePosition = highscorePosition;
    }
  }
  previousMovement = currentMovement;
}

void handleHighscoreReset() {
  if (currentMovement != previousMovement) {
    if (currentMovement == 2) {
      lcd.clear();
      printMenu(menuPosition);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inHighscores = false;
      inResetHighscores = false;
      inMenu = true;

    } else if (currentMovement == 3) {
      resetHighScores();
      lcd.clear();
      printMenu(menuPosition);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inHighscores = false;
      inResetHighscores = false;
      inMenu = true;
      menuPosition = 4;
    }
  }
  previousMovement = currentMovement;
}

void handleHowTo() {
  if (currentMovement != previousMovement) {
    if (currentMovement == 2) {
      lcd.clear();
      printMenu(menuPosition);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inHighscores = false;
      inResetHighscores = false;
      inHowTo = false;
      inMenu = true;
    }
  }
  previousMovement = currentMovement;
}

void handleDifficultyChange() {
  if (currentMovement != previousMovement) {
    if (currentMovement == 2) {
      lcd.clear();
      printMenu(menuPosition);
      EEPROM.put(difficultyMemoryAddr, difficulty);
      inSettings = false;
      inSettingsInput = false;
      inAbout = false;
      inHighscores = false;
      inResetHighscores = false;
      inHowTo = false;
      inDifficulty = false;
      inMenu = true;
    } else if (currentMovement == 0 && difficulty < 3) {
      difficulty++;
    } else if (currentMovement == 1 && difficulty > 1) {
      difficulty--;
    }

    if (difficulty != previousDifficulty) {
      printDifficulty(difficulty);
      previousDifficulty = difficulty;
      noLives = 4 - difficulty;
      bombTimer = 4000 - 1000 * difficulty;
      bombBlinkInterval = 120 - 20 * difficulty;
    }
  }
  previousMovement = currentMovement;
}

void handleGameLogic() {
  currentMillis = millis();
  if (currentMillis - previousMillis > 1000) {  // count seconds
    playTime++;
    playTime %= 1000;
    previousMillis = millis();
    printGameStats(noLives, playTime);
  }

  if (noLives != previousLives) {
    printGameStats(noLives, playTime);
    previousLives = noLives;
  }

  if (joyButtonState) {
    placeBomb(playerX, playerY);
  }

  if (currentMovement != previousMovement) {
    movePlayer(currentMovement);
    previousMovement = currentMovement;
    renderMap();
  }

  if (millis() - lastPlayerBlink > playerBlinkInterval) {
    playerBlinkState = !playerBlinkState;
    lastPlayerBlink = millis();
  }

  // we render the bomb only when one is placed
  if (bombPlaced && (millis() - lastBombBlink > bombBlinkInterval)) {
    bombBlinkState = !bombBlinkState;
    lastBombBlink = millis();
    renderBomb();
  }

  if (bombBlinkState && soundOn) {
    tone(buzzerPin, bombTone);
  } else {
    noTone(buzzerPin);
  }

  // we render the map only when a change to the walls happens
  if (bombPlaced && (millis() - bombPlacedTime > bombTimer)) {
    detonateBomb();
    renderMap();
  }

  renderPlayer();

  if (playerDead() || playerWin()) {
    // save highscore only when score is on leaderboard
    bombBlinkState = false;
    noTone(buzzerPin);
    printScore();
    int newPlace = updateHighscore(score, playerName);
    if (newPlace != 10) {
      saveHighscores();
      printScore();
      printScoreBeat(newPlace);
    }
    delay(3000);

    if (playerWin()) {
      printWin();
      inEndGameScreen = true;
      inGame = false;
      inMenu = false;
    }

    else if (playerDead()) {
      printGameOver();
      inEndGameScreen = true;
      inGame = false;
      inMenu = false;
    }
  }
}