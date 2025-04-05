// version 0.1, 1.4.2025

#define FAKE_GATE 1     // fake gate for testing

#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <TEvent.h>
#include <Timer.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

#define MP3_GATE_DIALING 1
#define MP3_CHEVRON_SEAL 2
#define MP3_WORMHOLE_START 3
#define MP3_WORMHOLE_STOP 4
#define MP3_WORMHOLE_RUNNING 5
#define MP3_CHEVRON_1 6
#define MP3_CHEVRON_2 7
#define MP3_CHEVRON_3 8
#define MP3_CHEVRON_4 9
#define MP3_CHEVRON_5 10
#define MP3_CHEVRON_6 11
#define MP3_CHEVRON_7 12
#define MP3_UNKNOWN 13

// https://forum.arduino.cc/t/sending-struct-over-i2c/886392/30
// https://github.com/EinarArnason/ArduinoQueue
struct i2c_message {
    // action:
    //   --- DHD => GATE
    //   1 -> chevron 1 ID
    //   2 -> chevron 2 ID
    //   3 -> chevron 3 ID
    //   4 -> chevron 4 ID
    //   5 -> chevron 5 ID
    //   6 -> chevron 6 ID
    //   7 -> chevron 7 ID
    //   20 -> RED button pressed, valid address
    //   21 -> RED button pressed, INVALID address, reset dial
    //   22 -> reset dial/close gate (RED button pressed to close gate)
    //   99 -> No Operation
    //   --- DHD => MP3
    //   X -> Play sound X (1-14)
    //   50 -> Stop sounds
    //   99 -> No Operation
    //   --- GATE => DHD
    //   10 -> chevron dialing started
    //   11 -> chevron dialing done
    //   99 -> No Operation
    uint8_t action;
    // chevron:
    //   chevron ID -> chevron 1 dialing done
    uint8_t chevron;
};
#define ACTION_DIAL_START 10
#define ACTION_DIAL_END 11
#define ACTION_ADDR_VALID 20
#define ACTION_ADDR_INVALID 21
#define ACTION_GATE_RESET 22
#define ACTION_SOUND_STOP 50
#define ACTION_NOOP 99
#define ACTION_NODATA 255  // indicates that no data were recieved over the I2C bus

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



