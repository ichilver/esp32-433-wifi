# esp32-433-wifi

## Overview

Trying to get a QAM 433Mhz receiver working with a ESP32 and WiFi enabled.   

All examples work fine when the WiFi section is not enabled by the hash define.   

* example.cpp - Is just the basic 433Mhz receiver that listens for a message    
* example-using-both-cores.cpp - My 433 code on CPU1 and the Wifi on CPU0   
* example-thread.cpp - Tried using threads    
* example-timer-interrupt.cpp - Instead of a pin edge triggered interrupt I tried using a timer interrupt for every 25us.   

##  Hardware

1. ESP32 Wroom DevKit v4   
1. QAM-RX 433Mhz receiver   

The data pin of the QAM-RX is connected to the GPIO 35 on ESP32.   

## 433Mhz Overview

When we receive a long preample between 8ms and 25ms we start recording the message to an array until we receive the postample.   

Once the postample is received we stop listening to give other code (not in the examples) a chance to process the received message.   

Then when the message is decoded we put the state back into listening mode again.   
