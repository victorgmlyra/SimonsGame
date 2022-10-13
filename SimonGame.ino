#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"

// Wifi and Azure Configs
#define WIFI_SSID "Pixel_2704"
#define WIFI_PASSWORD "123456789"
const char* SCOPE_ID = "0ne007D56E7";
const char* DEVICE_ID = "yed4c6sehx";
const char* DEVICE_KEY = "+rGkkOXQWUX3rTPAt8zfJRBLq47giNTubU8ClUKDUvE=";

// Push-buttons definitions
#define RED_BUTTON D3 
#define GREEN_BUTTON D4
#define YELLOW_BUTTON D9
#define BLUE_BUTTON D10
// LEDs definitions
#define RED_LED D5
#define GREEN_LED D6
#define YELLOW_LED D7
#define BLUE_LED D8

#define BUZZER D0

// Functions Definitions
void menu();
void gameMode1();
void gameMode2();
void gameMode3();
void win();
void lose();
void playAgain();
void generateSequence();
void showSequence();
void inputSequence();
int listenInput(int LEDdelay);
void lightLed(int ledNum, int LEDdelay);
void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
void sendTelemetry(char* var_msg, int value);
#include "src/connection.h"

// Variables
uint8_t LEDs[] = {RED_LED, GREEN_LED, YELLOW_LED, BLUE_LED};
uint8_t BUTTONS[] = {RED_BUTTON, GREEN_BUTTON, YELLOW_BUTTON, BLUE_BUTTON};
int buttonState[] = {0, 0, 0, 0};         // variable for reading the pushbutton status
int lastState[] = {0, 0, 0, 0};
int sounds[] = {1046, 880, 659, 523};

int sequence[31];
int gameMode = 0; // 1, 2 or 3
int gameState = 0;
int lenSeq = 8;   // 8, 14, 20, 31
int inputSeqNum = 0;
int currLen = 0;
int maxLen = 0;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // LCD setup
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to WIFI...");

  // WIFI and Azure
  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  if (context != NULL) {
    lastTick = 0;  // set timer in the past to enable first telemetry a.s.a.p
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  if (isConnected)
    lcd.print("Connected!");
  else
    lcd.print("Not Connected :(");
  delay(10000);

  // Setup Push-buttons
  pinMode(RED_BUTTON, INPUT_PULLUP);
  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  pinMode(YELLOW_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_BUTTON, INPUT_PULLUP);

  // Setup LEDs
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(YELLOW_LED, LOW);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW);  

  // Buzzer
  delay(100);
  pinMode (BUZZER, OUTPUT);
  noTone(BUZZER);

  lcd.clear();
}

void loop() {

  switch (gameMode) {
    case 0:
      menu();
      break;
    case 1:
      gameMode1();
      break;
    case 2:
      gameMode2();
      break;
    case 3:
      gameMode3();
      break;
    case 4:
      win();
      break;
    case 5:
      lose();
      break;
    case 6:
      playAgain();
    default:
      // Serial.println("Not a gameMode.");
      break;
  }
}

/********************
    GAME FUNCTIONS
********************/

void gameMode1() 
{
  switch (gameState) {
    case 0:
      generateSequence();
      gameState++;
      break;
    case 1:
      delay(800);
      showSequence();
      gameState++;
      break;
    case 2:
      inputSequence();
      break;
    case 3:
      gameState = 1;
      if (currLen == lenSeq){
        gameMode = 4;
        gameState = 0;
      }
      break;
  }
  delay(100);
}

void gameMode2() 
{
  switch (gameState) {
    case 0:
      sequence[0] = random(0, 4);
      gameState++;
      break;
    case 1:
      delay(800);
      showSequence();
      gameState++;
      break;
    case 2:
      inputSequence();
      break;
    case 3: 
      if (currLen == lenSeq)
        gameMode = 4;
      else
        gameState++;
    case 4:
      {
        int buttonPressed = listenInput(200);
        if (buttonPressed != -1) {
          sequence[currLen] = buttonPressed;
          gameState = 1;
        }
      }
      break;
  }
}

void gameMode3() 
{
  // TODO
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Not Implemented");
  delay(2000);
  gameMode = 0;
  lcd.clear();
}

void win()
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("You Won!!");
  lcd.setCursor(2, 1);
  lcd.print("Play Again?");

  gameMode = 6;
}

void lose()
{
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("You Lost :(");
  lcd.setCursor(3, 1);
  lcd.print("Try Again?");

  gameMode = 6;
}

void playAgain()
{
  if (currLen > maxLen)
    maxLen = currLen;
  sendTelemetry("Max_length", maxLen);
  int buttonPressed = listenInput(300);
  if (buttonPressed == 1){
    gameMode = 0;
    gameState = 0;
    currLen = 0;
    inputSeqNum = 0;
    lcd.clear();
  }
  delay(100);
}

void generateSequence()
{
  randomSeed(millis()); //in this way is really random!!!
  for(int i = 0; i < lenSeq; i++) {
    sequence[i] = random(0, 4);
  }
}

void showSequence()
{
  currLen++;
  lcd.setCursor(13, 0);
  lcd.print(currLen);
  sendTelemetry("Curr_length", currLen);
  for(int i = 0; i < currLen; i++){
    lightLed(sequence[i], 400);
    delay(100);
  }
}

void inputSequence()
{
  int buttonPressed = listenInput(300); 
  if (buttonPressed != -1){
    if (buttonPressed == sequence[inputSeqNum]) {
      inputSeqNum++;
    } else{
      inputSeqNum = 0;
      gameState = 0;
      gameMode = 5;
      return;
    }
  }

  if (inputSeqNum == currLen){
    inputSeqNum = 0;
    gameState++;
  }
}

/********************
    INTERFACE FUNCTIONS
********************/

void menu() 
{
  int tempGameMode = 0;
  switch (gameState) {
    case 0:
      tempGameMode = 1;
      lenSeq = 8;
      lcd.setCursor(0, 0);
      lcd.print("Game 1:");
      lcd.setCursor(4, 1);
      lcd.print("8 Colours");
      break;
    case 1:
      tempGameMode = 1;
      lenSeq = 14;
      lcd.setCursor(0, 0);
      lcd.print("Game 1:");
      lcd.setCursor(4, 1);
      lcd.print("14 Colours");
      break;
    case 2:
      tempGameMode = 1;
      lenSeq = 20;
      lcd.setCursor(0, 0);
      lcd.print("Game 1:");
      lcd.setCursor(3, 1);
      lcd.print("20 Colours");
      break;
    case 3:
      tempGameMode = 1;
      lenSeq = 31;
      lcd.setCursor(0, 0);
      lcd.print("Game 1:");
      lcd.setCursor(3, 1);
      lcd.print("31 Colours");
      break;
    case 4:
      tempGameMode = 2;
      lenSeq = 31;
      lcd.setCursor(0, 0);
      lcd.print("Game 2:");
      lcd.setCursor(2, 1);
      lcd.print("Multiplayer");
      break;
    case 5:
      tempGameMode = 3;
      lenSeq = 31;
      lcd.setCursor(0, 0);
      lcd.print("Game 3:");
      lcd.setCursor(2, 1);
      lcd.print("Multiplayer");
      break;
  }

  int buttonPressed = listenInput(100);
  if (buttonPressed == 2){
    lcd.clear();
    gameState -= 1;
  }
  if (buttonPressed == 3){
    lcd.clear();
    gameState += 1;
  }
  if (buttonPressed == 1){
    lcd.setCursor(9, 0);
    lcd.print("SEQ:");
    gameMode = tempGameMode;
    gameState = 0;
  }

  if (gameState > 5)
    gameState = 5;
  if (gameState < 0)
    gameState = 0;

  delay(50);
}

int listenInput(int LEDdelay)
{
  for (int ledNum = 0; ledNum < 4; ledNum++){
    buttonState[ledNum] = digitalRead(BUTTONS[ledNum]);
    if(lastState[ledNum] == HIGH && buttonState[ledNum] == LOW){
      lightLed(ledNum, LEDdelay);
      lastState[ledNum] = buttonState[ledNum];
      return ledNum;
    }
    lastState[ledNum] = buttonState[ledNum];
  }
  return -1;
}

void lightLed(int ledNum, int LEDdelay)
{
  digitalWrite(LEDs[ledNum], HIGH);
  tone(BUZZER, sounds[ledNum]);
  delay(LEDdelay);
  digitalWrite(LEDs[ledNum], LOW);
  noTone(BUZZER);
}

/********************************
    WIFI AND AZURE FUNCTIONS
********************************/

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}

void sendTelemetry(char* var_msg, int value)
{
  if (isConnected) {
    char msg[64] = {0};
    int pos = 0, errorCode = 0;

    pos = snprintf(msg, sizeof(msg) - 1, "{\"%s\": %d}", var_msg, value);
    // else
    //   pos = snprintf(msg, sizeof(msg) - 1, "{\"Max_length\": %d}", value);
    errorCode = iotc_send_telemetry(context, msg, pos);

    msg[pos] = 0;

    if (errorCode != 0) {
      LOG_ERROR("Sending message has failed with error code %d", errorCode);
    }

    iotc_do_work(context);  // do background work for iotc
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }
}
