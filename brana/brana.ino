#define FAKE_GATE 1     // fake gate for testing

#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <CNCShield.h>

// https://forum.arduino.cc/t/sending-struct-over-i2c/886392/30
// https://github.com/EinarArnason/ArduinoQueue
struct i2c_message {
    // action:
    //   --- DHD => GATE
    //   0 -> No Operation
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
    //   --- DHD => MP3
    //   0 -> Stop sounds
    //   X -> Play sound X (1-14)
    //   --- GATE => DHD
    //   11 -> chevron 1 dialing done
    //   12 -> chevron 2 dialing done
    //   13 -> chevron 3 dialing done
    //   14 -> chevron 4 dialing done
    //   15 -> chevron 5 dialing done
    //   16 -> chevron 6 dialing done
    //   17 -> chevron 7 dialing done
    uint8_t action;
    // chevron:
    //   chevron ID -> chevron 1 dialing done
    uint8_t chevron;
};

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
  Serial << "+++ Setup start" << endl;

  Wire.begin(8);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(i2c_recieve);
  Wire.onRequest(i2c_send);

  pinMode(Calibrate_LED, OUTPUT);
  #ifdef FAKE_GATE
    pinMode(Calibrate_Resistor, INPUT_PULLUP);
  #else
    pinMode(Calibrate_Resistor, INPUT);
  #endif
  Serial << "* LED mode set" << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }

  Serial << "* CNC enable" << endl;
  cnc_shield.begin();
  motor_gate->set_speed(SPEED_STEPS_PER_SECOND);
  motor_chevron->set_speed(SPEED_STEPS_PER_SECOND);

  resetGate();

  Serial << "* Setup done" << endl;

}

void loop(){
  process_in_queue();

  if (address_last_key_millis > 0 && millis() - address_last_key_millis > address_key_input_timeout){
    // timeout
    Serial << "- Gate timeout" << endl;
    resetGate();
  }
}

void dial(){
  uint8_t dial_direction;
  uint8_t steps;
  Serial << "* dial action: i2c_message_in.action" << endl;
  if ((i2c_message_in.action % 2) != 0){ // if dial symbol is even, rotate symbols left
    dial_direction = CLOCKWISE; // dial rotation is opposit to the direction of the symbol !!!
    Serial << "* dial directon:CLOCKWISE symbol direction:COUNTER current_symbol:" << current_symbol << " i2c_message_in.chevron:" << i2c_message_in.chevron << endl;
    if ( current_symbol < i2c_message_in.chevron ){
      steps = 39 - i2c_message_in.chevron + current_symbol;
      Serial << "* dial crossing top: 39 - i2c_message_in.chevron + current_symbol = " << steps << " steps" << endl;
    }else{
      steps = current_symbol - i2c_message_in.chevron;
      Serial << "* dial not crossing top: current_symbol - i2c_message_in.chevron = " << steps << " steps" << endl;
    }

  }else{
    dial_direction = COUNTER; // dial rotation is opposit to the direction of the symbol !!!
    Serial << "* dial directon:COUNTER symbol direction:CLOCKWISE current_symbol:" << current_symbol << " i2c_message_in.chevron:" << i2c_message_in.chevron << endl;
    if ( current_symbol < i2c_message_in.chevron ){
      steps = i2c_message_in.chevron - current_symbol;
      Serial << "* dial not crossing top: i2c_message_in.chevron - current_symbol = " << steps << " steps" << endl;
    }else{
      steps = 39 - current_symbol + i2c_message_in.chevron;
      Serial << "* dial crossing top: 39 - current_symbol + i2c_message_in.chevron = " << steps << " steps" << endl;
    }

  }

  cnc_shield.enable();
  motor_gate->step(gate_chevron_steps * steps, dial_direction);
  current_symbol = i2c_message_in.chevron;
  Serial << "* current_symbol after: " << current_symbol  << endl;

  motor_chevron->step(5, CLOCKWISE);
  delay(500);
  motor_chevron->step(8, COUNTER);
  cnc_shield.disable();

  delay(3000);
}

void resetGate(){
  Serial << "--- Address sequence reset" << endl;
  address_last_key_millis = 0;

  // turn off all LEDs
  digitalWrite(Ramp_LED, LOW);
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevron_LED[tmp_chevron], LOW);
  }

  // rotate symbols to the initial position
  Serial << "Calib LED ON" << endl;
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
    Serial << "Calib LED read:" << calib << endl;
    #ifdef FAKE_GATE
      if (calib>99){
    #else
      if (calib>3){
    #endif
        Serial << "Calib steps:" << i << endl;
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
    Serial << "* Processing message from DHD" << endl;
    i2c_message_in = i2c_message_queue_in.dequeue();
    Serial << "* Message details:" << i2c_message_in.action << "/" << i2c_message_in.chevron << endl;

    // incoming dial chevron
    if (i2c_message_in.action > 0 and i2c_message_in.action < 8){
      dial();
      digitalWrite(dLED[i2c_message_in.action-1], HIGH);
      delay(5000);

    // valid address entered, establish gate
    } else if (i2c_message_in.action == 20){

    // INVALID address entered, reset gate
    } else if (i2c_message_in.action == 21){

    // reset gate (after stop button ?)
    } else if (i2c_message_in.action == 22){
      Serial << "- reset dial recieved" << endl;
      resetGate();
    }

  }

}

void i2c_recieve() {
  Serial << "++ wireRecieve" << endl;
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  Serial << "* Recieved message:" << i2c_message_recieve.action << "/" << i2c_message_recieve.chevron << endl;
  i2c_message_queue_in.enqueue(i2c_message_recieve);
}

void i2c_send(){
  if (i2c_message_queue_out.itemCount()) {
    Serial << "* Sending message from the queue" << endl;
    i2c_message_send = i2c_message_queue_out.dequeue();
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
  }

}

