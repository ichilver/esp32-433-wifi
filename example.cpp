
#include <Arduino.h>

#include "main.h"

// ########################################################################################
//
// Delcare and initialise the various classes
//
// Initialize variables
volatile PULSE rawBitBuffer[RAWBITBUFFERSIZE];
volatile unsigned int rawBitBufferPosition  = 0;

volatile byte baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;


void IRAM_ATTR qam_rx4_isr() {
  static unsigned int prevTime = 0; // lastTime
  unsigned int newTime = micros();  // now
  //unsigned int newTime = ESP.getCycleCount()/240;

  // Calculate the duration of the current signal state
  unsigned int duration = newTime - prevTime;

  bool state = digitalRead(35);

  // If we find the preample (4.5ms < x < 12.5ms) trigger baseProtocolState to receiving mode
  if ((duration > BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MIN) && (duration < BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MAX) ) {
    if (baseProtocolState == BASEPROTOCOL433_RX_RECEIVEMODE) {
        baseProtocolState = BASEPROTOCOL433_RX_MESSAGERECEIVED;
        detachInterrupt(35);
    } else {
        baseProtocolState = BASEPROTOCOL433_RX_RECEIVEMODE;
    }
  }

	if (baseProtocolState == BASEPROTOCOL433_RX_RECEIVEMODE) {
		// Store the duration of the received pulse
        rawBitBuffer[rawBitBufferPosition].state = !state;
        rawBitBuffer[rawBitBufferPosition].duration = duration;
        rawBitBufferPosition++;
	}

	if (rawBitBufferPosition == (RAWBITBUFFERSIZE - 1)) {
		baseProtocolState =BASEPROTOCOL433_RX_LISTENMODE;
		rawBitBufferPosition = 0;
	}

  prevTime = newTime;

}
#endif


void setup() {
  Serial.begin(115200);

  rawBitBufferPosition = 0;
  baseProtocolState = BASEPROTOCOL433_SETUP;

  // Configure QAM-RX4 data pin as an input pin
  pinMode(35, INPUT);

  Serial.println("Setup() done");

}

void loop() {
    if (baseProtocolState == BASEPROTOCOL433_SETUP) {
        attachInterrupt(digitalPinToInterrupt(35), qam_rx4_isr, CHANGE);
        baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
    }

    if (baseProtocolState == BASEPROTOCOL433_RX_MESSAGERECEIVED) {
        sleep(1);
        Serial.print("rBBP = ");
        Serial.println(rawBitBufferPosition);
        Serial.print("bPS = ");
        Serial.println(baseProtocolState);
        for (x = 0; x <= rawBitBufferPosition; x++) {
            Serial.print("x = ");
            Serial.print(x);
            Serial.print(", S = ");
            Serial.print(rawBitBuffer[x].state);
            Serial.print(", D = ");
            Serial.println(rawBitBuffer[x].duration);
        }      

        baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
        attachInterrupt(digitalPinToInterrupt(35), qam_rx4_isr, CHANGE);
    }

    vTaskDelay(1);

}

