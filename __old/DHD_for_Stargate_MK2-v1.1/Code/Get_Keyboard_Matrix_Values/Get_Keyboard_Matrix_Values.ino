/*  use this code to get the raw data from the keyboard matrix. 

    The results may fluctuate slightly when pressing the same key multiple times,
    so it is best to check each key 4 or 5 times and note the highest value,
    then add 2 or 3 to it's value in the DHD code. 
    example: If the 12th key returns the highest value being 603, then test for 606:

        else if (keypress < 606){ // button 12
          symbol = 4;
        }
        
    questions: bob@schoolofawareness.com
*/
int keypad;    
       
void setup(){
  Serial.begin(9600);  
}

void loop(){
  while (analogRead(0) > 950) {             
    {}
  }     
  Serial.print("The Result Is: ");     
  keypad = analogRead(0); 
  keyIsPressed();          // rem this out to get continuous data
  Serial.print(keypad);            
  Serial.println();
  delay(10);  
}

void keyIsPressed() {
  while (analogRead(0) < 950) {
    {}
  }
  delay(10);
}
