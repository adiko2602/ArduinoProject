#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>
#include "NRFLite.h"

// Configure static variable
const static uint8_t RADIO_ID = 0;
const static uint8_t DESTINATION_RADIO_ID = 1;
const static uint8_t PIN_RADIO_CE = 2;
const static uint8_t PIN_RADIO_CSN = 3;
const static uint8_t BTN_UP = 6;
const static uint8_t BTN_DOWN = 7;
const static uint8_t LCD_ADDRESS = 0x27;
const static uint8_t TEMP_SENSOR_ADDRESS = 0x76;
const static float HYSTERESIS_TRESHOLD = 0.2;
const static float TEMP_SET_STEP = 0.2;

// Define special character for lcd
byte _heating[8] = {
  B00000,
  B00100,
  B00100,
  B00110,
  B01111,
  B11111,
  B11111,
  B01110
};

// Using for send command data
enum RadioPacketType {
  TurnOn,
  TurnOff,
};

struct RadioPacket {
  RadioPacketType PacketType;
  uint8_t FromRadioId;
  uint32_t OnTimeMillis;
};

// Global variables and objects
NRFLite _radio;
LiquidCrystal_I2C _lcd(LCD_ADDRESS, 16, 2);
Adafruit_BMP280 bmp;

uint32_t _lastRelayStateSendTime;
uint32_t _lastLcdUpdate;

float _setTemperature = 23.0;
bool _relayState = false;
uint8_t _communicationError = 0;

void setup() {
  Serial.begin(9600);

  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);

  // Communicate with NRF24L01
  if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN)) {
    Serial.println("Blad polaczenia z radiem");
    while (1);
  }

  // Comunicate with BMP280
  if (!bmp.begin(TEMP_SENSOR_ADDRESS)) {
    Serial.println("Problem z polaczeniem termometru");
    while (1);
  }

  // Initialize lcd
  _lcd.init();
  _lcd.backlight();
  _lcd.createChar(0, _heating);
  _lcd.setCursor(0, 0);
  lcdUpdate();
}

void loop() {
  // check if keybord key is pressed
  switch (checkKeyboard()) {
    case 1:
      // Set temperature higher
      setTemperature(TEMP_SET_STEP);
      break;
    case 2:
      // Set temperature lower
      setTemperature(TEMP_SET_STEP * -1.0);
      break;
    default:
      break;
  }

  // Send a command for relay state every 10 seconds.
  if (millis() - _lastRelayStateSendTime > 9999) {
    checkState();
    if (_relayState) {
      sendTurnOn();
    } else {
      sendTurnOff();
    }
  }

  // Show any received data.
  checkRadio();

  // Check if is communication problem
  if (_communicationError > 6) {
    showCommunicationError(_communicationError);
  } else {
    if (millis() - _lastLcdUpdate > 999) {
      lcdUpdate();
      _lastLcdUpdate = millis();
    }
  }
}

// Send turn on state to reciver
void sendTurnOn() {
  Serial.println("Wysylanie wiadomosci wlacz...");

  RadioPacket radioData;
  radioData.PacketType = TurnOn;
  radioData.FromRadioId = RADIO_ID;
  radioData.OnTimeMillis = millis();

  if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData)))  
  {
    Serial.println("Odbiornik odpowiedzial na wiadomosc");
    _communicationError = 0;
  } else {
    _communicationError++;
    Serial.println("Odbiornik nie odpowiedzial na wiadomosc");
  }

  _lastRelayStateSendTime = millis();
}

// Send turn off state to reciver
void sendTurnOff() {
  Serial.println("Wysylanie wiadomosci wylacz...");

  RadioPacket radioData;
  radioData.PacketType = TurnOff;
  radioData.FromRadioId = RADIO_ID;
  radioData.OnTimeMillis = millis();

  if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData))) 
  {
    Serial.println("Odbiornik odpowiedzial na wiadomosc");
    _communicationError = 0;
  } else {
    _communicationError++;
    Serial.println("Odbiornik nie odpowiedzial na wiadomosc");
  }

  _lastRelayStateSendTime = millis();
}

void checkRadio() {
  while (_radio.hasData()) 
  {
    RadioPacket radioData;
    _radio.readData(&radioData);

    sendToSerial(radioData, "Odbiornik wyslal: ");
  }
}

void sendToSerial(RadioPacket radioData, String msg) {
  msg += radioData.FromRadioId;
  msg += ", ";
  msg += radioData.OnTimeMillis;
  msg += " ms";
  Serial.println(msg);
}

uint8_t checkKeyboard() {
  delay(200);
  if (!digitalRead(BTN_UP)) return 1;
  if (!digitalRead(BTN_DOWN)) return 2;
  return 0;
}

// Check if is need to turn on or off heating
void checkState() {
  float actualTemp = bmp.readTemperature();
  float hysteresis[] = { _setTemperature - HYSTERESIS_TRESHOLD, _setTemperature + HYSTERESIS_TRESHOLD };

  if (actualTemp < hysteresis[1] && actualTemp > hysteresis[0]) _relayState = _relayState;
  else if (actualTemp < hysteresis[0]) _relayState = true;
  else if (actualTemp > hysteresis[1]) _relayState = false;
  lcdUpdate();
}

// Set disired temperature
void setTemperature(float temp) {
  if (_setTemperature > 10.0 && _setTemperature < 32.0)
    _setTemperature = _setTemperature + temp;
  lcdUpdate();
}

// Update lcd things
void lcdUpdate() {
  String actualTemp = "Act temp: ";
  String setTemp = "Set temp: ";
  actualTemp += String(bmp.readTemperature(), 1);
  setTemp += String(_setTemperature, 1);

  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print(actualTemp);
  _lcd.setCursor(0, 1);
  _lcd.print(setTemp);
  if (_relayState) {
    _lcd.setCursor(15, 1);
    _lcd.write(byte(0));
  }
}

void showCommunicationError(uint8_t errors) {
  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print("Connection err");
  _lcd.setCursor(0, 1);
  _lcd.print("num of err ");
  _lcd.print(errors);
}
