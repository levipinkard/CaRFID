/*
 * CaRFID by Levi Pinkard
 * 
 * The Adafruit_PN532 ibrary and some of the code in this file is from Adafruit Industries, from the iso14443a_uid file.
 * 
 * Software License Agreement (BSD License)
 * Copyright (c) 2012, Adafruit Industries
 * All rights reserved.
 */
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)
#define START_BUT (13) 
#define STOP_BUT (12)
#define CORRECT_LED (11) 
#define CANCEL_LED (10)
#define TRANS_BY (1)
#define KEY_IN (8)
#define ACC (4)
#define RUN (6)
#define START (5)
//Secondary start pin used for some vehicles with multiple connections during start
#define START_2 (7)
//Brightness from 0-255 used for the correct/start LED and the incorrect/stop/cancel LED
#define COR_BRIGHTNESS (25)
#define INC_BRIGHTNESS (255)
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
bool START_APPROVED;
bool running;
void setup(void) {
  START_APPROVED = false;
  running = false;
  pinMode(KEY_IN, OUTPUT);
  pinMode(RUN, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(START_2, OUTPUT);
  pinMode(ACC, OUTPUT);
  pinMode(START_BUT, INPUT);
  pinMode(STOP_BUT, INPUT);
  pinMode(CORRECT_LED, OUTPUT);
  pinMode(CORRECT_LED, OUTPUT);
  pinMode(TRANS_BY, OUTPUT);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    while (1); // Halt, no reader detected
  }
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
}
void loop(void) {
  //Ensures that authentication hasn't already happened
  if (!START_APPROVED){
    boolean success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
    if (success) {
      //This section is dirty, will clean-up later to simplify chip enrollment
      //Set to false to disable a card/chip entirely
      boolean correctRight = true;
      boolean correctLeft = true;
      boolean correctSpare = true;
      boolean correctVivo = true;
      //UID values for valid cards. Set to your own cards, labeled for my initial use (xSIID in right hand, NExT in left, VivoKey Spark 2 in left, and a spare card to act as a "key").
      uint8_t rightHand[] = {0x2, 0xAF ,0xF3 ,0x12 ,0xB2 ,0x80 ,0x12};
      uint8_t leftHand[] = {0x2, 0xAF ,0xF3 ,0x12 ,0xB2 ,0x80 ,0x12};
      uint8_t vivoHand[] = {0x2, 0xAF ,0xF3 ,0x12 ,0xB2 ,0x80 ,0x12};
      uint8_t spareCard[] = {0x4,0xCA ,0xF1 ,0x45};
      for (uint8_t i = 0; i < uidLength; i++) 
      { if (uidLength >= 7) {
          //Authenticates the 7-byte UIDs
          correctSpare = false;
          if (rightHand[i] != uid[i]) correctRight = false;
          if (leftHand[i] != uid[i]) correctLeft = false;
          if (vivoHand[i] != uid[i]) correctVivo = false;
        } else if (uidLength >= 4) { 
          //Authenticates 4-byte UIDS
          correctRight = false;
          correctLeft = false;
          correctVivo = false;
          if (spareCard[i] != uid[i]) correctSpare = false;
        } else { //Neither 7 or 4 byte = wrong by default
          correctRight = false;
          correctLeft = false;
          correctVivo = false;
          correctSpare = false;
        }
      }
      if(correctLeft || correctVivo || correctRight || correctSpare){
        //Turns on transponder bypass device
        digitalWrite(TRANS_BY, HIGH);
        delay(500);
        //Inserts key
        digitalWrite(KEY_IN, HIGH);
        //Enables start button
        START_APPROVED = true;
        //Turns on accessory mode (radio, wipers, etc)
        digitalWrite(ACC, HIGH);
        //Disables cancel LED if enabled, and turns on correct/start LED
        analogWrite(CANCEL_LED, 0);
        analogWrite(CORRECT_LED, COR_BRIGHTNESS);
      } else {
        //If UID is incorrect, flash the cancel/stop LED for 2 seconds
        analogWrite(CANCEL_LED, INC_BRIGHTNESS);
        delay(2000);
        analogWrite(CANCEL_LED, 0);
      }
    // Wait 1 second before continuing
    delay(1000);
    }
  } else { //Waiting for input post-authentication
    
    //Buttons connect to ground with a pull-up resistor, value must be inverted on read
    bool stop_read = !digitalRead(STOP_BUT);
    bool start_read = !digitalRead(START_BUT);
    if (stop_read) {
      //STOP. EVERYTHING. (Potentially critical) Stop button has priority if conflict occurs.
      //Tries to move through stages naturally
      digitalWrite(START, LOW);
      digitalWrite(START_2, LOW);
      digitalWrite(RUN, LOW);
      delay(300);
      digitalWrite(ACC, LOW);
      delay(300);
      digitalWrite(KEY_IN, LOW);
      delay(300);
      analogWrite(CORRECT_LED, 0);
      //Flashes cancel/stop LED for a second
      analogWrite(CANCEL_LED, INC_BRIGHTNESS);
      delay(1000);
      analogWrite(CANCEL_LED, 0);
      //Stops start until next authentication, disables running (enabling start button)
      START_APPROVED = false;
      running = false;
    } else if (start_read && !running) {
      //Disables accessory
      digitalWrite(ACC, LOW);
      //Enables run
      digitalWrite(RUN, HIGH);
      delay(1000);
      //Starts for 4 seconds in case of cold crank, then stops
      digitalWrite(START, HIGH);      
      digitalWrite(START_2, HIGH);
      delay(4000);
      digitalWrite(START, LOW);
      digitalWrite(START_2, LOW);
      analogWrite(CORRECT_LED, COR_BRIGHTNESS);
      //Enable running, stopping start from being hit again)
      running = true;
    }
  }
}
