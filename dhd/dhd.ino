#include <stargate.h>
#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>
#include <TEvent.h>
#include <Timer.h>

// i2c objects to read and send messages with Gate
i2c_message i2c_message_gate_send;
i2c_message i2c_message_gate_recieve;
i2c_message i2c_message_gate_out;
ArduinoQueue<i2c_message> i2c_message_queue_gate_out(4);

// i2c objects to read and send messages with MP3
i2c_message i2c_message_mp3_send;
i2c_message i2c_message_mp3_recieve;
i2c_message i2c_message_mp3_out;
ArduinoQueue<i2c_message> i2c_message_queue_mp3_out(4);

// I2C timeout variables
unsigned long i2c_gate_last_alive = 0;
bool i2c_gate_alive = false;
unsigned long i2c_mp3_last_alive = 0;
bool i2c_mp3_alive = false;
const uint16_t i2c_timeout = 5000;

Timer t;

// Connect Bluetooth to the DHD for symbol input
#ifdef FAKE_GATE
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(11, 10);
#endif

// timing of the dhd key input reset
unsigned long address_last_key_millis = 0;

// variables to store pressed symbols and process them
int address_queue[8];           // 7 symbols, 8th is red
int address_queue_index = 0;    // index of the last symbol in the address queue

// list of valida addresses
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

  // Init bluetooth
  #ifdef FAKE_GATE
  bluetooth.begin(9600);
  while(!bluetooth);
  #endif

  // set pin modes
  pinMode(KEYPAD_INPUT, INPUT);
  Serial << F("* Set PIN modes") << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(DHD_Chevron_LED[i], OUTPUT); // make the DHD LED pins outputs
  }
  Serial << F("* LED OFF") << endl;
  for (int i = 0; i < 9; i ++){
    digitalWrite(DHD_Chevron_LED[i], LOW);  // turn all LEDs off
  }

  // Init I2C bus as MASTER and schedule the I2C push/pull tasks
  Serial << F("* I2C init") << endl;
  Wire.begin();
  t.every(500, i2c_send_gate);
  t.every(500, i2c_recieve_gate);
  t.every(500, i2c_send_mp3);
  t.every(500, i2c_recieve_mp3);
  t.every(5000, i2c_check_timeout);

  // reset teh game to default
  Serial << F("* HDH reset") << endl;
  resetDHD();

  // DHD ready
  Serial << F("+++ Setup done") << endl;
}

void loop(){
  // trigger timer events
  t.update();

  // read keypad input twice a second
  if (address_last_key_millis + 500 < millis() ){
    // read the raw value
    uint16_t keypress = analogRead(KEYPAD_INPUT);

    // consider it a valid keypress as long as the value is lower than the defined treshold
    if (keypress < KEYPRESS_RAW_THRESHOLD){
      Serial << F("+++ Key pressed") << endl;
      // start the key input timer
      address_last_key_millis = millis();
      // decode the raw read value to symbol ID
      readKey(keypress);
    }
  }

  // process the bluetooth input
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

  // reset the DHD to default in case no keypress was recieved in defined timeframe
  if (address_last_key_millis > 0 && millis() - address_last_key_millis > KEYPRESS_TIMEOUT){
    Serial << F("- Key press timeout. Doing reset!") << endl;
    resetDHD();
  }
}

void resetDHD(){
  Serial << F("+++ DHD reset") << endl;

  // reset key timeout timer
  address_last_key_millis = 0;
  address_queue_index = 0;

  // turn off all LEDs
  Serial << F("* Turn off all LEDs") << endl;
  for (int i = 0; i < 9; i ++){
    digitalWrite(DHD_Chevron_LED[i], LOW);
  }

  Serial << F("* Send reset command to gate") << endl;
  i2c_message_gate_out.action = ACTION_GATE_RESET;
  i2c_message_gate_out.chevron = 0;
  i2c_message_queue_gate_out.enqueue(i2c_message_gate_out);

  Serial << F("--- DHD reset done") << endl;
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
      Serial << F("* ignoring duplicate symbol: ") << symbol << endl;
      address_last_key_millis = millis();
      return;
    }
  }

  if (address_queue_index < 7){
    Serial << F("* LED ID: ") << DHD_Chevron_LED[address_queue_index] << F(" on index ") << address_queue_index << F(" is ON ") << endl;
    digitalWrite(DHD_Chevron_LED[address_queue_index], HIGH);
  }else{
    Serial << F("- Skipping RED button LED at this stage, to be turned on only when last chevron is in") << endl;
  }

  Serial << F("* Send push button to MP3") << endl;
  i2c_message_mp3_out.action = address_queue_index + 6; // offset 6 becouse thats where first button soudn starts
  i2c_message_mp3_out.chevron = 0;
  i2c_message_queue_mp3_out.enqueue(i2c_message_mp3_out);

  Serial << F("* Send symbol comand to gate") << endl;
  i2c_message_gate_out.action = address_queue_index+1;
  i2c_message_gate_out.chevron = symbol;
  i2c_message_queue_gate_out.enqueue(i2c_message_gate_out);

  Serial << F("* Add symbol:") << symbol << F(" to address queue at index:") << address_queue_index << endl;
  address_queue[address_queue_index] = symbol;

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
      resetDHD();
      return;
    }
  }

  address_queue_index += 1;
  Serial << F("* New address index:") << address_queue_index << endl;
  address_last_key_millis = millis();
}


void i2c_send_gate(){
  if (i2c_message_queue_gate_out.itemCount()) {
    DEBUG_I2C_GATE_DEV << F("i Sending message from the queue to gate") << endl;
    i2c_message_gate_send = i2c_message_queue_gate_out.dequeue();
    Wire.write((byte *)&i2c_message_gate_send, sizeof(i2c_message));
    DEBUG_I2C_GATE_DEV << F("i Sending data to gate") << endl;
    Wire.beginTransmission(8);
    Wire.write((byte *)&i2c_message_gate_send, sizeof(i2c_message));
    Wire.endTransmission();
  }
}

void i2c_send_mp3(){
  if (i2c_message_queue_mp3_out.itemCount()) {
    DEBUG_I2C_MP3_DEV << F("i Sending message from the queue to mp3") << endl;
    i2c_message_mp3_send = i2c_message_queue_mp3_out.dequeue();
    DEBUG_I2C_MP3_DEV << F("i Sending data to mp3") << endl;
    Wire.beginTransmission(9);
    Wire.write((byte *)&i2c_message_mp3_send, sizeof(i2c_message));
    Wire.endTransmission();
  }
}

void i2c_recieve_gate(){
  i2c_message_gate_recieve.action = ACTION_NODATA;
  DEBUG_I2C_GATE_DEV << F("i Requesting data from gate") << endl;
  Wire.requestFrom(8, sizeof(i2c_message));
  if (Wire.available()){
    Wire.readBytes((byte*)&i2c_message_gate_recieve, sizeof(i2c_message));
  }
  DEBUG_I2C_GATE_DEV << F("i MSG gate:") << i2c_message_gate_recieve.action << endl;
  if (i2c_message_gate_recieve.action == ACTION_NOOP){
    DEBUG_I2C_GATE_DEV << F("i Recieved keepalive from gate") << endl;
    i2c_gate_last_alive = millis();
    if (!i2c_gate_alive){
      DEBUG_I2C_GATE_DEV << F("i I2C connection with gate restored!") << endl;
    }
    i2c_gate_alive = true;

  } else if ( i2c_message_gate_recieve.action == ACTION_DIAL_START){
    DEBUG_I2C_GATE_DEV << F("i Recieved dial start of chevron: ") << i2c_message_gate_recieve.chevron << endl;
    // TODO: ?

    //Serial << F("* Send gate dial start to MP3") << endl;
    //i2c_message_mp3_out.action = MP3_GATE_DIALING;
    //i2c_message_mp3_out.chevron = 0;
    //i2c_message_queue_mp3_out.enqueue(i2c_message_mp3_out);

  } else if ( i2c_message_gate_recieve.action == ACTION_DIAL_END){
    DEBUG_I2C_GATE_DEV << F("i Recieved dial end") << endl;
    // TODO: ?

    //Serial << F("* Send chevron seal") << endl;
    //i2c_message_mp3_out.action = MP3_CHEVRON_SEAL;
    //i2c_message_mp3_out.chevron = 0;
    //i2c_message_queue_mp3_out.enqueue(i2c_message_mp3_out);

  } else if ( i2c_message_gate_recieve.action == ACTION_NODATA){
    DEBUG_I2C_GATE_DEV << F("i No data recieved from gate!") << endl;

  } else {
    Serial << F("!!! Recieved unknown message: ") << i2c_message_gate_recieve.action << F("/") << i2c_message_gate_recieve.chevron << endl;

  }
}

void i2c_recieve_mp3(){
  i2c_message_mp3_recieve.action = ACTION_NODATA;
  DEBUG_I2C_MP3_DEV << F("i Requesting data from mp3") << endl;
  Wire.requestFrom(8, sizeof(i2c_message));
  if (Wire.available()){
    Wire.readBytes((byte*)&i2c_message_mp3_recieve, sizeof(i2c_message));
  }
  DEBUG_I2C_MP3_DEV << F("i MSG mp3:") << i2c_message_mp3_recieve.action << endl;
  if (i2c_message_mp3_recieve.action == ACTION_NOOP){
    DEBUG_I2C_MP3_DEV << F("i Recieved keepalive from mp3") << endl;
    i2c_mp3_last_alive = millis();
    if (!i2c_mp3_alive){
      DEBUG_I2C_GATE_DEV << F("i I2C connection with mp3 restored!") << endl;
    }
    i2c_mp3_alive = true;
  } else if ( i2c_message_mp3_recieve.action == ACTION_NODATA){
    DEBUG_I2C_MP3_DEV << F("i No data recieved from mp3!") << endl;
  } else {
    Serial << F("Recieved: ") << i2c_message_mp3_recieve.action << F("/") << i2c_message_mp3_recieve.chevron << endl;
  }
}

void i2c_check_timeout(){
  if (i2c_gate_last_alive + i2c_timeout < millis()){
    Serial << F("! Gate I2C timeout") << endl;
    i2c_gate_alive = false;
  }
  if (i2c_mp3_last_alive + i2c_timeout < millis()){
    Serial << F("! MP3 I2C timeout") << endl;
    i2c_mp3_alive = false;
  }
}

void readKey(uint16_t keypress){
  int symbol = 0;
  Serial << F("Keypress: ") << keypress << endl;
  #ifdef FAKE_GATE
  if (keypress < 70){
    symbol = 1;
  } else if (keypress < 200){
    symbol = 3;
  } else if (keypress < 300){
    symbol = 5;
  } else if (keypress < 500){
    symbol = 7;
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

// #define DFPLAYER_RECEIVED_LENGTH 10
// #define DFPLAYER_SEND_LENGTH 10
// uint8_t _sending[DFPLAYER_SEND_LENGTH] = {0x7E, 0xFF, 06, 00, 01, 00, 00, 00, 00, 0xEF};
//
// void play(byte track){
//
//
//
//
// }