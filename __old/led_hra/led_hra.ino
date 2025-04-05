#include <EZButton.h>

#include <TEvent.h>
#include <Timer.h>

Timer t;

bool smer = true;
int svitici_led = 0;
int pristi_led = 0;
int8_t ledtimer = 0;
int rychlost = 100;

#include <EZButton.h>

#define BTN_1_PIN 12
#define BTN_1 0

void ReadButtons(bool *states, int num) {
  //Read all button states however you want
  states[BTN_1] = !digitalRead(BTN_1_PIN);
}

EZButton _ezb(1, ReadButtons, 2000, 500, 100);


void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  // put your setup code here, to run once:
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  ledtimer = t.every(500, dalsiLED);

  pinMode(BTN_1_PIN, INPUT_PULLUP);
  _ezb.Subscribe(BTN_1, ZmenSmer, RELEASED);
  _ezb.Subscribe(BTN_1, ZmenRychlost, HOLD);
}

void loop() {
  // put your main code here, to run repeatedly:
  t.update();
  _ezb.Loop();
}

void ZmenSmer() {
  smer = !smer;
  Serial.println("Smer");
  Serial.println(smer);
}

void ZmenRychlost() {
  Serial.println("Rychlost");
  t.stop(ledtimer);
  rychlost = rychlost+200;
  if (rychlost>1000){
    rychlost=100;
  }
  Serial.println(rychlost);
  ledtimer = t.every(rychlost, dalsiLED);
}


void dalsiLED() {
  if (smer){
    Serial.println("S +");
    pristi_led = svitici_led + 1;
    Serial.println(pristi_led);
  }else{
    Serial.println("S -");
    pristi_led = svitici_led - 1;
    Serial.println(pristi_led);
  }
  if (pristi_led<1){
    pristi_led=8;
  }else if (pristi_led>8){
    pristi_led=1;
  }
  svitici_led=pristi_led;
  Serial.println(svitici_led);
  nastavLED();
}


void nastavLED(){

  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(svitici_led+2, HIGH);

}