# esp32-433-wifi

## Overview

Trying to get a QAM 433Mhz receiver working with a ESP32 and WiFi enabled.   

All examples work fine when the WiFi section is not enabled by the hash define.   

* example.cpp - Is just the basic 433Mhz receiver that listens for a message    
* example-using-both-cores.cpp - My 433 code on CPU1 and the Wifi on CPU0   
* example-thread.cpp - Tried using threads    
* example-timer-interrupt.cpp - Instead of a pin edge triggered interrupt I tried using a timer interrupt for every 25us.   

## The Problem

All examples work for the 433Mhz receiving until the WiFi code is added.   As soon as the WiFi.begin() function is called things go wrong.   
I only receive broken messages, or no message at all.   

I'm guessing that it's not possible to run both 433 code and WiFi on the same ESP32.  The WiFi is running longer than the 433 code and it therefore misses part of the receiving message.   I tried running the WiFi code on CPU0 and the 433 code on CPU1 but that made no difference.

I tried turning the WiFi off, and only turning on once a message had been received but the connection to the WiFi network takes too long each time and isn't really an option.
##  Hardware

1. ESP32 Wroom DevKit v4   
1. QAM-RX 433Mhz receiver   

The data pin of the QAM-RX is connected to the GPIO 35 on ESP32.   

## 433Mhz Overview

When we receive a long preample between 8ms and 25ms we start recording the message to an array until we receive the postample.   

Once the postample is received we stop listening to give other code (not in the examples) a chance to process the received message.   

Then when the message is decoded we put the state back into listening mode again.   

The timer interrupt was set to every 25us to give enough sample size for detecting states.   

Below shows how the signals are detected.

```
 Basic Bit Formats
     Below is an example of how some bit formats work.

            '1' bit:
             _____
            |     |
            |     |
            |     |_____

            |-----|-----|
               T     T

            '0' bit:
             _____
            |     |
            |     |
            |     |_________________________

            |-----|-------------------------|
               T               5T

            'SYNC' bit:   (Also referred to as the PREAMPLE)
             _____
            |     |
            |     |
            |     |__________________________________________________

            |-----|--------------------------------------------------|
               T                         10T

            'PAUSE' bit:
             _____
            |     |
            |     |
            |     |_______________________ . . . ____

            |-----|----------------------- . . . ----|
               T                40T

            T = 250 us
```