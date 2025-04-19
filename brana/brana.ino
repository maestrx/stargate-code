#include <stargate.h>
#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <TEvent.h>
#include <Timer.h>
#include <CNCShield.h>
#include <DFRobotDFPlayerMini.h>

// i2c objects to read and send messages
i2c_message i2c_message_send;
i2c_message i2c_message_recieve;
i2c_message i2c_message_out;
i2c_message i2c_message_in;

// queus for i2c messages
ArduinoQueue<i2c_message> i2c_message_queue_in(32);
ArduinoQueue<i2c_message> i2c_message_queue_out(32);

// timer object
Timer t;
int8_t led_blink_timer;

// timing of the gate reset
unsigned long address_last_key_millis = 0;

// mp3 player object
DFRobotDFPlayerMini MP3player;

// motor objects
CNCShield cnc_shield;
StepperMotor *motor_gate = cnc_shield.get_motor(0);
StepperMotor *motor_chevron = cnc_shield.get_motor(1);

// variable containing the ID of symbol that is being currently dialed
uint8_t current_symbol;
bool gate_wormhole_established = false;

void setup(){
  // init local serial
  Serial.begin(115200);
  while(!Serial);
  Serial << F("+++ Setup start") << endl;

  // init gate mp3 player
  Serial1.begin(9600);
  while(!Serial1);
  Serial << F("Initializing DFPlayer ... (May take 3~5 seconds)") << endl;
  if (!MP3player.begin(Serial1, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
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
  Wire.begin(8);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(i2c_recieve);
  Wire.onRequest(i2c_send);

  // set pin modes
  Serial << F("* Set PIN modes") << endl;
  pinMode(Calibrate_LED, OUTPUT);
  pinMode(Calibrate_Resistor, INPUT);
  for (int i = 0; i < 9; i ++){
    pinMode(Gate_Chevron_LED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }

  // init motors and set to max speed
  Serial << F("* CNC enable") << endl;
  cnc_shield.begin();
  motor_gate->set_speed(1000);
  motor_chevron->set_speed(1000);

  // reset teh game to default
  Serial << F("* Gate reset") << endl;
  resetGate();

  // gate ready
  Serial << F("+++ Setup done") << endl;
}

// main loop
void loop(){
  // trigger timer events
  t.update();

  // check if there are any recieved I2C messages in the input queue
  process_in_queue();

  // reset the gate to default in case no command was recieved in defined timeframe
  //if (address_last_key_millis > 0 && millis() - address_last_key_millis > GATE_ACTION_TIMEOUT){
  //  Serial << F("- Gate timeout. Doing reset!") << endl;
  //  resetGate();
  //}
}

// dial/turn the gate
// executed when dial command recieved via I2C
void dial(){
  Serial << F("+++ gate dial") << endl;

  // internal variables
  uint8_t dial_direction;
  uint8_t steps;

  // determine the direction of the dial and numebr of chevrons to rotate
  Serial << F("* dial action: i2c_message_in.action") << endl;
  if ((i2c_message_in.action % 2) != 0){ // if dial symbol is even, rotate symbols left
    dial_direction = CLOCKWISE; // dial rotation is opposit to the direction of the symbol !!!
    Serial << F("* dial directon:CLOCKWISE symbol direction:COUNTER current_symbol:") << current_symbol << F(" i2c_message_in.chevron:") << i2c_message_in.chevron << endl;
    // determine the number of steps to rotate
    if ( current_symbol < i2c_message_in.chevron ){
      steps = GATE_SYMBOLS - i2c_message_in.chevron + current_symbol;
      Serial << F("* dial crossing top: 39 - i2c_message_in.chevron + current_symbol = ") << steps << F(" steps") << endl;
    }else{
      steps = current_symbol - i2c_message_in.chevron;
      Serial << F("* dial not crossing top: current_symbol - i2c_message_in.chevron = ") << steps << F(" steps") << endl;
    }

  }else{
    dial_direction = COUNTER; // dial rotation is opposit to the direction of the symbol !!!
    Serial << F("* dial directon:COUNTER symbol direction:CLOCKWISE current_symbol:") << current_symbol << F(" i2c_message_in.chevron:") << i2c_message_in.chevron << endl;
    // determine the number of steps to rotate
    if ( current_symbol < i2c_message_in.chevron ){
      steps = i2c_message_in.chevron - current_symbol;
      Serial << F("* dial not crossing top: i2c_message_in.chevron - current_symbol = ") << steps << F(" steps") << endl;
    }else{
      steps = GATE_SYMBOLS - current_symbol + i2c_message_in.chevron;
      Serial << F("* dial crossing top: 39 - current_symbol + i2c_message_in.chevron = ") << steps << F(" steps") << endl;
    }

  }

  // play gate rotate sound
  Serial << F("* Play gate dial sound") << endl;
  MP3player.stop();
  MP3player.play(1);

  // rotate the gate
  Serial << F("* Dialing the gate") << endl;
  cnc_shield.enable();
  motor_gate->step(GATE_CHEVRON_STEPS * steps, dial_direction);

  // play sound for teh chevron seal
  Serial << F("* Play chevron seal sound") << endl;
  MP3player.stop();
  MP3player.play(2);

  // while chevron seal sound id being played, seal the chevron
  Serial << F("* Sealing chevron") << endl;
  motor_chevron->step(GATE_CHEVRON_OPEN_STEPS, CLOCKWISE);
  delay(500);
  motor_chevron->step(GATE_CHEVRON_OPEN_STEPS, COUNTER);
  cnc_shield.disable();

  // set current symbol for next dialing
  current_symbol = i2c_message_in.chevron;
  Serial << F("* current_symbol after: ") << current_symbol << endl;
  Serial << F("--- gate dial end") << endl;
}

// reset gate to initial position
void resetGate(){
  Serial << F("+++ Gate reset") << endl;

  led_blink_timer = t.oscillate(Gate_Chevron_LED[8], 300, HIGH);

  if (gate_wormhole_established){
    Serial << F("* Gate reset, closing wormhole") << endl;
    MP3player.stop();
    MP3player.play(MP3_WORMHOLE_STOP);
    delay(1000);
  }
  gate_wormhole_established = false;

  // reset key timeout timer
  address_last_key_millis = 0;

  // turn off all LEDs
  Serial << F("* Turn off all LEDs") << endl;
  digitalWrite(Ramp_LED, LOW);
  for (int i = 0; i < 8; i ++){
    digitalWrite(Gate_Chevron_LED[i], LOW);
  }

  // rotate symbols to the initial position
  Serial << F("* Calib LED ON (PIN ") << Calibrate_LED << F("), dialing to default position") << endl;
  digitalWrite(Calibrate_LED, HIGH);
  delay(100);
  cnc_shield.enable();
  for (int i=0;i<2000;i++){
    int calib = analogRead(Calibrate_Resistor);
    if (calib>3){
      Serial << F("* Calib steps:") << i << endl;
      break;
    }
    motor_gate->step(5, CLOCKWISE);
    // trigger timer events
    t.update();
  }

  digitalWrite(Calibrate_LED, LOW);
  cnc_shield.disable();

  // empty incoming message queue
  while (i2c_message_queue_in.itemCount()) {
    i2c_message_queue_in.dequeue();
  }

  // set current symbol on the top of the gate
  current_symbol = 1;

  t.stop(led_blink_timer);
  digitalWrite(Gate_Chevron_LED[8], LOW);

  Serial << F("--- Gate reset done") << endl;
}

// process the incoming I2C message queue from DHD
void process_in_queue(){

  // in case tehre are some messages in teh queue
  if (i2c_message_queue_in.itemCount()) {

    // pop the message from the queue
    Serial << F("* Processing message from DHD") << endl;
    i2c_message_in = i2c_message_queue_in.dequeue();
    Serial << F("* Message details:") << i2c_message_in.action << F("/") << i2c_message_in.chevron << endl;

    // incoming dial chevron (1-7)
    if (i2c_message_in.action > 0 and i2c_message_in.action < 8){

      Serial << F("+++ Recieved dial message #") << i2c_message_in.action << endl;

      // send dial start event to dhd
      Serial << F("* Sending dial started message to DHD") << endl;
      i2c_message_out.action = ACTION_DIAL_START;
      i2c_message_out.chevron = i2c_message_in.action;
      i2c_message_queue_out.enqueue(i2c_message_out);

      // do the gate dial
      dial();

      // send dial done event to dhd
      Serial << F("* Sending dial done") << endl;
      i2c_message_out.action = ACTION_DIAL_END;
      i2c_message_out.chevron = i2c_message_in.action;
      i2c_message_queue_out.enqueue(i2c_message_out);

      // lit LED on the gate
      digitalWrite(Gate_Chevron_LED[i2c_message_in.action-1], HIGH);

      delay(3000);

    // valid address entered, establish gate
    } else if (i2c_message_in.action == ACTION_ADDR_VALID){
      // TODO: implement gate wormhole establishment
      Serial << F("+++ addr valid, start wormhole") << endl;
      MP3player.stop();
      MP3player.play(MP3_WORMHOLE_START);
      delay(1000);
      MP3player.play(MP3_WORMHOLE_RUNNING);
      gate_wormhole_established = true;

    // INVALID address entered, reset gate
    } else if (i2c_message_in.action == ACTION_ADDR_INVALID){
      Serial << F("+++ addr invalid, fail dial") << endl;
      // TODO: implement gate dial failure
      MP3player.stop();
      MP3player.play(MP3_UNKNOWN);

    // reset gate (after stop button ?)
    } else if (i2c_message_in.action == ACTION_GATE_RESET){
      Serial << F("+++ reset dial recieved") << endl;
      resetGate();

    }
  }
}

// recieve incoming message from DHD and put it into the incoming queue
void i2c_recieve() {
  DEBUG_I2C_DEV << F("i I2C recieve") << endl;
  while (Wire.available() >= sizeof(i2c_message)) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  DEBUG_I2C_DEV << F("i Recieved message:") << i2c_message_recieve.action << F("/") << i2c_message_recieve.chevron << endl;
  // if message is gate reset, than clear the queue and add only reset message
  if (i2c_message_recieve.action == ACTION_GATE_RESET){
    Serial << F("- reset command recieved, wiping in queue") << endl;
    while (i2c_message_queue_in.itemCount()) {
      i2c_message_queue_in.dequeue();
    }
  }
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
