#define FAKE_GATE 1     // fake gate for testing

#define DEBUG_I2C false
#define DEBUG_I2C_DEV if(DEBUG_I2C)Serial

#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <CNCShield.h>

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

ArduinoQueue<i2c_message> i2c_message_queue_in(32);
ArduinoQueue<i2c_message> i2c_message_queue_out(32);

#define SPEED_STEPS_PER_SECOND 1000

const int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array
unsigned long address_last_key_millis = 0;
const long address_key_input_timeout = 10000;

#ifdef FAKE_GATE
  const int gate_chevron_steps = 810;
#else
  const int gate_chevron_steps = 246;
#endif
const int chevron_open_steps = 8;   // 9585 kroku dokola , 639 * 15

CNCShield cnc_shield;
StepperMotor *motor_gate = cnc_shield.get_motor(0);
StepperMotor *motor_chevron = cnc_shield.get_motor(1);

const int Calibrate_LED = 15;
const int Calibrate_Resistor = A8;
const int Ramp_LED = 16;
const int Chevron_LED[] = {40,41,42,43,44,45,46,47};     // TBD!!!

uint8_t current_symbol;


void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial << F("+++ Setup start") << endl;

  Wire.begin(8);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(i2c_recieve);
  Wire.onRequest(i2c_send);

  pinMode(Calibrate_LED, OUTPUT);
  #ifdef FAKE_GATE
    pinMode(Calibrate_Resistor, INPUT_PULLUP);
  #else
    pinMode(Calibrate_Resistor, INPUT);
  #endif
  Serial << F("* LED mode set") << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }

  Serial << F("* CNC enable") << endl;
  cnc_shield.begin();
  motor_gate->set_speed(SPEED_STEPS_PER_SECOND);
  motor_chevron->set_speed(SPEED_STEPS_PER_SECOND);

  resetGate();

  Serial << F("* Setup done") << endl;

}

void loop(){
  process_in_queue();

  if (address_last_key_millis > 0 && millis() - address_last_key_millis > address_key_input_timeout){
    // timeout
    Serial << F("- Gate timeout") << endl;
    resetGate();
  }
}

void dial(){
  uint8_t dial_direction;
  uint8_t steps;
  Serial << F("* dial action: i2c_message_in.action") << endl;
  if ((i2c_message_in.action % 2) != 0){ // if dial symbol is even, rotate symbols left
    dial_direction = CLOCKWISE; // dial rotation is opposit to the direction of the symbol !!!
    Serial << F("* dial directon:CLOCKWISE symbol direction:COUNTER current_symbol:") << current_symbol << F(" i2c_message_in.chevron:") << i2c_message_in.chevron << endl;
    if ( current_symbol < i2c_message_in.chevron ){
      steps = 39 - i2c_message_in.chevron + current_symbol;
      Serial << F("* dial crossing top: 39 - i2c_message_in.chevron + current_symbol = ") << steps << F(" steps") << endl;
    }else{
      steps = current_symbol - i2c_message_in.chevron;
      Serial << F("* dial not crossing top: current_symbol - i2c_message_in.chevron = ") << steps << F(" steps") << endl;
    }

  }else{
    dial_direction = COUNTER; // dial rotation is opposit to the direction of the symbol !!!
    Serial << F("* dial directon:COUNTER symbol direction:CLOCKWISE current_symbol:") << current_symbol << F(" i2c_message_in.chevron:") << i2c_message_in.chevron << endl;
    if ( current_symbol < i2c_message_in.chevron ){
      steps = i2c_message_in.chevron - current_symbol;
      Serial << F("* dial not crossing top: i2c_message_in.chevron - current_symbol = ") << steps << F(" steps") << endl;
    }else{
      steps = 39 - current_symbol + i2c_message_in.chevron;
      Serial << F("* dial crossing top: 39 - current_symbol + i2c_message_in.chevron = ") << steps << F(" steps") << endl;
    }

  }

  cnc_shield.enable();
  motor_gate->step(gate_chevron_steps * steps, dial_direction);
  current_symbol = i2c_message_in.chevron;
  Serial << F("* current_symbol after: ") << current_symbol  << endl;

  motor_chevron->step(5, CLOCKWISE);
  delay(500);
  motor_chevron->step(8, COUNTER);
  cnc_shield.disable();

  delay(3000);
}

void resetGate(){
  Serial << F("--- Address sequence reset") << endl;
  address_last_key_millis = 0;

  // turn off all LEDs
  digitalWrite(Ramp_LED, LOW);
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevron_LED[tmp_chevron], LOW);
  }

  // rotate symbols to the initial position
  Serial << F("Calib LED ON") << endl;
  digitalWrite(Calibrate_LED, HIGH);
  cnc_shield.enable();
  #ifdef FAKE_GATE
    for (int i=0;i<5000;i++){
      motor_gate->step(3, CLOCKWISE);
   #else
    for (int i=0;i<1000;i++){
      motor_gate->step(15, CLOCKWISE);
   #endif

    int calib = analogRead(Calibrate_Resistor);
    // Serial << F("Calib LED read:") << calib << endl;
    #ifdef FAKE_GATE
      if (calib>99){
    #else
      if (calib>3){
    #endif
        Serial << F("Calib steps:") << i << endl;
        break;
      }
  }
  #ifndef FAKE_GATE
    cnc_shield.disable();
  #endif
  digitalWrite(Calibrate_LED, LOW);

  // set current symbol on the top of the gate
  current_symbol = 1;
}

void process_in_queue(){
  if (i2c_message_queue_in.itemCount()) {

    i2c_message_in = i2c_message_queue_in.dequeue();
    Serial << F("* Message details:") << i2c_message_in.action << F("/") << i2c_message_in.chevron << endl;

    // incoming dial chevron
    if (i2c_message_in.action > 0 and i2c_message_in.action < 8){
      Serial << F("* Sending dial started") << endl;
      i2c_message_out.action = ACTION_DIAL_START;
      i2c_message_out.chevron = i2c_message_in.chevron;
      i2c_message_queue_out.enqueue(i2c_message_out);

      // do the dial
      dial();

      // do chevron sound
      Serial << F("* Sending dial done") << endl;
      i2c_message_out.action = ACTION_DIAL_END;
      i2c_message_out.chevron = i2c_message_in.chevron;
      i2c_message_queue_out.enqueue(i2c_message_out);
      digitalWrite(dLED[i2c_message_in.action-1], HIGH);

      delay(3000);

    // valid address entered, establish gate
    } else if (i2c_message_in.action == ACTION_ADDR_VALID){

    // INVALID address entered, reset gate
    } else if (i2c_message_in.action == ACTION_ADDR_INVALID){

    // reset gate (after stop button ?)
    } else if (i2c_message_in.action == ACTION_GATE_RESET){
      Serial << F("- reset dial recieved") << endl;
      resetGate();
    }

  }

}

void i2c_recieve() {
  DEBUG_I2C_DEV << F("i I2C recieve") << endl;
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  DEBUG_I2C_DEV << F("i Recieved message:") << i2c_message_recieve.action << F("/") << i2c_message_recieve.chevron << endl;
  i2c_message_queue_in.enqueue(i2c_message_recieve);
}

void i2c_send(){
  if (i2c_message_queue_out.itemCount()) {
    DEBUG_I2C_DEV << F("i Sending message from the queue") << endl;
    i2c_message_send = i2c_message_queue_out.dequeue();
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }else{
    DEBUG_I2C_DEV << F("i Sending NOOP response ") << endl;
    i2c_message_send.action = ACTION_NOOP;
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }

}

