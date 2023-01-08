#include "SPI.h"
#include "NRFLite.h"

// Configuration static variable
const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;
const static uint8_t PIN_RELAY = 2;
const static uint8_t PIN_LED_ERROR = 3;

// Define RadioPacketType for determine what reciver need to do when will recive message from transmiter
enum RadioPacketType {
  TurnOn,
  TurnOff,
};

// Create struct for diffrent needed data
struct RadioPacket {
  RadioPacketType PacketType;
  uint8_t FromRadioId;
  uint32_t OnTimeMillis;
};

// Global variables and objects
NRFLite _radio;
uint32_t _lastGetData;

void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_ERROR, OUTPUT);

  digitalWrite(PIN_LED_ERROR, HIGH);
  
  // Communicate with radio
  if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN)) {
    Serial.println("Problem polaczenia z radiem");
    digitalWrite(PIN_LED_ERROR, LOW);
    while (1);
  }
}

void loop() {

  // Show any received data.
  checkRadio();
  if(millis() - _lastGetData > 59999) {
    setRelayState(false);
    digitalWrite(PIN_LED_ERROR, LOW);
    delay(500);
    digitalWrite(PIN_LED_ERROR, HIGH);
    delay(500);
  }
}


void checkRadio() {
  while (_radio.hasData())  // 'hasData' puts the radio into Rx mode.
  {
    RadioPacket radioData;
    _radio.readData(&radioData);

    if (radioData.PacketType == TurnOn) {
      sendToSerial(radioData, "Turn on from ");
      setRelayState(true);
    } else if (radioData.PacketType == TurnOff) {
      sendToSerial(radioData, "Turn of from ");
      setRelayState(false);
    }
    _lastGetData = millis();
  }
}

// Serial for radioData
void sendToSerial(RadioPacket radioData, String msg) {
  msg += radioData.FromRadioId;
  msg += ", ";
  msg += radioData.OnTimeMillis;
  msg += " ms";
  Serial.println(msg);
}

// Set relay
void setRelayState(bool state) {
  if(state) digitalWrite(PIN_RELAY, HIGH);
  else if(!state) digitalWrite(PIN_RELAY, LOW);
}
