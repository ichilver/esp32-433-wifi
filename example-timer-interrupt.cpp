
//#define USE_WIFI
#ifdef USE_WIFI
  const char* wifi_ssid     = "MyWiFi";
  const char* wifi_password = "00000000001";
  const char* wifi_hostname = "myesp32";

  #include <WiFi.h>

  #define DEBUG_WIFI
#endif

#include <Arduino.h>
#include <esp32-hal-timer.h>

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

volatile unsigned int durationCount = 0;
hw_timer_t *timer = NULL;
volatile bool timerRunFlag = 0;

volatile uint32_t intOnCoreNum = 9;

volatile byte baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
volatile byte prevBaseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
volatile bool bufferOverflowFlag = 0;

void IRAM_ATTR timerCallback(){
  
  unsigned int duration = 0;

  static bool prevState = 0;

  if (timerRunFlag) {

    bool curState = digitalRead(35);

    if (curState != prevState) {
      // State has changed
      duration = (durationCount+1)*40;

      // If we find the preample (4.5ms < x < 12.5ms) trigger baseProtocolState to receiving mode
      if ((duration > BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MIN) && (duration < BASEPROTOCOL433_RX_PREAMPLE_THRESOLD_MAX) ) {
        if (baseProtocolState == BASEPROTOCOL433_RX_RECEIVEMODE) {
            baseProtocolState = BASEPROTOCOL433_RX_MESSAGERECEIVED;
            timerRunFlag = 0;
        } else {
            baseProtocolState = BASEPROTOCOL433_RX_RECEIVEMODE;
        }
      }

      if (baseProtocolState == BASEPROTOCOL433_RX_RECEIVEMODE) {
        // Store the duration of the received pulse
        rawBitBuffer[rawBitBufferPosition].state = !curState;
        rawBitBuffer[rawBitBufferPosition].duration = duration;
        rawBitBufferPosition++;
      }

      if (rawBitBufferPosition == (RAWBITBUFFERSIZE - 1)) {
        baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;
        bufferOverflowFlag = 1;
        rawBitBufferPosition = 0;
      }

      prevState = curState;
      durationCount = 0;
    } else {
      // no change inc the counter
      // each count is 20usec
      durationCount++;
    }
  }
}


void setup() {
  Serial.begin(115200);

  rawBitBufferPosition = 0;
  baseProtocolState = BASEPROTOCOL433_RX_LISTENMODE;

  // Configure QAM-RX4 data pin as an input pin
  pinMode(35, INPUT);
  
  #ifdef USE_WIFI
    setup_wifi();
  #endif

  // Initialize the timer
  // ESP32 Frequency is 80Mhz
  // Timer speed = Clock Speed / prescaler
  //       25000 = 80000000 / 3200
  //      250000 = 80000000 / 320
  //      100000 = 80000000 / 800
  timer = timerBegin(0, 3200, true); 

  // Set the timer interval and attach the callback function
  timerAttachInterrupt(timer, &timerCallback, true);

  // Time between callbacks = (1 / Timer frequency) * Alarm value
  // Time between callbacks = (1 /  25,000 Hz) * 1 ticks = 40 microseconds
  // Time between callbacks = (1 / 250,000 Hz) * 5 ticks = 20 microseconds
  // Time between callbacks = (1 / 100,000 Hz) * 4 ticks = 40 microseconds
  timerAlarmWrite(timer, 1, true); 

  // Start the timer
  timerAlarmEnable(timer);  
  timerRunFlag = 1;

  Serial.println("Setup() done");

}

void loop() {
  unsigned long now = millis();
  int x;

  if (now - lastRedTime > 2000) {
    Serial.print("loop()  - core# ");
    Serial.println(xPortGetCoreID());
    lastRedTime = now;
  }

  if (prevBaseProtocolState != baseProtocolState) {
    Serial.print("bPS = ");
    Serial.println(baseProtocolState);
    Serial.print("pos = ");
    Serial.println(rawBitBufferPosition);
    prevBaseProtocolState = baseProtocolState;
  }

  if (bufferOverflowFlag == 1) {
    Serial.println("bufferOverflow");
    bufferOverflowFlag = 0;
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
    timerRunFlag = 1;
    // timerAlarmEnable(timer);  
    //esp_timer_start_periodic(timer, 20);
  }

  vTaskDelay(1);

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