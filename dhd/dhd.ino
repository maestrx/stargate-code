#define FAKE_GATE 1     // fake gate for testing

#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <TEvent.h>
#include <Timer.h>

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
    //   0 -> No Operation
    //   X -> Play sound X (1-14)
    //   50 -> Stop sounds
    //   --- GATE => DHD
    //   0 -> No Operation
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

i2c_message i2c_message_gate_send;
i2c_message i2c_message_gate_recieve;
i2c_message i2c_message_gate_out;
ArduinoQueue<i2c_message> i2c_message_queue_gate_out(4);

i2c_message i2c_message_mp3_send;
i2c_message i2c_message_mp3_recieve;
i2c_message i2c_message_mp3_out;
ArduinoQueue<i2c_message> i2c_message_queue_mp3_out(4);

Timer t;

#ifdef FAKE_GATE
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(10, 11);
#endif

// variables to read buttons and decode them
const byte keypad_input = A0;
unsigned long address_last_key_millis = 0;
const long address_key_input_timeout = 10000;
int symbol;

// variables to store pressed symbols and process them
int address_queue[8];           // 7 symbols, 8th is red
int address_queue_index = 0;    // index of the last symbol in the address queue

int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array

int valid_address_list[][7] = {
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
  {27,7 ,15,32,12,30,1 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
  {1,2 ,3,4,5,6,7 },        // bluetooth test
};

void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial << F("+++ Setup start") << endl;

  #ifdef FAKE_GATE
  bluetooth.begin(9600);
  while(!bluetooth);
  #endif

  pinMode(keypad_input, INPUT);
  Serial << F("LED mode set") << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT); // make the DHD LED pins outputs
  }
  Serial << F("LED OFF") << endl;
  for (int i = 0; i < 9; i ++){
    digitalWrite(dLED[i], LOW);  // turn all LEDs off
  }

  Wire.begin(); // Start the I2C Bus as MASTER
  t.every(500, i2c_send_gate);
  t.every(500, i2c_recieve_gate);
  t.every(500, i2c_send_mp3);
  t.every(5000, i2c_keepalive);

  Serial << F("* Setup done") << endl;
}

void loop(){
  t.update();

  if (address_last_key_millis + 500 < millis() ){
    if (analogRead(keypad_input) < 950){
      Serial << F("+++ Key pressed") << endl;
      address_last_key_millis = millis();
      readKey();
    }
  }

  #ifdef FAKE_GATE
  if (bluetooth.available() > 0) {
    uint8_t inchar = bluetooth.read();
    if (48 <= inchar and inchar <= 57){
      Serial << F("In charX:") << inchar << endl;
      uint8_t code = inchar - 48;
      if (inchar == 48){
        code = 99;
      }
      Serial << F("Button code: ") << code << endl;
      processKey(code);
    }
  }
  #endif

  if (address_last_key_millis > 0 && millis() - address_last_key_millis > address_key_input_timeout){
    // timeout
    Serial << F("- Key press timeout") << endl;
    resetDial();
  }
}

void resetDial(){
    Serial << F("--- Address sequence reset") << endl;
    address_queue_index = 0;
    address_last_key_millis = 0;
    for (int i = 0; i < 9; i ++){
      digitalWrite(dLED[i], LOW);  // turn all 8 LEDs off
    }
}

void readKey(){
  int symbol = 0;
  int keypress = analogRead(keypad_input);
  #ifdef FAKE_GATE
  if (keypress < 70){
    symbol = 10;
  } else if (keypress < 200){
    symbol = 11;
  } else if (keypress < 300){
    symbol = 12;
  } else if (keypress < 500){
    symbol = 13;
  } else if (keypress < 700){
    symbol = 99;
  }

  #else

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
  #endif
  Serial << F("Button symbol: ") << symbol << endl;
  processKey(symbol);
}

void processKey(uint8_t symbol){
  Serial << F("* address_queue_index:") << address_queue_index << endl;
  Serial << F("* symbol:") << symbol << endl;

  if (address_queue_index > 7){ // ignore if anything after 7th symbol and red button
    Serial << F("* ignoring after 7th index:") << address_queue_index << endl;
    address_last_key_millis = millis();
    return;
  } else if (address_queue_index == 7 and symbol != 99){ // ignore if anything else than red button after 7th symbol
    Serial << F("* ignoring 7th index if not red:") << address_queue_index << F(" Symbol:") << symbol << endl;
    address_last_key_millis = millis();
    return;
  }else if (address_queue_index != 7 and symbol == 99){
    Serial << F("* ignoring red if not 7th index:") << address_queue_index << F(" Symbol:") << symbol << endl;
    address_last_key_millis = millis();
    return; // ignore red button if not after 7th symbol
  }

  // check if symbol not already in the address queue
  for (int i = 0; i < address_queue_index; i++){
    if (address_queue[i] == symbol){
      Serial << F("* ignoring duplicate symbol") << symbol << endl;
      address_last_key_millis = millis();
      return;
    }
  }

  if (address_queue_index < 7){
    Serial << F("* LED ID:") << dLED[address_queue_index] << F(" ON ") << endl;
    digitalWrite(dLED[address_queue_index], HIGH);
  }else{
    Serial << F("- Skipping RED button LED at tyhis stage") << endl;
  }

  // play sounds for the symbol
  i2c_message_mp3_out.action = 6 + address_queue_index;
  i2c_message_mp3_out.chevron = 0;
  i2c_message_queue_mp3_out.enqueue(i2c_message_mp3_out);
  Serial << F("* Sent sound PLAY ID:") << (6 + address_queue_index) << endl;

  i2c_message_gate_out.action = address_queue_index+1;
  i2c_message_gate_out.chevron = symbol;
  i2c_message_queue_gate_out.enqueue(i2c_message_gate_out);
  Serial << F("* Symbol command sent to gate") << endl;

  address_queue[address_queue_index] = symbol;
  Serial << F("* Added symbol:") << symbol << F(" to address queue at index:") << address_queue_index << endl;

  if (address_queue_index == 7){
    // check if the address is valid
    Serial << F("* Verifying that the complete address is valid") << endl;
    bool addrvalid = false;
    for (int i = 0; i < sizeof(valid_address_list) / sizeof(valid_address_list[0]); i++){
      //Serial << F("* Comparing address: ") << address_queue[0] << F(":") << address_queue[1] << F(":") << address_queue[2] << F(":") << address_queue[3] << F(":") << address_queue[4] << F(":") << address_queue[5] << F(":") << address_queue[6] << F(":") << endl;
      //Serial << F("* Comparing address: ") << valid_address_list[i][0] << F(":") << valid_address_list[i][1] << F(":") << valid_address_list[i][2] << F(":") << valid_address_list[i][3] << F(":") << valid_address_list[i][4] << F(":") << valid_address_list[i][5] << F(":") << valid_address_list[i][6] << F(":") << endl;
      bool valid = true;
      for (int j = 0; j < 7; j++){
        if (address_queue[j] != valid_address_list[i][j]){
          // Serial << F("- addr symbol is invalid: ") << address_queue[j] << F("<>") << valid_address_list[i][j] << endl;
          valid = false;
        }
      }
      if (valid){
          // valid address
          Serial << F("* Address is valid") << endl;
          addrvalid = true;
          break;
      }
    }
    if (! addrvalid){
      Serial << F("- Address is INVALID") << endl;
      address_last_key_millis = millis();
      resetDial();
      return;
    }
  }

  address_queue_index += 1;
  Serial << F("* New address index:") << address_queue_index << endl;
  address_last_key_millis = millis();
}


void i2c_send_gate(){
  if (i2c_message_queue_gate_out.itemCount()) {
    Serial << F("* Sending message from the queue to gate") << endl;
    i2c_message_gate_send = i2c_message_queue_gate_out.dequeue();
    Wire.write((byte *)&i2c_message_gate_send, sizeof(i2c_message));
    Serial << F("* Sending data to gate") << endl;
    Wire.beginTransmission(8);
    Wire.write((byte *)&i2c_message_gate_send, sizeof(i2c_message));
    Wire.endTransmission();
  }
}

void i2c_send_mp3(){
  if (i2c_message_queue_mp3_out.itemCount()) {
    Serial << F("* Sending message from the queue to mp3") << endl;
    i2c_message_mp3_send = i2c_message_queue_mp3_out.dequeue();
    Serial << F("* Sending data to mp3") << endl;
    Wire.beginTransmission(9);
    Wire.write((byte *)&i2c_message_mp3_send, sizeof(i2c_message));
    Wire.endTransmission();
  }
}

void i2c_recieve_gate(){
  // Serial << F("* Requesting data from gate") << endl;
  Wire.requestFrom(8, sizeof(i2c_message));
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_gate_recieve, sizeof(i2c_message));
  }
  if (i2c_message_gate_recieve.action == 0){
    Serial << F("* Recieved keepalive from gate") << endl;

  } else if (i2c_message_gate_recieve.action == 0) {
    Serial << F("Recieved: ") << i2c_message_gate_recieve.action << F("/") << i2c_message_gate_recieve.chevron << endl;
  }
}

void i2c_recieve_mp3(){
  // Serial << F("* Requesting data from gate") << endl;
  Wire.requestFrom(9, sizeof(i2c_message));
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_mp3_recieve, sizeof(i2c_message));
  }
  if (i2c_message_gate_recieve.action != 0){
    Serial << F("Recieved: ") << i2c_message_gate_recieve.action << F("/") << i2c_message_gate_recieve.chevron << endl;
  }
}

void i2c_keepalive(){
  Serial << F("* Sending keepalive to mp3") << endl;
  i2c_message_mp3_out.action = 0;
  i2c_message_mp3_out.chevron = 0;
  i2c_message_queue_mp3_out.enqueue(i2c_message_mp3_out);

  Serial << F("* Sending keepalive to gate") << endl;
  i2c_message_gate_out.action = 0;
  i2c_message_gate_out.chevron = symbol;
  i2c_message_queue_gate_out.enqueue(i2c_message_gate_out);
}
