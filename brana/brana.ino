#include <PrintStream.h>
#include <CNCShield.h>
#include <MD_CirQueue.h>
#include <Wire.h>

#define SPEED_STEPS_PER_SECOND 1000

#define ATRAPA

const uint8_t QUEUE_SIZE = 24;
#ifdef ATRAPA
  const int gate_chevron_steps = 810; // 246;
#else
  const int gate_chevron_steps = 246;
#endif
const int chevron_open_steps = 8;
// 9585 kroku dokola , 639 * 15

MD_CirQueue Q(QUEUE_SIZE, sizeof(uint8_t));
CNCShield cnc_shield;
StepperMotor *motor_gate = cnc_shield.get_motor(0);
StepperMotor *motor_chevron = cnc_shield.get_motor(1);

const int Calibrate_LED = 15;
const int Calibrate_Resistor = A8;
const int Ramp_LED = 10;                     // TBD!!!
const int Chevron_LED[] = {2,3,4,5,6,7,8,9,10};     // TBD!!!

uint8_t command_action;
uint8_t command_symbol;
uint8_t current_symbol;

void setup()
{

  Serial.begin(115200);
  while(!Serial);
  Serial << "+++ Setup start" << endl;

  Wire.begin(8);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(receiveEvent);  // Populate the variables heard from the DHD over the I2C

  pinMode(Calibrate_LED, OUTPUT);
  #ifdef ATRAPA
    pinMode(Calibrate_Resistor, INPUT_PULLUP);
  #else
    pinMode(Calibrate_Resistor, INPUT);
  #endif


  Serial << "* CNC enable" << endl;
  cnc_shield.begin();
  motor_gate->set_speed(SPEED_STEPS_PER_SECOND);
  motor_chevron->set_speed(SPEED_STEPS_PER_SECOND);

  resetDial();

  cnc_shield.enable();
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);
  delay(1000);
  Serial << "steps" << endl;
  motor_gate->step(gate_chevron_steps * 1, CLOCKWISE);

}

void loop()
{
  if (!Q.isEmpty()){
    Serial << "+ reading queue" << endl;
    readQueue();
  }
}

void readQueue(){
  Q.pop((uint8_t *)&command_action);
  Serial << "* Queue pop command_action:" << command_action << endl;
  Q.pop((uint8_t *)&command_symbol);
  Serial << "* Queue pop command_symbol:" << command_symbol << endl;

  if (command_action == 0){
    Serial << "- reset dial recieved" << endl;
    resetDial();
  }else{
    Serial << "* dial" << endl;
    dial();
  }
}

void dial(){
  uint8_t dial_direction;
  uint8_t steps;
  Serial << "* dial action: command_action" << endl;
  if ((command_action % 2) != 0){ // if dial symbol is even, rotate symbols left
    dial_direction = CLOCKWISE; // dial rotation is opposit to the direction of the symbol !!!
    Serial << "* dial directon:CLOCKWISE symbol direction:COUNTER current_symbol:" << current_symbol << " command_symbol:" << command_symbol << endl;
    if ( current_symbol < command_symbol ){
      steps = 39 - command_symbol + current_symbol;
      Serial << "* dial crossing top: 39 - command_symbol + current_symbol = " << steps << " steps" << endl;
    }else{
      steps = current_symbol - command_symbol;
      Serial << "* dial not crossing top: current_symbol - command_symbol = " << steps << " steps" << endl;
    }

  }else{
    dial_direction = COUNTER; // dial rotation is opposit to the direction of the symbol !!!
    Serial << "* dial directon:COUNTER symbol direction:CLOCKWISE current_symbol:" << current_symbol << " command_symbol:" << command_symbol << endl;
    if ( current_symbol < command_symbol ){
      steps = command_symbol - current_symbol;
      Serial << "* dial not crossing top: command_symbol - current_symbol = " << steps << " steps" << endl;
    }else{
      steps = 39 - current_symbol + command_symbol;
      Serial << "* dial crossing top: 39 - current_symbol + command_symbol = " << steps << " steps" << endl;
    }

  }

  cnc_shield.enable();
  motor_gate->step(gate_chevron_steps * steps, dial_direction);
  current_symbol = command_symbol;
  Serial << "* current_symbol after: " << current_symbol  << endl;

  motor_chevron->step(5, CLOCKWISE);
  delay(500);
  motor_chevron->step(8, COUNTER);
  cnc_shield.disable();

  delay(3000);
}

void resetDial(){
  Serial << "--- reset dial" << endl;

  // turn off all LEDs
  digitalWrite(Ramp_LED, LOW);
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevron_LED[tmp_chevron], LOW);
  }

  // rotate symbols to the initial position

  Serial << "Calib LED ON" << endl;
  digitalWrite(Calibrate_LED, HIGH);
  cnc_shield.enable();
  #ifdef ATRAPA
    for (int i=0;i<5000;i++){
      motor_gate->step(3, CLOCKWISE);
   #else
    for (int i=0;i<1000;i++){
      motor_gate->step(15, CLOCKWISE);
   #endif

    int calib = analogRead(Calibrate_Resistor);
    Serial << "Calib LED read:" << calib << endl;
    #ifdef ATRAPA
      if (calib>99){
    #else
      if (calib>3){
    #endif
        Serial << "Calib steps:" << i << endl;
        break;
      }
  }
  #ifndef ATRAPA
    cnc_shield.disable();
  #endif
  digitalWrite(Calibrate_LED, LOW);

  // set current symbol on the top of the gate
  current_symbol = 1;
}


void receiveEvent(int bytes) {
  Serial << "++ recieved command" << endl;
  uint8_t wire_command_action;
  uint8_t wire_command_symbol;
  bool b;
  wire_command_action = Wire.read();
  b = Q.push((uint8_t *)&wire_command_action);
  Serial << "* Recieved wire_command_action:" << wire_command_action << " and queued:" << b << endl;
  wire_command_symbol = Wire.read();
  b = Q.push((uint8_t *)&wire_command_symbol);
  Serial << "* Recieved wire_command_symbol:" << wire_command_symbol << " and queued:" << b << endl;
}
