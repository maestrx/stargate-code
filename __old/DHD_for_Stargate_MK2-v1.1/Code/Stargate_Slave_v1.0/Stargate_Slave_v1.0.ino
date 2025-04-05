/* 
  Arduino dialling, calibration and light display script for Stargate Mk2 by Glitch
  http://www.glitch.org.uk

  This script is released as Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
  http://creativecommons.org/licenses/by-nc-sa/4.0/

---------------
  Additional script by Bob Urban. The additional code is released under beer license.
  If you use it and meet me, you owe me a beer.

  This code is for the Arduino Uno running the gate functions with the adafruit motorshield.
  There are three arduinos total for this DHD/Stargate/SFX version.

  The code for each arduino:
    DHD_Keyboard_Master_v1.0.ino
    Stargate_Slave_v1.0.ino (this file)
    MP3_Slave_v1.0.ino

  You may contact me at bob@schoolofawareness.com if you have any questions or improvements.    
*/

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

  /*
  Stepper motor for chevron
  Create the motor shield object with the default I2C address
  Connect a stepper motor with 200 steps per revolution (1.8 degree)
  to motor port #2 (M3 and M4)
  */
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor *SMChevron = AFMS.getStepper(200, 2);

  /*
  Stepper  motor for symbol ring
  Connect a stepper motor with 200 steps per revolution (1.8 degree)
  to motor port #1 (M1 and M2)
  */
Adafruit_StepperMotor *SMGate = AFMS.getStepper(200, 1);

  /*
  Populate Chevrons[] array with output pins according to each chevron in dialling order.
  Note: pins 0 & 1 cannot be used while the Arduino is connected to a PC
  For this reason the pins defined by default start from 2.
  */
int Chevrons[] = {2,3,4,5,6,7,8,9,10};

  //Populate Ring_Chevrons[] array with output pins according to each chevron in clockwise (or anticlockwise) order.
int Ring_Chevrons[] = {2,3,4,8,9,5,6,7,10};


  //Chevron_Locked should be set to the last chevron in the dialling sequence
int Chevron_Locked = Chevrons[8];

  //Ramp_Lights is the output pin wired to the ramp lights (this assumes all lights are connected to a single pin).
int Ramp_Lights = 11;

  //Calibrate_LED is the output pin wired to the calibration led.
int Calibrate_LED = 12;

  //LDR is the analogue input pin wired to the light dependant resistor.
int LDR = A0;

  //Set Cal to 0 for calibration and then dialling.
  //Set Cal to 3 to bypass calibration and jump to dialling.
int Cal = 3;

  //The calibration point is set 3% higher than the average. Adjust this number if you experience problems with calibration.
int  Calibration_Percent = 3;

  //How long should the locked chevron be lit before continuing dialling. Default is 1500 (1.5 seconds).
int ChevronLocked = 1500;

  //How many steps the stepper motor must turn per symbol.
  //This value is overritten by the calibrate function.
  //If you bypass calibration this value must be set accordingly.
float Step_Per_Symbol = 30.77;

  //How many steps to move the stepper motor for the chevron.
int Chevron_Step = 10;

  //Set Ring_Display to a number between 1 and 7 for predefined display functions.
  //Set Ring_Display to 0 for calibration or dialling.
int Ring_Display = 1;

  //Sample Startgate addresses. un-comment the address you want.
  //Abydos
  //int Address[] = {27,7,15,32,12,30,1};

  // Test addresses
  //int Address[] = {8,38,10,35,4,20,1};
// int Address[] = {8,2,7,3,9,4,8};

  //Othala (Asgard homeworld)
  //int Address[] = {11,27,23,16,33,3,9,1};

  //Destiny
  //int Address[] = {6,17,21,31,35,24,5,11,1};

  //Other variable definitions. No need to change these.
float LDR_avg = 0.0;
float LDR_cal = 0.0;
int Dialling = 0;
int Ring = 0;
float Step_increment = 0;
int Address_Length = 0;

int rd = 1;
int rdd = 20; // delay between chev 9 and next chev lighting
int rld = 1000; // amount of time top chev is lit
int rnd = 1620; // time before next chev seq starts
int rc = 8; // top ring chevron number
int fc = 0; // flash count
int sgad = 200; // sga dialing delay
int sga = 100; // ring loop speed
int sga8 = 420; // last chev in ring loop delay
int sgaDL = 1; // sga led dial count so you can interrupt with button 1 push
int sgc = 1; // sgc led dial count so you can interrupt with button 1 push

int k1;   // k1 - k7 hold the gate addresses
int k2;
int k3;
int k4;
int k5;
int k6;
int k7;
int Address[] = {k1,k2,k3,k4,k5,k6,k7}; // represents an array with the DHD's input
int activate = 0; // Pretty much self explanatory
int wake = 0; // ready the gate to start dialing

int x = 0; // reprents which SFX to trigger

void setup() {
  Serial.begin(9600);  // set up Serial library at 9600 bps
  Serial.println("Working Stargate Mk2");
  Wire.begin(8); // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(receiveEvent1);  // Populate the variables heard from the DHD over the I2C
   
  AFMS.begin();  // create with the default frequency 1.6KHz
  
  SMGate->setSpeed(10);  // 10 rpm 
  SMChevron->setSpeed(10);  // 10 rpm 
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    pinMode(Chevrons[tmp_chevron], OUTPUT);
  }
  
  
  pinMode(Calibrate_LED, OUTPUT);
  pinMode(LDR,INPUT);
  pinMode(Ramp_Lights, OUTPUT);
  Address_Length = sizeof (Address) / sizeof(int);

  if (Ring_Display > 0){
    Cal = 3;
    Dialling = 10;
    digitalWrite(Ramp_Lights, LOW);
  }
  else if (Cal == 3){
    Dialling = 1;
  }
  else{
    Cal = 0;
    Dialling = 0;
    Ring_Display = 0;
  }
}
void receiveEvent1(int bytes) {
    k1 = Wire.read();
    k2 = Wire.read();
    k3 = Wire.read();
    k4 = Wire.read();
    k5 = Wire.read();
    k6 = Wire.read();
    k7 = Wire.read();
    activate = Wire.read();
    wake = Wire.read();    
}
void loop() {
     // Ring_Display = 0; // unrem for debugging. Turns off the Chevron LEDs's patterns
 /* Serial.print("wake = ");
    Serial.println(wake);
 */
    if (wake == -1){ /* this should be 'wake == 1' but for some reason the I2C sends -1 instead
      and I haven't yet figured out why. But what the hell, it is a stable glitch for me. 
      If the first address selected on the DHD does not turn the chevron LEDs all off and the 
      ramp lights on, then unrem the above Serial.print(wake); to see what it's doing.
      */
      Ring_Display = 0;
      digitalWrite(Ramp_Lights, HIGH);
      for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){ // turn off LEDs
        digitalWrite(Chevrons[tmp_chevron], LOW);
      }
    }
    if (activate == 1){ // detects the DHD's big red button has been pressed
      Dialling = 1;  // trigger the dialing sequence to start
      Ring_Display = 0; // turns off the chevron LEDs pattern sequences
      wake = 0; // reset the wake variable
      activate = 0; // reset the activate variable
      sgaDL = 1; // reset sga dial LED sequence
      sgc = 1; // reset sgc dial LED sequence
      Address[0] = k1;  // populate the address array with the DHD output
      Address[1] = k2;
      Address[2] = k3;
      Address[3] = k4;
      Address[4] = k5;
      Address[5] = k6;
      Address[6] = k7;

  /*  digitalWrite(Ramp_Lights, HIGH);
      for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){ // turn off LEDs
        digitalWrite(Chevrons[tmp_chevron], LOW); 
      } */
      delay(800);
      
    }

  if (rd < 50 && Ring_Display > 0){
    rd ++;
  }
  if (rd == 50 && Ring_Display > 0){
    digitalWrite(Ring_Chevrons[0], LOW);
    digitalWrite(Ring_Chevrons[1], LOW);
    digitalWrite(Ring_Chevrons[2], LOW);
    digitalWrite(Ring_Chevrons[3], LOW);
    digitalWrite(Ring_Chevrons[4], LOW);
    digitalWrite(Ring_Chevrons[5], LOW);
    digitalWrite(Ring_Chevrons[6], LOW);
    digitalWrite(Ring_Chevrons[7], LOW);
    digitalWrite(Ring_Chevrons[8], LOW);    
    Ring_Display ++;
    rd = 1;
  }
  if (Ring_Display > 8){
    Ring_Display = 1;
  }
   
  if (Cal < 3){
    calibrate();
  }
  else if (Dialling <= Address_Length){
    digitalWrite(Ramp_Lights, HIGH);
    dial(Dialling);
    if (Dialling == Address_Length){
      delay(800); //Added by me to slow down the last chevron lock
      SMChevron->step(Chevron_Step, FORWARD, SINGLE);
      SMChevron->release();
      delay(420);
      x = 0;
      Wire.beginTransmission(9); 
      Wire.write(x);  // Stop any audio
      Wire.endTransmission();    
      delay(20); 
      x = 3;  // Wormhole Activation SFX trigger
      Wire.beginTransmission(9); 
      Wire.write(x);  // play sound effect
      Wire.endTransmission();    

      delay(5500);
    
      Serial.println("Wormhole Established");
      for (int tmp_chevron1 = 0; tmp_chevron1 < 9; tmp_chevron1 ++){
        digitalWrite(Chevrons[tmp_chevron1], HIGH);
      }
      x = 0;
      Wire.beginTransmission(9); 
      Wire.write(x);  // Stop any audio
      Wire.endTransmission();    
      delay(20); 
      x = 5;  // Event Horizon SFX trigger
      Wire.beginTransmission(9); 
      Wire.write(x);  // play sound effect
      Wire.endTransmission();  
      
      delay(10000);
      
      x = 0;
      Wire.beginTransmission(9); 
      Wire.write(x);  // Stop any audio
      Wire.endTransmission();    
      delay(20); 
      x = 4;  // Wormhole De-Activate SFX trigger
      Wire.beginTransmission(9); 
      Wire.write(x);  // play sound effect
      Wire.endTransmission();    
      delay(85);        
      
      Serial.println("Wormhole Disengaged");
      delay(4200);
      x = 0;
      Wire.beginTransmission(9); 
      Wire.write(x);  // Stop all tracks
      Wire.endTransmission();    
      delay(85);       
      for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
        digitalWrite(Chevrons[tmp_chevron], LOW);
      }
      digitalWrite(Ramp_Lights, LOW);
      Ring_Display = 1;      
    }
    Dialling ++;
  }
  
  else if (Ring_Display == 1){
    if ((Ring < 1) || (Ring > 9)){
      Ring = 1;
    }
    ring_sgaDial(1620);
    Ring ++;
    if (Ring >= 10){
      Ring = 1;
    }
  }
  else if (Ring_Display == 2){
    if ((Ring < 1) || (Ring > 18)){
      Ring = 1;
    }
    ring_chase_lights(420);
    Ring ++;
    if (Ring >= 19){
      Ring = 1;
    }
  }
  else if (Ring_Display == 3){
    if ((Ring < 1) || (Ring > 8)){
      Ring = 1;
    }
    ring_loop(420);
    Ring ++;
    if (Ring >= 9){
      Ring = 1;
    }
  }
  else if (Ring_Display == 4){
    ring_dial(420);
  }  
  else if (Ring_Display == 5){
    if ((Ring < 1) || (Ring > 14)){
      Ring = 1;
    }
    ring_lights_snake(420);
    Ring ++;
    if (Ring >= 14){
      Ring = 1;
    }
  }
  else if (Ring_Display == 6){
    ring_lights_random_triangle(150);
  }
  else if (Ring_Display == 7){
    ring_lights_random(420);
  }
  else if (Ring_Display == 8){
    if ((Ring < 1) || (Ring > 9)){
      Ring = 1;
    }
    ring_lights_triangle(420);
    Ring ++;
    if (Ring > 9){
      Ring = 1;
    }
  }

}

void dial(int Chevron) {
  x = 0;
  Wire.beginTransmission(9); 
  Wire.write(x);  // Stop any audio
  Wire.endTransmission();    
  delay(20); 
  x = 1;  // Gate start and run SFX trigger
  Wire.beginTransmission(9); 
  Wire.write(x);  // play sound effect
  Wire.endTransmission();    
  delay(420);  
    
  Serial.print("Chevron ");
  Serial.print(Chevron);
  Serial.println(" Encoded");

  int Steps_Turn = 0;
  if (Chevron == 1) {
   
    Steps_Turn = round(Step_Per_Symbol * (Address[(Chevron - 1)] - 1));
    SMGate->step(Steps_Turn, BACKWARD, DOUBLE);
  }
  else{
    Steps_Turn = Address[(Chevron - 1)] - Address[(Chevron - 2)];
    if ((Chevron % 2) == 0) {
      if  (Steps_Turn < 0) {
        Steps_Turn = Steps_Turn * -1;
      }else{
        Steps_Turn = 39 - Steps_Turn;
      }
      Steps_Turn = round(Step_Per_Symbol * Steps_Turn);
      SMGate->step(Steps_Turn, FORWARD, DOUBLE);
    }
    else{
      if  (Steps_Turn < 0) {
        Steps_Turn = 39 + Steps_Turn;
      }
      Steps_Turn = round(Step_Per_Symbol * Steps_Turn);
      SMGate->step(Steps_Turn, BACKWARD, DOUBLE);
    }
  } 
  SMGate->release();
  if ((Dialling == 9) && (Address[(Chevron - 1)] != 1)){
    for (int tmp_fail_safe_gate = 0; tmp_fail_safe_gate <= 100; tmp_fail_safe_gate ++){
      int tmp_fail_safe_chevron = random(7);
      digitalWrite(Chevrons[tmp_fail_safe_chevron], LOW);
      SMGate->step(5, BACKWARD, DOUBLE);
      SMGate->release();
      digitalWrite(Chevrons[tmp_fail_safe_chevron], HIGH);
    }
    for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
      digitalWrite(Chevrons[tmp_chevron], LOW);
    }
    Dialling = 10;
  }
  else{
    x = 0;
    Wire.beginTransmission(9); 
    Wire.write(x);  // Stop any audio
    Wire.endTransmission();    
    delay(20); 
    x = 2;  // Chevron Lock SFX trigger
    Wire.beginTransmission(9); 
    Wire.write(x);  // play sound effect
    Wire.endTransmission();    
    delay(20);     
    
    Serial.print("Chevron ");
    Serial.print(Chevron);
    Serial.println(" Locked");
    
    delay(420); // Added by me to slow the chevron lock event between dialing and event
    digitalWrite(Chevron_Locked, HIGH);
    SMChevron->step(Chevron_Step, BACKWARD, SINGLE);
    SMChevron->release();
  }
  if (Dialling < Address_Length){
    delay(ChevronLocked);
    SMChevron->step(Chevron_Step, FORWARD, SINGLE);
    SMChevron->release();
    digitalWrite(Chevron_Locked, LOW);
    delay(20);
    if ((Address_Length < 8) || (Dialling < 4)){
      digitalWrite(Chevrons[(Chevron - 1)], HIGH);
    }
    else if ((Address_Length == 8) && ((Dialling >= 4) && (Dialling <= Address_Length))){
      if (Dialling == 4){
        digitalWrite(Chevrons[(6)], HIGH);
      }
      else if (Dialling == 5){
        digitalWrite(Chevrons[(3)], HIGH);
      }
      else if (Dialling == 6){
        digitalWrite(Chevrons[(4)], HIGH);
      }
      else if (Dialling == 7){
        digitalWrite(Chevrons[(5)], HIGH);
      }
    }
    else if ((Address_Length == 9) && ((Dialling >= 4) && (Dialling <= Address_Length))){
      if (Dialling == 4){
        digitalWrite(Chevrons[(6)], HIGH);
      }
      else if (Dialling == 5){
        digitalWrite(Chevrons[(7)], HIGH);
      }
      else if (Dialling == 6){
        digitalWrite(Chevrons[(3)], HIGH);
      }
      else if (Dialling == 7){
        digitalWrite(Chevrons[(4)], HIGH);
      }
      else if (Dialling == 8){
        digitalWrite(Chevrons[(5)], HIGH);
      }
    }
  }
  delay(620);
}

void calibrate() {
  if (Cal == 0){
    digitalWrite(Calibrate_LED, HIGH);
    Serial.print("Reading 1 : ");
    int LDR_1 = analogRead(LDR);
    delay(2000);
    
    LDR_1 = analogRead(LDR);
    Serial.println(LDR_1);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 2 : ");
    
    int LDR_2 = analogRead(LDR);
    Serial.println(LDR_2);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 3 : ");
    
    int LDR_3 = analogRead(LDR);
    Serial.println(LDR_3);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 4 : ");
    
    int LDR_4 = analogRead(LDR);
    Serial.println(LDR_4);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 5 : ");
    
    int LDR_5 = analogRead(LDR);
    Serial.println(LDR_5);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 6 : ");
    
    int LDR_6 = analogRead(LDR);
    Serial.println(LDR_6);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 7 : ");
    
    int LDR_7 = analogRead(LDR);
    Serial.println(LDR_7);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 8 : ");
    
    int LDR_8 = analogRead(LDR);
    Serial.println(LDR_8);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 9 : ");
    
    int LDR_9 = analogRead(LDR);
    Serial.println(LDR_9);
    SMGate->step(10, BACKWARD, DOUBLE);
    Serial.print("Reading 10 : ");
    
    int LDR_10 = analogRead(LDR);
    Serial.println(LDR_10);
    LDR_avg = (LDR_1 + LDR_2 + LDR_3 + LDR_4 + LDR_5 + LDR_6 + LDR_7 + LDR_8 + LDR_9 + LDR_10) / 10;
    
      //Calculate average ldr value and set calibration point 3% higher
    LDR_cal = LDR_avg + ((LDR_avg / 100) * Calibration_Percent);
    
    Serial.print("LDR Average : ");
    Serial.println(LDR_avg);
    Serial.print("LDR Calibrate Point : ");
    Serial.println(LDR_cal);
    SMGate->release();
    Cal = 1;
  }
  else if (Cal == 1){
    digitalWrite(Calibrate_LED, HIGH);
    SMGate->step(1, FORWARD, MICROSTEP);
    SMGate->release();
    int tmp_cal = analogRead(LDR);
    if(tmp_cal > LDR_cal){
      Serial.print("LDR Calibrated : ");
      Serial.println(tmp_cal);
      Cal = 2;
      for (int tmp_chevron1 = 0; tmp_chevron1 < 9; tmp_chevron1 ++){
        digitalWrite(Chevrons[tmp_chevron1], HIGH);
      }
      delay(1000);
      for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
        digitalWrite(Chevrons[tmp_chevron], LOW);
      }
    }
  }
  else{
    digitalWrite(Calibrate_LED, HIGH);
    SMGate->step(1, FORWARD, MICROSTEP);
    SMGate->release();
    Step_increment ++;
    int tmp_cal = analogRead(LDR);
    if((tmp_cal > LDR_cal) && (Step_increment > 30)){
      // 39 symbols
      Step_Per_Symbol = Step_increment / 39;
      Serial.print("Steps Calculate : ");
      Serial.println(Step_Per_Symbol);
      Serial.print("Total Steps : ");
      Serial.println(Step_increment);
      digitalWrite(Calibrate_LED, LOW);
      Cal = 3;
      Dialling = 1;
      for (int tmp_chevron1 = 0; tmp_chevron1 < 9; tmp_chevron1 ++){
        digitalWrite(Chevrons[tmp_chevron1], HIGH);
      }
      delay(1000);
      for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
        digitalWrite(Chevrons[tmp_chevron], LOW);
      }
      delay(4000);
    }
    else if (Step_increment == 133){
      digitalWrite(Ring_Chevrons[5], HIGH);
    }
    else if (Step_increment == 266){
      digitalWrite(Ring_Chevrons[6], HIGH);
    }
    else if (Step_increment == 399){
      digitalWrite(Ring_Chevrons[7], HIGH);
    }
    else if (Step_increment == 532){
      digitalWrite(Ring_Chevrons[8], HIGH);
    }
    else if (Step_increment == 665){
      digitalWrite(Ring_Chevrons[0], HIGH);
    }
    else if (Step_increment == 798){
      digitalWrite(Ring_Chevrons[1], HIGH);
    }
    else if (Step_increment == 931){
      digitalWrite(Ring_Chevrons[2], HIGH);
    }
    else if (Step_increment == 1064){
      digitalWrite(Ring_Chevrons[3], HIGH);
    }  
  }
}

void ring_sgaDial(int Ring_Delay) {
    //digitalWrite(Ramp_Lights, HIGH);
    //delay(1000);
  // -------------- dial 1 -------------
  if (sgaDL == 1){
    delay(1620);
    digitalWrite(Ring_Chevrons[0], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[1], HIGH);
    digitalWrite(Ring_Chevrons[0], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[2], HIGH);
    digitalWrite(Ring_Chevrons[1], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[3], HIGH);
    digitalWrite(Ring_Chevrons[2], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[5], HIGH);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[6], HIGH);
    digitalWrite(Ring_Chevrons[5], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[6], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[0], HIGH);
    delay(rnd);
    sgaDL = 2;
  }
  // -------------- dial 2 -------------
  else if (sgaDL == 2){
    digitalWrite(Ring_Chevrons[1], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[2], HIGH);
    digitalWrite(Ring_Chevrons[1], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[3], HIGH);
    digitalWrite(Ring_Chevrons[2], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[5], HIGH);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[6], HIGH);
    digitalWrite(Ring_Chevrons[5], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[6], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[1], HIGH);
    delay(rnd);
    sgaDL = 3;
  }

   // -------------- dial 3 -------------
  else if (sgaDL == 3){
    digitalWrite(Ring_Chevrons[2], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[3], HIGH);
    digitalWrite(Ring_Chevrons[2], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[5], HIGH);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[6], HIGH);
    digitalWrite(Ring_Chevrons[5], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[6], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[2], HIGH);
    delay(rnd); 
    sgaDL = 4;
  }

  // -------------- dial 4 -------------
  else if (sgaDL == 4){
    digitalWrite(Ring_Chevrons[3], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[5], HIGH);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[6], HIGH);
    digitalWrite(Ring_Chevrons[5], LOW); 
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[6], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[5], HIGH);
    delay(rnd);
    sgaDL = 5;
  } 

  // -------------- dial 5 -------------
  else if(sgaDL == 5){
    digitalWrite(Ring_Chevrons[3], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[6], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[6], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[6], HIGH);
    delay(rnd); 
    sgaDL = 6;
  }

  // -------------- dial 6 -------------
  else if(sgaDL == 6){
    digitalWrite(Ring_Chevrons[3], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[7], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[7], LOW);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[7], HIGH);
    delay(rnd);
    sgaDL = 7;
  }  

  // -------------- dial 7 -------------
  else if(sgaDL == 7){
    digitalWrite(Ring_Chevrons[3], HIGH);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[4], LOW);
    delay(sga);
    digitalWrite(Ring_Chevrons[8], HIGH);
    delay(sga8);
    digitalWrite(Ring_Chevrons[8], LOW);
    delay(sgad);
    
    digitalWrite(Ring_Chevrons[3], HIGH);
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[8], HIGH);
    delay(rnd);  
  
      while (fc < 4){
        digitalWrite(Ring_Chevrons[0], LOW);
        digitalWrite(Ring_Chevrons[1], LOW);
        digitalWrite(Ring_Chevrons[2], LOW);
        digitalWrite(Ring_Chevrons[3], LOW);
        digitalWrite(Ring_Chevrons[4], LOW);
        digitalWrite(Ring_Chevrons[5], LOW);
        digitalWrite(Ring_Chevrons[6], LOW);
        digitalWrite(Ring_Chevrons[7], LOW);
        digitalWrite(Ring_Chevrons[8], LOW);    
        delay(300);
        digitalWrite(Ring_Chevrons[0], HIGH);
        digitalWrite(Ring_Chevrons[1], HIGH);
        digitalWrite(Ring_Chevrons[2], HIGH);
        digitalWrite(Ring_Chevrons[3], HIGH);
        digitalWrite(Ring_Chevrons[4], HIGH);
        digitalWrite(Ring_Chevrons[5], HIGH);
        digitalWrite(Ring_Chevrons[6], HIGH);
        digitalWrite(Ring_Chevrons[7], HIGH);
        digitalWrite(Ring_Chevrons[8], HIGH);
        delay(300);  
        fc++;
      }
      
        digitalWrite(Ring_Chevrons[0], LOW);
        digitalWrite(Ring_Chevrons[1], LOW);
        digitalWrite(Ring_Chevrons[2], LOW);
        digitalWrite(Ring_Chevrons[3], LOW);
        digitalWrite(Ring_Chevrons[4], LOW);
        digitalWrite(Ring_Chevrons[5], LOW);
        digitalWrite(Ring_Chevrons[6], LOW);
        digitalWrite(Ring_Chevrons[7], LOW);
        digitalWrite(Ring_Chevrons[8], LOW); 
        delay(1500);
       // digitalWrite(Ramp_Lights, LOW);
        fc = 0; 
        rd = 50;     
        sgaDL = 1;
        Ring = 0;
        delay(1620);        
    }
}

void ring_chase_lights(int Ring_Delay) {
  if (Ring < 10){
    digitalWrite(Ring_Chevrons[(Ring - 1)], HIGH);
  }
  else{
    digitalWrite(Ring_Chevrons[(Ring - 10)], LOW);
  }
  delay(Ring_Delay);
}

void ring_loop(int Ring_Delay) {
  if (Ring == 1){
    digitalWrite(Ring_Chevrons[0], HIGH);
    digitalWrite(Ring_Chevrons[8], HIGH);
    digitalWrite(Ring_Chevrons[1], LOW);
    digitalWrite(Ring_Chevrons[7], LOW);
  }
  else if ((Ring == 2) || (Ring == 8)){
    digitalWrite(Ring_Chevrons[1], HIGH);
    digitalWrite(Ring_Chevrons[7], HIGH);
    digitalWrite(Ring_Chevrons[0], LOW);
    digitalWrite(Ring_Chevrons[2], LOW);
    digitalWrite(Ring_Chevrons[6], LOW);
    digitalWrite(Ring_Chevrons[8], LOW);
  }
  else if ((Ring == 3) || (Ring == 7)){
    digitalWrite(Ring_Chevrons[2], HIGH);
    digitalWrite(Ring_Chevrons[6], HIGH);
    digitalWrite(Ring_Chevrons[1], LOW);
    digitalWrite(Ring_Chevrons[3], LOW);
    digitalWrite(Ring_Chevrons[5], LOW);
    digitalWrite(Ring_Chevrons[7], LOW);
  }
  else if ((Ring == 4) || (Ring == 6)){
    digitalWrite(Ring_Chevrons[3], HIGH);
    digitalWrite(Ring_Chevrons[5], HIGH);
    digitalWrite(Ring_Chevrons[2], LOW);
    digitalWrite(Ring_Chevrons[4], LOW);
    digitalWrite(Ring_Chevrons[6], LOW);
  }
  else if (Ring == 5){
    digitalWrite(Ring_Chevrons[4], HIGH);
    digitalWrite(Ring_Chevrons[3], LOW);
    digitalWrite(Ring_Chevrons[5], LOW); 
  }
  delay(Ring_Delay);
}

void ring_dial(int Ring_Delay) {
    //digitalWrite(Ramp_Lights, HIGH);
    //delay(1000);
    //----------------- dial 1
    if (sgc == 1){
      delay(1620);
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[0], HIGH);
      delay(rnd);
      sgc = 2;
    }
    //----------------- dial 2
    else if(sgc == 2){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[1], HIGH);
      delay(rnd);
      sgc = 3;
    }
    //----------------- dial 3
    else if(sgc == 3){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[2], HIGH);
      delay(rnd);
      sgc = 4;
    }
    //----------------- dial 4
    else if(sgc == 4){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[5], HIGH);
      delay(rnd);  
      sgc = 5;
    }      
    //----------------- dial 5
    else if(sgc == 5){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[6], HIGH);
      delay(rnd);
      sgc = 6;
    }
    //----------------- dial 6
    else if(sgc == 6){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[rc], LOW);
      delay(rdd);
      digitalWrite(Ring_Chevrons[7], HIGH);
      delay(rnd);  
      sgc = 7;
    }    
    //----------------- dial 7
    else if(sgc == 7){
      digitalWrite(Ring_Chevrons[rc], HIGH);
      delay(rld);
      digitalWrite(Ring_Chevrons[3], HIGH);
      digitalWrite(Ring_Chevrons[4], HIGH);
  
      delay(1620);
  
      while (fc < 4){
        digitalWrite(Ring_Chevrons[0], LOW);
        digitalWrite(Ring_Chevrons[1], LOW);
        digitalWrite(Ring_Chevrons[2], LOW);
        digitalWrite(Ring_Chevrons[3], LOW);
        digitalWrite(Ring_Chevrons[4], LOW);
        digitalWrite(Ring_Chevrons[5], LOW);
        digitalWrite(Ring_Chevrons[6], LOW);
        digitalWrite(Ring_Chevrons[7], LOW);
        digitalWrite(Ring_Chevrons[8], LOW);    
        delay(300);
        digitalWrite(Ring_Chevrons[0], HIGH);
        digitalWrite(Ring_Chevrons[1], HIGH);
        digitalWrite(Ring_Chevrons[2], HIGH);
        digitalWrite(Ring_Chevrons[3], HIGH);
        digitalWrite(Ring_Chevrons[4], HIGH);
        digitalWrite(Ring_Chevrons[5], HIGH);
        digitalWrite(Ring_Chevrons[6], HIGH);
        digitalWrite(Ring_Chevrons[7], HIGH);
        digitalWrite(Ring_Chevrons[8], HIGH);
        delay(300);  
  
        fc++;
      }
      
        digitalWrite(Ring_Chevrons[0], LOW);
        digitalWrite(Ring_Chevrons[1], LOW);
        digitalWrite(Ring_Chevrons[2], LOW);
        digitalWrite(Ring_Chevrons[3], LOW);
        digitalWrite(Ring_Chevrons[4], LOW);
        digitalWrite(Ring_Chevrons[5], LOW);
        digitalWrite(Ring_Chevrons[6], LOW);
        digitalWrite(Ring_Chevrons[7], LOW);
        digitalWrite(Ring_Chevrons[8], LOW); 
        delay(1500);
        //digitalWrite(Ramp_Lights, LOW);
        fc = 0; 
        rd = 50;
        sgc = 1;
        Ring = 0;
        delay(1620);
    }     
}

void ring_lights_random(int Ring_Delay){
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevrons[tmp_chevron], LOW);
  }
  digitalWrite(Ring_Chevrons[random(9)], HIGH);
  delay(Ring_Delay);
}

void ring_lights_snake(int Ring_Delay){
  int tmp_Ring = Ring - 4;
  if (tmp_Ring <= 0){
    tmp_Ring = 9+ tmp_Ring;
  }
  digitalWrite(Ring_Chevrons[(tmp_Ring - 1)], LOW);
  if (Ring <= 9){
    digitalWrite(Ring_Chevrons[(Ring - 1)], HIGH);
  }
  delay(Ring_Delay);
}

void ring_lights_random_triangle(int Ring_Delay){
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevrons[tmp_chevron], LOW);
  }
  int tmp_Ring = random(1, 9);
  int tmp_Ring_2 = tmp_Ring - 3;
  int tmp_Ring_3 = tmp_Ring + 3;
  if (tmp_Ring_2 <= 0 ){
    tmp_Ring_2 = 9 + tmp_Ring_2;
  }
  if (tmp_Ring_3 >= 10 ){
    tmp_Ring_3 = tmp_Ring_3 - 9;
  }
  digitalWrite(Ring_Chevrons[(tmp_Ring - 1)], HIGH);
  digitalWrite(Ring_Chevrons[(tmp_Ring_2 - 1)], HIGH);
  digitalWrite(Ring_Chevrons[(tmp_Ring_3 - 1)], HIGH);
  delay(Ring_Delay);
}

void ring_lights_triangle(int Ring_Delay){
  for (int tmp_chevron = 0; tmp_chevron < 9; tmp_chevron ++){
    digitalWrite(Chevrons[tmp_chevron], LOW);
  }
  int tmp_Ring_2 = Ring - 3;
  int tmp_Ring_3 = Ring + 3;
  if (tmp_Ring_2 <= 0 ){
    tmp_Ring_2 = 9 + tmp_Ring_2;
  }
  if (tmp_Ring_3 >= 10 ){
    tmp_Ring_3 = tmp_Ring_3 - 9;
  }
  digitalWrite(Ring_Chevrons[(Ring - 1)], HIGH);
  digitalWrite(Ring_Chevrons[(tmp_Ring_2 - 1)], HIGH);
  digitalWrite(Ring_Chevrons[(tmp_Ring_3 - 1)], HIGH);
  delay(Ring_Delay);
}
