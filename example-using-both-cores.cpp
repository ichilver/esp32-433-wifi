
//#define USE_WIFI
#ifdef USE_WIFI
  const char* wifi_ssid     = "MyWiFi";
  const char* wifi_password = "00000000001";
  const char* wifi_hostname = "myesp32";

  #include <WiFi.h>

  #define DEBUG_WIFI
#endif

#include <Arduino.h>
#include <esp32-hal.h>
#include <esp_system.h>

TaskHandle_t Task1;

#include "main.h"

// ########################################################################################
//
// Delcare and initialise the various classes
//
// Initialize variables
// Initialize buffer index and buffer arrays
volatile PULSE rawBitBuffer[RAWBITBUFFERSIZE];
volatile unsigned int rawBitBufferPosition  = 0;

volatile unsigned long lastRedTime = 0;
volatile unsigned long lastRedTimeloop2 = 0;
volatile unsigned long lastRedTimeloop3 = 0;

volatile uint32_t intOnCoreNum = 9;

volatile byte baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
volatile byte wifiSetupState = 1;

void IRAM_ATTR qam_rx4_isr() {

  static unsigned int prevTime = 0; // lastTime
  unsigned int newTime = micros();  // now

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



void setup() {
  Serial.begin(115200);

  rawBitBufferPosition = 0;
  baseProtocolState = BASEPROTOCOL433_SETUP;

  // Configure QAM-RX4 data pin as an input pin
  pinMode(35, INPUT);
  
  #ifdef USE_WIFI
    wifiSetupState = 1;
    xTaskCreatePinnedToCore(loop3, "wifi_tasks", 20000, NULL, 23, NULL, 0);
  #endif
    // Run my main code on Core1
    xTaskCreatePinnedToCore(loop2, "qam_rx_task", 20000, NULL, 1, NULL, 1);

  Serial.println("Setup() done");

}

void loop() {
  unsigned long now = millis();

  if (now - lastRedTime > 2000) {
    Serial.print("loop()  - core# ");
    Serial.println(xPortGetCoreID());
    lastRedTime = now;
  }

  vTaskDelay(1);

}

void loop2(void * parameter) {
  int x;

  while(true) {

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
      Serial.print("loop2() - core# ");
      Serial.println(xPortGetCoreID());
      Serial.print("intOn  - core# ");
      Serial.println(intOnCoreNum);
      

      baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
      attachInterrupt(digitalPinToInterrupt(35), qam_rx4_isr, CHANGE);
    }

    vTaskDelay(1);

  }
}

void loop3(void * parameter) {
  int x;

  while(true) {

    unsigned long now = millis();
  
    if (now - lastRedTimeloop3 > 2000) {
      Serial.print("loop3()  - core# ");
      Serial.println(xPortGetCoreID());
      lastRedTimeloop3 = now;
    }

    #ifdef USE_WIFI
    if (wifiSetupState == 1) {
      setup_wifi();
      wifiSetupState = 0;
      Serial.print("loop3() - core# ");
      Serial.println(xPortGetCoreID());
    }
    #endif

    vTaskDelay(1);
  }
}



/*
 *
 * WiFi Functions
 * 
 */
#ifdef USE_WIFI

void setup_wifi() {
    // delete old config
    WiFi.disconnect(true);
    delay(1000);

    // We start by connecting to a WiFi network
    #ifdef DEBUG_WIFI
        Serial.println();
        Serial.print("setup_wifi() - core# ");
        Serial.println(xPortGetCoreID());
        Serial.print("Connecting to ");
        Serial.print(wifi_ssid);
        Serial.print(" with ");
        Serial.println(wifi_password);
        Serial.print("MAC ");
        Serial.println(WiFi.macAddress());
        Serial.print("WL_CONNECTED ");
        Serial.println(WL_CONNECTED);
    #endif

    // Set the WiFi into Station Mode
    WiFi.mode(WIFI_STA);

    // Set the hostname
    WiFi.setHostname(wifi_hostname);
    //WiFi.setHostname(hostname.c_str());
    //WiFi.encryptionType(WIFI_AUTH_WPA_PSK);

    // Start the WiFi
    WiFi.begin(wifi_ssid, wifi_password);

    // Wait for WiFi to connect
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        #ifdef DEBUG_WIFI
            Serial.print(WiFi.status());
        #endif
    }

    

    #ifdef DEBUG_WIFI
        // Display the IP Address and Details
        displayWifiDetails();
    #endif
}


void displayWifiDetails(){
      Serial.println("");
      Serial.print("displayWifiDetails() - core# ");
      Serial.println(xPortGetCoreID());
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.print("SSID       : ");
        Serial.print(wifi_ssid);
        Serial.print(" , key ");
        Serial.println(wifi_password);
        Serial.print("IP address : ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI       : ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println("WiFi disconnected");
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status());        
    }
        Serial.print("MAC        : ");
        Serial.println(WiFi.macAddress());
}

#endif