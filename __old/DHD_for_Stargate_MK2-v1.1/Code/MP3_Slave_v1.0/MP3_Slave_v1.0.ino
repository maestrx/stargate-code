/*
  Script by Bob Urban. The code in here is released under beer license.
  If you use it and meet me, you owe me a beer.

  This code is for the Arduino Uno running the Sound Effects with a SparkFun MP3 Player Shield .
  There are three arduinos total for this DHD/Stargate/SFX version.

  The code for each arduino:
    DHD_Keyboard_Master_v1.0.ino
    Stargate_Slave_v1.0.ino 
    MP3_Slave_v1.0.ino   (this file)

  You may contact me at bob@schoolofawareness.com if you have any questions or improvements.    
*/

#include <Wire.h>
#include <SPI.h>           
#include <SdFat.h>         
//#include <SdFatUtil.h>     
#include <SFEMP3Shield.h>  

SdFat sd; // Create object to handle SD functions

SFEMP3Shield MP3player; /* Create Mp3 library object
   These variables are used in the MP3 initialization to set up
   some stereo options: */
   
const uint8_t volume = 10; // MP3 Player volume 0=max, 255=lowest (off)
const uint16_t monoMode = 1;  // Mono setting 0=off, 3=max

int x = 0; // represnts the DHD's triggers

void setup() {
  Wire.begin(9); // Start the I2C Bus as Slave on address 9
  Wire.onReceive(receiveEvent1);  // listen for SFX trigger instructions

  Serial.begin(9600);

  initSD();  // Initialize the SD card
  initMP3Player(); // Initialize the MP3 Shield
}

void receiveEvent1(int bytes) {
  x = Wire.read();    // listen for commands from the DHD
}

void loop() {
  if (x == 0){// Listen for a command to stop playing from DHD or gate
    MP3player.stopTrack();   
  }

  else if (x == 1) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(1); // Gate start and turn
    }
  }  
  else if (x == 2) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(2); // Chevron Lock
    }
  }    
  else if (x == 3) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(3); // Wormhole Activate
    }

  }    
  else if (x == 4) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(4); // Wormhole De-activate
    }
  }   
  else if (x == 5) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(5); // Event Horizon (puddle sfx)
    }
  }       
  else if (x == 6) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(6); // DHD 1st Address
    }
  }
  else if (x == 7) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(7); // DHD 2nd Address
    }
  }
  else if (x == 8) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(8); // DHD 3rd Address
    }
  }
  else if (x == 9) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(9); // DHD 4th Address
    }
  }    
  else if (x == 10) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(10); // DHD 5th Address
    }
  }   
  else if (x == 11) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(11); // DHD 6th Address
    }
  }   
  else if (x == 12) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(12); // DHD 7th Address
    }
  }   
  else if (x == 13) {
    if (x != 0 && !MP3player.isPlaying()) { 
      uint8_t result = MP3player.playTrack(13); // DHD red button's LEDs activate
    }
  }                   
}

//  The script below was taken from the mp3 library's sample code:

// initSD() initializes the SD card and checks for an error.
void initSD() {
  //Initialize the SdCard.
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) 
    sd.initErrorHalt();
  if(!sd.chdir("/")) 
    sd.errorHalt("sd.chdir");
}

/* Below: initMP3Player() sets up all of the initialization for the
   MP3 Player Shield. It runs the begin() function, checks
   for errors, applies a patch if found, and sets the volume/
   stereo mode. */
   
void initMP3Player() {  
  uint8_t result = MP3player.begin(); // init the mp3 player shield
  
  if(result != 0) {  // check result, see readme for error codes.
    // Error checking can go here!
  }
  
  MP3player.setVolume(volume, volume);
  MP3player.setMonoMode(monoMode);
}
