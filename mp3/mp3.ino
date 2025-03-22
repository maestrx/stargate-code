#include <MD_CirQueue.h>

#include <Wire.h>
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/*rx =*/3, /*tx =*/2);
#define FPSerial softSerial

DFRobotDFPlayerMini MP3player;

const uint8_t QUEUE_SIZE = 10;
MD_CirQueue Q(QUEUE_SIZE, sizeof(int));

int action;

void setup() {
  Wire.begin(9); // Start the I2C Bus as Slave on address 9
  Wire.onReceive(receiveEvent1);  // listen for SFX trigger instructions

  FPSerial.begin(9600);
  Serial.begin(115200);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!MP3player.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  MP3player.volume(30);  //Set volume value. From 0 to 30
}



void loop() {

  if (!Q.isEmpty()){
    uint8_t action;
    Q.pop((uint8_t *)&action);
    Serial.print("Action:");
    Serial.println(action);
    if (action == 0){// Listen for a command to stop playing from DHD or gate
      Serial.println("stop");
      MP3player.stop(); 
      Serial.println("stop");
    }else {
      MP3player.play(action); // Gate start and turn
    }
    action = -1;
  }

}

void receiveEvent1(int bytes) {
  action = Wire.read();
  Serial.print("Got:");
  Serial.println(action);

  bool b;
  b = Q.push((uint8_t *)&action);
  Serial.println(b ? " Queued" : " Queue fail");
}
