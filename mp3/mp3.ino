// version 0.1, 1.4.2025

#include <stargate.h>
#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <TEvent.h>
#include <Timer.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

i2c_message i2c_message_send;
i2c_message i2c_message_recieve;
i2c_message i2c_message_out;
i2c_message i2c_message_in;

ArduinoQueue<i2c_message> i2c_message_queue_in(16);
ArduinoQueue<i2c_message> i2c_message_queue_out(4);

#ifdef FAKE_GATE
SoftwareSerial MP3Serial(/*rx =*/10, /*tx =*/11);
#else
SoftwareSerial MP3Serial(/*rx =*/3, /*tx =*/2);
#endif
DFRobotDFPlayerMini MP3player;


void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial << F("+++ Setup start") << endl;

  MP3Serial.begin(9600);

  Wire.begin(9);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(i2c_recieve);
  Wire.onRequest(i2c_send);

  Serial << F("Initializing DFPlayer ... (May take 3~5 seconds)") << endl;
  if (!MP3player.begin(MP3Serial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial << F("Unable to begin: Check wiring and verify SD card!") << endl;
    while(true){
      delay(0);
    }
  }
#ifdef FAKE_GATE
  MP3player.volume(15);  //Set volume value. From 0 to 30
#else
  MP3player.volume(30);  //Set volume value. From 0 to 30
#endif

  Serial << F("* Setup done") << endl;
}



void loop() {
  process_in_queue();

}

void process_in_queue(){
  if (i2c_message_queue_in.itemCount()) {
    Serial << F("* Processing message from DHD") << endl;
    i2c_message_in = i2c_message_queue_in.dequeue();

    Serial << F("* Message details:") << i2c_message_in.action << F("/") << i2c_message_in.chevron << endl;
    if (i2c_message_in.action == 50){
      Serial << F("* Stop playback") << endl;
      MP3player.stop();

    } else {
      Serial << F("* Play track ID: ") << i2c_message_in.action << endl;
      MP3player.play(i2c_message_in.action);
    }
  }
}


void i2c_recieve() {
  Serial << F("++ wireRecieve") << endl;
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  Serial << F("* Recieved message:") << i2c_message_recieve.action << F("/") << i2c_message_recieve.chevron << endl;
  i2c_message_queue_in.enqueue(i2c_message_recieve);
}

void i2c_send(){
  if (i2c_message_queue_out.itemCount()) {
    Serial << F("* Sending message from the queue") << endl;
    i2c_message_send = i2c_message_queue_out.dequeue();
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }else{
    Serial << F("* Sending NOOP response ") << endl;
    i2c_message_send.action = ACTION_NOOP;
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }

}



