#include <ArduinoQueue.h>
#include <PrintStream.h>
#include <Wire.h>

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
i2c_message i2c_message_in;

ArduinoQueue<i2c_message> i2c_message_queue_in(32);
ArduinoQueue<i2c_message> i2c_message_queue_out(32);

const int dLED[] = {2,3,4,5,6,7,8,9,};  // LED pin array
unsigned long address_last_key_millis = 0;
const long address_key_input_timeout = 10000;

void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial << "+++ Setup start" << endl;

  Wire.begin(8);                  // Start the I2C Bus as SLAVE on address 8
  Wire.onReceive(i2c_recieve);  // Populate the variables heard from the DHD over the I2C
  Wire.onRequest(i2c_send);

  Serial << "* Setup done" << endl;


  Serial << "LED mode set" << endl;
  for (int i = 0; i < 9; i ++){
    pinMode(dLED[i], OUTPUT);   // turn GPIO pins 2 thru 9 to outputs
  }
  resetGate();

}

void loop(){
  process_in_queue();

  if (address_last_key_millis > 0 && millis() - address_last_key_millis > address_key_input_timeout){
    // timeout
    Serial << "- Gate timeout" << endl;
    resetGate();
  }
}

void resetGate(){
    Serial << "--- Address sequence reset" << endl;
    address_last_key_millis = 0;
    for (int i = 0; i < 9; i ++){
      digitalWrite(dLED[i], LOW);  // turn all 8 LEDs off
    }
}


void process_in_queue(){
  if (i2c_message_queue_in.itemCount()) {
    Serial << "* Processing message from DHD" << endl;
    i2c_message_in = i2c_message_queue_in.dequeue();
    Serial << "* Message details:" << i2c_message_in.action << "/" << i2c_message_in.chevron << endl;

    // incoming dial chevron
    if (i2c_message_in.action > 0 and i2c_message_in.action < 8){
      digitalWrite(dLED[i2c_message_in.action-1], HIGH);
      delay(5000);
    
    // valid address entered, establish gate
    } else if (i2c_message_in.action == 20){
      
    // INVALID address entered, reset gate
    } else if (i2c_message_in.action == 21){
      
    // reset gate (after stop button ?)
    } else if (i2c_message_in.action == 22){
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


