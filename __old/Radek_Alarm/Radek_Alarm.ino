#include <TEvent.h>
#include <Timer.h>

const int rele = 4;
const int sensor = 3;
const int led = 6;
const int buzzer = 8; 

Timer t;

bool strezeni_zapnuto = false;
bool probiha_poplach = false;

const unsigned long cas_pipnuti_buzzer = 1 * 1000;
const unsigned long cas_pipnuti_sireny = (unsigned long) 10 * 60 * 1000;

const unsigned long cas_spusteni_strezeni = 3 * 1000;
const unsigned long cas_cekani_po_otevreni_dveri_pred_spustenim_sireny = 5 * 1000;
const unsigned long doba_prniho_houkani_sireny = 10 * 1000;
const unsigned long doba_pauzy_po_prvnim_houkani = 10 * 1000;
const unsigned long doba_druheho_houkani_sireny = 10 * 1000;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  // nastaveni pipnu a vychozich hodnot
  pinMode(sensor, INPUT_PULLUP);
  pinMode(rele, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(rele, LOW);
  digitalWrite(led, LOW);
  digitalWrite(buzzer, LOW);

  // verinovy pipnuti potvrzujici start 
  digitalWrite(buzzer, HIGH);
  t.after(cas_pipnuti_buzzer, doStopBuzzer);
  Serial.println("Zapnuti pipnuti buzzeru");

  // prvnotni zpozdeni pred spustenim strezeni
  t.after(cas_spusteni_strezeni, doStartStrezeni);
  Serial.println("Cekani na start strezeni"); 
}

// vypnuti buzzeru
void doStopBuzzer() {
  digitalWrite(buzzer, LOW);
  Serial.println("Vypnuti buzzeru");
}

// zacni strezit
void doStartStrezeni() {
  strezeni_zapnuto = true;
  probiha_poplach = false;
  digitalWrite(rele, HIGH);
  digitalWrite(led, HIGH);  
  t.after(cas_pipnuti_sireny, doStopSirenaInit);
  Serial.println("Strezeni zapnuto");
}

// vypnuti orvotniho buzzeru
void doStopSirenaInit() {
  digitalWrite(rele, LOW);
  digitalWrite(led, LOW);  
  Serial.println("Konec prohouknuti sireny");
}

// prvni spusteni sireny po otevreni dveri
void doSpustSirenu() {
    digitalWrite(rele, HIGH);
    digitalWrite(led, HIGH);
    t.after(doba_prniho_houkani_sireny, doStopSirena);
    Serial.println("Prvni spusteni sireny");
}

// prvni vypnuti sireny po spusteni alarmu
void doStopSirena() {
  digitalWrite(rele, LOW);
  digitalWrite(led, LOW);
  t.after(doba_pauzy_po_prvnim_houkani, doSpustSirenu2);
  Serial.println("Prvni vypnuti sireny");
}

// druhe spusteni sireny po prvni pauze
void doSpustSirenu2() {
    digitalWrite(rele, HIGH);
    digitalWrite(led, HIGH);
    t.after(doba_druheho_houkani_sireny, doStopSirena2);
    Serial.println("Druhe spusteni sireny");
}

// druhe vypnuti sireny a zapnuti strezeni
void doStopSirena2() {
  digitalWrite(rele, LOW);
  digitalWrite(led, LOW);
  doStartStrezeni();
  Serial.println("Opetovny start strezeni");
}

// hlavni cyklus
void loop() {  
  t.update();

  if ( strezeni_zapnuto and !probiha_poplach ) {
    if (!digitalRead(sensor)){
      probiha_poplach = true;
      t.after(cas_cekani_po_otevreni_dveri_pred_spustenim_sireny, doSpustSirenu);
      Serial.println("Cekani na sirenu zacalo. Otevrene dvere?");
    }
  }
}
