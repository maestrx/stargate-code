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


ArduinoQueue<i2c_message> i2c_message_queue_in(32);
ArduinoQueue<i2c_message> i2c_message_queue_out(32);

Timer t;

uint32_t last_keypress_millis = 0;


#include <SoftwareSerial.h>
SoftwareSerial bluetooth(10, 11);


int address_queue[8];             // 7 symbols, 8th is red, 9th is valid address validation
int address_queue_index = 0;
int valid_address_list[][7] = {
  {1,2 ,3,4,5,6,7 },   // Abydos
  {7 ,16,24,28,6 ,10,1 },   // Test
};
const int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array
unsigned long address_last_key_millis = millis();
const long address_key_input_timeout = 10000;

void setup(){
  Serial.begin(115200);

  bluetooth.begin(9600);
  
  while(!Serial);
  Serial << "+++ Setup start" << endl;
  Wire.begin(); // Start the I2C Bus as MASTER
  pinMode(A0, INPUT);

  t.every(500, i2c_recieve);
  t.every(500, i2c_send);
  Serial << "* Setup done" << endl;



  Serial << "LED mode set" << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }
  Serial << "LED OFF" << endl;
  for (int i = 0; i < 9; i ++){
    digitalWrite(dLED[i], LOW);  // turn all 8 LEDs off
  }

}


void loop(){
  t.update();

  if (last_keypress_millis + 500 < millis() ){
    if (analogRead(0) < 950){
      Serial << "+++ Key pressed" << endl;
      last_keypress_millis = millis();
      readKey();
    }
  }  

  if (bluetooth.available() > 0) {
    uint8_t inchar = bluetooth.read();
    if (48 <= inchar and inchar <= 57){
      Serial << "In charX:" << inchar << endl;
      uint8_t code = inchar - 48;
      if (inchar == 48){
        code = 99;
      }
      Serial << "Button code: " << code << endl;
      processKey(code);
    }
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
    i2c_message_out.action = 22;
    i2c_message_out.chevron = 0;
    i2c_message_queue_out.enqueue(i2c_message_out);
    Serial << "* Symbol command sent to gate" << endl;    
}

void readKey(){
  int keypress0 = analogRead(0);

  uint8_t code = 0;

  if (keypress0 < 70){
    code = 10;
  } else if (keypress0 < 200){
    code = 11;
  } else if (keypress0 < 300){
    code = 12;
  } else if (keypress0 < 500){
    code = 13;
  } else if (keypress0 < 700){
    code = 99;
  }
  Serial << "Button code: " << code << endl;   
  processKey(code);
}

void processKey(uint8_t symbol){
  Serial << "* address_queue_index:" << address_queue_index << endl;
  Serial << "* symbol:" << symbol << endl;

  if (address_queue_index > 7){ // ignore if anything after 7th symbol and red button
    Serial << "* ignoring after 7th index:" << address_queue_index << endl;
    address_last_key_millis = millis();
    return;
  } else if (address_queue_index == 7 and symbol != 99){ // ignore if anything else than red button after 7th symbol
    Serial << "* ignoring 7th index if not red:" << address_queue_index << " Symbol:" << symbol << endl;
    address_last_key_millis = millis();
    return;
  }else if (address_queue_index != 7 and symbol == 99){
    Serial << "* ignoring red if not 7th index:" << address_queue_index << " Symbol:" << symbol << endl;
    address_last_key_millis = millis();
    return; // ignore red button if not after 7th symbol
  }

  // check if symbol not already in the address queue
  for (int i = 0; i < address_queue_index; i++){
    if (address_queue[i] == symbol){
      Serial << "* ignoring duplicate symbol" << symbol << endl;
      address_last_key_millis = millis();
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

  i2c_message_out.action = address_queue_index+1;
  i2c_message_out.chevron = symbol;
  i2c_message_queue_out.enqueue(i2c_message_out);
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
      address_last_key_millis = millis();
      resetDial();
      return;
    }
  }

  address_queue_index += 1;
  Serial << "* New address index:" << address_queue_index << endl;
  address_last_key_millis = millis();
}



void i2c_send(){
  if (i2c_message_queue_out.itemCount()) {
    Serial << "* Sending message from the queue to 8" << endl;
    i2c_message_send = i2c_message_queue_out.dequeue();
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
    Serial << "* Sending data to 8" << endl;
    Wire.beginTransmission(8);
    Wire.write((byte *)&i2c_message_send, sizeof(i2c_message));
    Wire.endTransmission();
  }else{
    // Serial << "* Nothing to send to 8" << endl;
    i2c_message_send.action = 0;
    i2c_message_send.chevron = 0;
  }


}

void i2c_recieve(){
  // Serial << "* Requesting data from 8" << endl;
  Wire.requestFrom(8, sizeof(i2c_message));
  while (Wire.available()) {
    Wire.readBytes((byte*)&i2c_message_recieve, sizeof(i2c_message));
  }
  if (i2c_message_recieve.action != 0){
    Serial << "Recieved: " << i2c_message_recieve.action << "/" << i2c_message_recieve.chevron << endl;
  }

}


