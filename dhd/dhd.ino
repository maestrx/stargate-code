#include <PrintStream.h>
#include <Wire.h>

#define SKIP_SOUNDS 1

// variables to read buttons and decode them
int keypress;           // We'll place our result here
int symbol;             // We'll place the symbol result here

int address_queue[8];  // 7 symbols, 8th is red, 9th is valid address validation
int address_queue_index = 0;
unsigned long address_last_key_millis = 0;
const long address_key_input_timeout = 10000;

int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array

int valid_address_list[][7] = {
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
};

void setup(){
  Serial.begin(115200);  // Ready the serial monitor, so that we can see the result
  while(!Serial);
  Serial << "+++ Setup start" << endl;

  Wire.begin(); // Start the I2C Bus as MASTER

  Serial << "LED mode set" << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }
  Serial << "LED OFF" << endl;
  for (int i = 0; i < 9; i ++){
    digitalWrite(dLED[i], LOW);  // turn all 8 LEDs off
  }

  Serial << "* Setup done" << endl;
}

void loop(){
  if (analogRead(0) < 950){
    Serial << "+++ Key pressed" << endl;
    readKey();
  }

  if (address_last_key_millis > 0 && millis() - address_last_key_millis > address_key_input_timeout){
    // timeout
    Serial << "- Key press timeout" << endl;
    resetDial();
  }
}

void resetDial(){
    Serial << "--- Address sequence reset" << endl;
    address_queue_index = 0;
    address_last_key_millis = 0;
    for (int i = 0; i < 9; i ++){
      digitalWrite(dLED[i], LOW);  // turn all 8 LEDs off
    }
}

void readKey(){
  keypress = analogRead(0);
  decode();
  Serial << "* address_queue_index:" << address_queue_index << endl;
  Serial << "* symbol:" << symbol << endl;

  if (address_queue_index > 7){ // ignore if anything after 7th symbol and red button
    Serial << "* ignoring after 7th index:" << address_queue_index << endl;
    address_last_key_millis = millis();
    delay(500);
    return;
  } else if (address_queue_index == 7 and symbol != 99){ // ignore if anything else than red button after 7th symbol
    Serial << "* ignoring 7th index if not red:" << address_queue_index << " Symbol:" << symbol << endl;
    address_last_key_millis = millis();
    delay(500);
    return;
  }else if (address_queue_index != 7 and symbol == 99){
    Serial << "* ignoring red if not 7th index:" << address_queue_index << " Symbol:" << symbol << endl;
    address_last_key_millis = millis();
    delay(500);
    return; // ignore red button if not after 7th symbol
  }

  // check if symbol not already in the address queue
  for (int i = 0; i < address_queue_index; i++){
    if (address_queue[i] == symbol){
      Serial << "* ignoring duplicate symbol" << symbol << endl;
      address_last_key_millis = millis();
      delay(500);
      return;
    }
  }


  if (address_queue_index < 7){
    Serial << "* LED ID:" << dLED[address_queue_index] << " ON " << endl;
    digitalWrite(dLED[address_queue_index], HIGH);
  }else{
    Serial << "- Skipping RED button LED at tyhis stage" << endl;
  }

  #ifndef SKIP_SOUNDS
    // play sounds for the symbol
    Wire.beginTransmission(9);
    Wire.write(0);                        // Stop any audio
    Wire.endTransmission();
    Serial << "* Sent sound STOP" << endl;

    Wire.beginTransmission(9);
    Wire.write(6 + address_queue_index);  // play sound effect
    Wire.endTransmission();
    Serial << "* Sent sound PLAY ID:" << (6 + address_queue_index) << endl;
  #endif

  Wire.beginTransmission(8);
  Wire.write(1 + address_queue_index);  // play sound effect
  Wire.write(symbol);
  Wire.endTransmission();
  Serial << "* Symbol command sent to gate" << endl;

  address_queue[address_queue_index] = symbol;
  Serial << "* Added symbol:" << symbol << " to address queue at index:" << address_queue_index << endl;

  if (address_queue_index == 7){
    // check if the address is valid
    Serial << "* Verifying that the complete address is valid" << endl;
    bool addrvalid = false;
    for (int i = 0; i < sizeof(valid_address_list) / sizeof(valid_address_list[0]); i++){
      //Serial << "* Comparing address: " << address_queue[0] << ":" << address_queue[1] << ":" << address_queue[2] << ":" << address_queue[3] << ":" << address_queue[4] << ":" << address_queue[5] << ":" << address_queue[6] << ":" << endl;
      //Serial << "* Comparing address: " << valid_address_list[i][0] << ":" << valid_address_list[i][1] << ":" << valid_address_list[i][2] << ":" << valid_address_list[i][3] << ":" << valid_address_list[i][4] << ":" << valid_address_list[i][5] << ":" << valid_address_list[i][6] << ":" << endl;
      bool valid = true;
      for (int j = 0; j < 7; j++){
        if (address_queue[j] != valid_address_list[i][j]){
          // Serial << "- addr symbol is invalid: " << address_queue[j] << "<>" << valid_address_list[i][j] << endl;
          valid = false;
        }
      }
      if (valid){
          // valid address
          Serial << "* Address is valid" << endl;
          addrvalid = true;
          break;
      }
    }
    if (! addrvalid){
      Serial << "- Address is INVALID" << endl;
      resetDial();
      delay(500);
      return;
    }
  }

  address_queue_index += 1;
  Serial << "* New address index:" << address_queue_index << endl;
  address_last_key_millis = millis();
  delay(500);
}

void decode(){      // this is where the symbols are decoded from the analog read
  if (keypress < 96){ // button 11
    symbol = 3;
  }
  else if (keypress < 150){ // button 35
    symbol = 31; // the symbol number on the gate
  }
  else if (keypress < 189){ // button 39
    symbol = 99; // DHD big red start button
  }
  else if (keypress < 232){ // button 34
    symbol = 27;
  }
  else if (keypress < 252){ // button 30
    symbol = 2;
  }
  else if (keypress < 290){ // button 38
    symbol = 30;
  }
  else if (keypress < 317){ // button 29
    symbol = 17;
  }
  else if (keypress < 323){ // button 33
    symbol = 8;
  }
  else if (keypress < 329){ // button 25
    symbol = 26;
  }
  else if (keypress < 382){ // button 24
    symbol = 39;
  }
  else if (keypress < 391){ // button 28
    symbol = 14;
  }
  else if (keypress < 404){ // button 37
    symbol = 35;
  }
  else if (keypress < 418){ // button 20
    symbol = 16;
  }
  else if (keypress < 429){ // button 32
    symbol = 36;
  }
  else if (keypress < 444){ // button 23
    symbol = 33;
  }
  else if (keypress < 459){ // button 19
    symbol = 22;
  }
  else if (keypress < 478){ // button 27
    symbol = 18;
  }
  else if (keypress < 486){ // button 15
    symbol = 5;
  }
  else if (keypress < 507){ // button 18
    symbol = 19;
  }
  else if (keypress < 519){ // button 22
    symbol = 10;
  }
  else if (keypress < 524){ // button 14
    symbol = 9;
  }
  else if (keypress < 550){ // button 36
    symbol = 15;
  }
  else if (keypress < 557){ // button 13
    symbol = 20;
  }
  else if (keypress < 564){ // button 31
    symbol = 25;
  }
  else if (keypress < 571){ // button 17
    symbol = 1;
  }
  else if (keypress < 594){ // button 26
    symbol = 23;
  }
  else if (keypress < 607){ // button 12
    symbol = 4;
  }
  else if (keypress < 619){ // button 21
    symbol = 28;
  }
  else if (keypress < 651){ // button 16
    symbol = 32;
  }
  else if (keypress < 680){ // button 10
    symbol = 21;
  }
  else if (keypress < 693){ // button 9
    symbol = 11;
  }
  else if (keypress < 711){ // button 8
    symbol = 37;
  }
  else if (keypress < 734){ // button 7
    symbol = 12;
  }
  else if (keypress < 771){ // button 6
    symbol = 34;
  }
  else if (keypress < 774){ // button 5
    symbol = 38;
  }
  else if (keypress < 781){ // button 4
    symbol = 29;
  }
  else if (keypress < 790){ // button 3
    symbol = 6;
  }
  else if (keypress < 804){ // button 2
    symbol = 24;
  }
  else if (keypress < 826){ // button 1
    symbol = 7;
  }
}

