#include <stargate.h>
#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

// i2c objects to read and send messages
i2c_message i2c_message_send;
i2c_message i2c_message_recieve;
i2c_message i2c_message_out;
i2c_message i2c_message_in;

// queus for i2c messages
ArduinoQueue<i2c_message> i2c_message_queue_in(16);
ArduinoQueue<i2c_message> i2c_message_queue_out(4);

// mp3 player object
#ifdef FAKE_GATE
SoftwareSerial MP3Serial(/*rx =*/10, /*tx =*/11);
#else
SoftwareSerial MP3Serial(/*rx =*/3, /*tx =*/2);
#endif
DFRobotDFPlayerMini MP3player;

void setup() {
  // init local serial
  Serial.begin(115200);
  while(!Serial);
  Serial << F("+++ Setup start") << endl;

  // init gate mp3 player
  MP3Serial.begin(9600);
  Serial << F("Initializing DFPlayer ... (May take 3~5 seconds)") << endl;
  if (!MP3player.begin(MP3Serial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial << F("Unable to begin: Check wiring and verify SD card!") << endl;
    while(true){
      delay(0);
    }
  }

  // set voluem for the mp3 player
  Serial << F("* MP3 volume set") << endl;
  MP3player.volume(MP3_VOLUME);  //Set volume value. From 0 to 30

  // Init I2C bus
  Serial << F("* I2C init") << endl;
  Wire.begin(9);                  // Start the I2C Bus as SLAVE on address 9
  Wire.onReceive(i2c_recieve);
  Wire.onRequest(i2c_send);

  // mp3 ready
  Serial << F("+++ Setup done") << endl;
}

// main loop
void loop() {
  // check if there are any recieved I2C messages in the input queue
  process_in_queue();

}

// process the incoming I2C message queue from DHD
void process_in_queue(){

  // in case tehre are some messages in teh queue
  if (i2c_message_queue_in.itemCount()) {
    Serial << F("* Processing message from DHD") << endl;
    i2c_message_in = i2c_message_queue_in.dequeue();
    Serial << F("* Message details:") << i2c_message_in.action << F("/") << i2c_message_in.chevron << endl;

    // incoming sound stop message
    if (i2c_message_in.action == 50){
      Serial << F("* Stop playback") << endl;
      MP3player.stop();

    // play sound ID from message
    } else {
      Serial << F("* Play track ID: ") << i2c_message_in.action << endl;
      MP3player.play(i2c_message_in.action);
    }
  }
}

// recieve incoming message from DHD and put it into the incoming queue
void i2c_recieve() {
  DEBUG_I2C_DEV << F("i I2C recieve") << endl;
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  DEBUG_I2C_DEV << F("i Recieved message:") << i2c_message_recieve.action << F("/") << i2c_message_recieve.chevron << endl;
  i2c_message_queue_in.enqueue(i2c_message_recieve);
}

// respond to the I2C message poll by DHD
void i2c_send(){
  // if tehre is a message, send it
  if (i2c_message_queue_out.itemCount()) {
    DEBUG_I2C_DEV << F("i Sending message from the queue") << endl;
    i2c_message_send = i2c_message_queue_out.dequeue();
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));

  // otherwise send NOOP message
  }else{
    DEBUG_I2C_DEV << F("i Sending NOOP response ") << endl;
    i2c_message_send.action = ACTION_NOOP;
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }
}
