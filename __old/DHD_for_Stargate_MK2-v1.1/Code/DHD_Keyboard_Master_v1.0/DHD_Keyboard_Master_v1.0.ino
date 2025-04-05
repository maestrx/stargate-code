/*
  Script by Bob Urban. The code in here is released under beer license.
  If you use it and meet me, you owe me a beer.

  This code is for the Arduino nano running the DHD.
  There are three arduinos total for this DHD/Stargate/SFX version.

  The code for each arduino:
    DHD_Keyboard_Master_v1.0.ino (this file)
    Stargate_Slave_v1.0.ino 
    MP3_Slave_v1.0.ino

  You may contact me at bob@schoolofawareness.com if you have any questions or improvements.    
*/

#include <Wire.h>
int keypress;           // We'll place our result here
int symbol;          // We'll place the symbol result here

int dial = 1;   // sets which address # is current
int k1;   // k1 - k7 hold the gate addresses
int k2;
int k3;
int k4;
int k5;
int k6;
int k7;
int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array
int wake = 0;
int x = 0;

void setup(){
  Serial.begin(9600);  // Ready the serial monitor, so that we can see the result
  
  Wire.begin(); // Start the I2C Bus as MASTER
  
  for (int tmp_dLED = 0; tmp_dLED < 9; tmp_dLED ++){
    pinMode(dLED[tmp_dLED], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }
  for (int tmp_dLED = 0; tmp_dLED < 9; tmp_dLED ++){
    digitalWrite(dLED[tmp_dLED], LOW);  // turn all 8 LEDs off
  }  
}

void loop(){
  keypress = 0;  // reset variable
  keypress = analogRead(0);   // grab the current value from the keyboard matrix
  
  while (analogRead(0) > 950) {     // don't do anything until pressing a button is detected        
       {}
  }
  if (keypress < 950 && dial == 1){  // if a button is detected and it is the first address, do stuff
    
    delay (100);              // Once a keypress is detected, wait 100ms and grab the value again
    keypress = analogRead(0); // to avoid false readings from the initial press.
    
    decode(); // find out which button was pressed         
    x = 0;
    Wire.beginTransmission(9); 
    Wire.write(x);  // Stop any audio
    Wire.endTransmission();    
    delay(20); 
    x = 6;
    Wire.beginTransmission(9); 
    Wire.write(x);  // play sound effect
    Wire.endTransmission();    
    delay(85); 
    

    wake = 1;
    Wire.beginTransmission(8); 
    Wire.write(wake); 
    Wire.endTransmission();    
    delay(65); 
    Serial.println(wake);
    
    keyIsPressed();  // don't proceed until the button is let go (debouncer)
    k1 = symbol;  // assign the first symbol to the variable
    if (k1 != 420){ // check to make sure you didn't push the Big Red button
      digitalWrite(dLED[0], HIGH);  // turn on the first LED
      Serial.print("Key 1: ");   
      Serial.println(k1);             
        
      dial = 2;    // make it so the next button pressed will be for the 2nd address    
    }   
    else {
      dial = 1;  
      Serial.println("not big red yet ");                            
    } 
                       
  }
  else if (keypress < 950 && dial == 2){
    delay (100);
    keypress = analogRead(0);
    decode(); 
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 7;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);     
     
    keyIsPressed();
    k2 = symbol;
    if (k2 != k1 && k2 != 420){ // check to make sure you didn't press the red button or an address already selected
      digitalWrite(dLED[1], HIGH);
      Serial.print("Key 2: ");   
      Serial.println(k2);    
                 
      dial = 3;   
    }  
    else {
      dial = 2;  
      Serial.println("Duplicate or big red");                     
    }
  }
  else if (keypress < 950 && dial == 3){
    delay (100);
    keypress = analogRead(0);
    decode(); 
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 8;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);    
    keyIsPressed();
    k3 = symbol;
    if (k3 != k1 && k3 != k2 && k3 != 420){
      digitalWrite(dLED[2], HIGH);
      Serial.print("Key 3: ");   
      Serial.println(k3);  
      
      dial = 4;  
    }     
    else {
      dial = 3;  
      Serial.println("Duplicate or big red");                        
    }                                                       
  }
  else if (keypress < 950 && dial == 4){
    delay (100);
    keypress = analogRead(0);
    decode(); 
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 9;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);     
    keyIsPressed();
    k4 = symbol;
    if (k4 != k1 && k4 != k2 && k4 != k3 && k4 != 420){    
      digitalWrite(dLED[3], HIGH);
      Serial.print("Key 4: ");   
      Serial.println(k4);    
                 
      dial = 5; 
    }   
    else {
      dial = 4;   
      Serial.println("Duplicate or big red");                                                    
    }
  }
  else if (keypress < 950 && dial == 5){
    delay (100);
    keypress = analogRead(0);
    decode();     
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 10;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);     
    keyIsPressed();
    k5 = symbol;
    if (k5 != k1 && k5 != k2 && k5 != k3 && k5 != k4 && k5 != 420){
      digitalWrite(dLED[4], HIGH);
      Serial.print("Key 5: ");   
      Serial.println(k5);      
               
      dial = 6;  
    }  
    else {
      dial = 5;   
      Serial.println("Duplicate or big red");                                                       
    }                                                                   
  }
  else if (keypress < 950 && dial == 6){
    delay (100);
    keypress = analogRead(0);
    decode(); 
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 11;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);     
    keyIsPressed();
    k6 = symbol;
    if (k6 != k1 && k6 != k2 && k6 != k3 && k6 != k4 && k6 != k5 && k6 != 420){
      digitalWrite(dLED[5], HIGH);
      Serial.print("Key 6: ");   
      Serial.println(k6);   
                           
      dial = 7;  
    }  
    else {
      dial = 6;   
      Serial.println("Duplicate or big red");                                                      
    }                                                                         
  } 
  else if (keypress < 950 && dial == 7){
    delay (100);
    keypress = analogRead(0);
    decode();     
    x = 0; 
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20); 
    x = 12;
    Wire.beginTransmission(9); 
    Wire.write(x);
    Wire.endTransmission();    
    delay(20);     
    keyIsPressed();
    k7 = symbol;
    if (k7 != k1 && k7 != k2 && k7 != k3 && k7 != k4 && k7 != k5 && k7 != k6 && k7 != 420){
      digitalWrite(dLED[6], HIGH);
      Serial.print("Key 7: ");   
      Serial.println(k7);      
                     
      delay(1300);   // pause for drama

      x = 0; 
      Wire.beginTransmission(9); 
      Wire.write(x);
      Wire.endTransmission();    
      delay(20); 
      x = 13;
      Wire.beginTransmission(9); 
      Wire.write(x);
      Wire.endTransmission();    
      delay(1100);     
      digitalWrite(dLED[7], HIGH);  // big red button's led lights up  
      dial = 8;  
    } 
    else {
      dial = 7;   
      Serial.println("Duplicate or big red");                                                     
    }  
  }
  else if (keypress < 950 && dial == 8){
   decode(); 
    keyIsPressed();
    if (symbol == 420){  // make sure it's the big red middle button that is pushed
      int address[] = {k1,k2,k3,k4,k5,k6,k7}; // throw the 7 addresses into an array
      Serial.print("Gate Address: ");
      for(int tmp_address = 0; tmp_address < 7; tmp_address ++)
      {
        Serial.print(address[tmp_address]); // print address array
        Serial.print(", ");
      }
      Serial.println();
      int activate = 1;
      Wire.beginTransmission(8); // send the address to the gate, then activate the gate
      Wire.write(k1);
      Wire.write(k2); 
      Wire.write(k3);
      Wire.write(k4); 
      Wire.write(k5);
      Wire.write(k6); 
      Wire.write(k7);
      Wire.write(activate);
      Wire.endTransmission();    
      delay(65); 
      
      dial = 1;     // reset the DHD 
      delay(420);
      for (int tmp_dLED = 0; tmp_dLED < 9; tmp_dLED ++){ 
          digitalWrite(dLED[tmp_dLED], LOW);      // turn off all LEDs
      }  
      x = 0; 
      Wire.beginTransmission(9); 
      Wire.write(x); // stop all DHD sounds
      Wire.endTransmission();    
      delay(20);       
  /*    delay(3620);   
      activate = 0;
      Wire.beginTransmission(8);
      Wire.write(activate);
      Wire.endTransmission();  */
    }
   // delay(65);       
  }
}

void keyIsPressed(){
  while (analogRead(0) < 950) {   // hang out here until you let go fo the key press
    {}
  }
  delay(20);
}

void decode(){      // this is where the symbols are decoded from the analog read
  if (keypress < 146){ // button 35
    symbol = 31; // the symbol number on the gate
  }
  else if (keypress < 195){ // button 39
    symbol = 420; // DHD big red start button
  }
  else if (keypress < 228){ // button 34
    symbol = 27;
  }
  else if (keypress < 249){ // button 30
    symbol = 2;
  }
  else if (keypress < 288){ // button 38
    symbol = 30;
  }
  else if (keypress < 314){ // button 29
    symbol = 17;
  }
  else if (keypress < 320){ // button 33
    symbol = 8;
  }
  else if (keypress < 326){ // button 25
    symbol = 26;
  }
  else if (keypress < 379){ // button 24
    symbol = 39;
  }
  else if (keypress < 387){ // button 28
    symbol = 14;
  }
  else if (keypress < 402){ // button 37
    symbol = 35;
  }
  else if (keypress < 415){ // button 20
    symbol = 16;
  }
  else if (keypress < 426){ // button 32
    symbol = 36;
  }
  else if (keypress < 440){ // button 23
    symbol = 33;
  }
  else if (keypress < 456){ // button 19
    symbol = 22;
  }
  else if (keypress < 476){ // button 27
    symbol = 18;
  }
  else if (keypress < 484){ // button 15
    symbol = 5;
  }
  else if (keypress < 504){ // button 18
    symbol = 19;
  }
  else if (keypress < 516){ // button 22
    symbol = 10;
  }
  else if (keypress < 521){ // button 14
    symbol = 9;
  }
  else if (keypress < 547){ // button 36
    symbol = 15; 
  }
  else if (keypress < 555){ // button 13
    symbol = 20;
  }
  else if (keypress < 562){ // button 31
    symbol = 25;
  }
  else if (keypress < 569){ // button 17
    symbol = 1;
  }
  else if (keypress < 592){ // button 26
    symbol = 23;
  }
  else if (keypress < 606){ // button 12
    symbol = 4;
  }
  else if (keypress < 617){ // button 21
    symbol = 28;
  }
  else if (keypress < 650){ // button 16
    symbol = 32;
  }
  else if (keypress < 677){ // button 11
    symbol = 3;
  }
  else if (keypress < 680){ // button 10
    symbol = 21;
  }
  else if (keypress < 694){ // button 9
    symbol = 11;
  }
  else if (keypress < 711){ // button 8
    symbol = 37;
  }
  else if (keypress < 734){ // button 7
    symbol = 12;
  }
  else if (keypress < 770){ // button 6
    symbol = 34;
  }
  else if (keypress < 774){ // button 5
    symbol = 38;
  }
  else if (keypress < 781){ // button 4
    symbol = 29;
  }
  else if (keypress < 791){ // button 3
    symbol = 6;
  }
  else if (keypress < 804){ // button 2
    symbol = 24;
  }
  else if (keypress < 825){ // button 1
    symbol = 7;
  }
}












