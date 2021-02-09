#include <ButtonEvents.h>
#include <Akta3d_Potentiometer.h>
#include "light-manager.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


// ----- BUTTONS ----------
#define PREV_BUTTON_PIN D0
ButtonEvents prevButton;

#define PLAY_BUTTON_PIN D1
ButtonEvents playButton;

#define NEXT_BUTTON_PIN D2
ButtonEvents nextButton;

#define LIGHT_MODE_BUTTON_PIN D3
ButtonEvents lightModeButton;

#define ALERT_BUTTON_PIN D4
ButtonEvents alertButton;

// ----- POTAR ----------
#define VOLUME_POT_PIN A0
#define MAX_VOLUME 30
Akta3d_Potentiometer volumePot(VOLUME_POT_PIN, MAX_VOLUME);

// ----- BUTTONS ----------
#define LED_PIN D8
#define NB_LED 30
LightManager lightManager(LED_PIN, NB_LED);

// ----- MP3 ----------
#define NB_MAX_MP3_ADVERT 3
#define NB_MAX_MP3_FOLDER 2
#define MP3_RX_PIN D7
#define MP3_TX_PIN D6
const bool MP3_START_AFTER_FAIL = true; // allow to force start after a fail from MP3 player
SoftwareSerial mp3Serial(MP3_RX_PIN, MP3_TX_PIN);
DFRobotDFPlayerMini mp3Player;
bool mp3Play = false;
bool loopMp3 = false;
int currentMp3Folder = 1;

void setup() {
  // Used for debugging messages
  Serial.begin(9600); 

  // attach buttons
  prevButton.attach(PREV_BUTTON_PIN ,INPUT );
  playButton.attach(PLAY_BUTTON_PIN ,INPUT );
  nextButton.attach(NEXT_BUTTON_PIN ,INPUT );
  lightModeButton.attach(LIGHT_MODE_BUTTON_PIN ,INPUT );
  alertButton.attach(ALERT_BUTTON_PIN ,INPUT );
  
  lightManager.setup();

  //Use softwareSerial to communicate with mp3.
  mp3Serial.begin(9600);
  if (!mp3Player.begin(mp3Serial)) {
    Serial.println(F("Unable to begin mp3 player :"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  mp3Player.setTimeOut(500); //Set serial communictaion time out 500ms
  mp3Player.outputDevice(DFPLAYER_DEVICE_SD);
  mp3Player.disableLoop(); //disable loop.
  mp3Player.volume(10);
  
  Serial.println("Setup OK");
}

void loop() {
  //Serial.println("loop");
  
  // update all buttons
  prevButton.update();
  
  playButton.update();
  nextButton.update();
  lightModeButton.update();
  alertButton.update();
  volumePot.update();

  if ( prevButton.held() ) {    
    currentMp3Folder -= 1;
    if(currentMp3Folder < 1) {
      currentMp3Folder = NB_MAX_MP3_FOLDER;
    }
    
    if(loopMp3) {      
      mp3Player.loopFolder(currentMp3Folder);
      //mp3Player.play(1);
    } else {
      mp3Player.playFolder(currentMp3Folder, 1);
    }
    
    Serial.print("Play folder ");
    Serial.println(currentMp3Folder);
  }
  if ( prevButton.tapped() ) {
    Serial.println("Play previous");
    mp3Player.previous();
  }
  if ( playButton.tapped() ) {
    mp3Play = !mp3Play;
    if(mp3Play) {
      mp3Player.start();      
      Serial.println("Play");
    } else {
      mp3Player.pause();
      Serial.println("Pause");
    }
  }
  if ( nextButton.held() ) {
    currentMp3Folder += 1;
    if(currentMp3Folder > NB_MAX_MP3_FOLDER) {
      currentMp3Folder = 1;
    }
    
    if(loopMp3) {      
      mp3Player.loopFolder(currentMp3Folder);
      //mp3Player.play(1);
    } else {
      mp3Player.playFolder(currentMp3Folder, 1);
    }
    
    Serial.print("Play folder ");
    Serial.println(currentMp3Folder);
  }
  if ( nextButton.tapped() ) {
    Serial.println("Play next");
    mp3Player.next();
  }
  if ( lightModeButton.tapped() ) {
    Serial.println("Change lightMode");

    // test change mode
    lightManager.nextMode();
  }
  if ( alertButton.held() ) {
    lightManager.setColor1({random(255), random(255), random(255)});
  }
  if ( alertButton.tapped() ) {
    Serial.println("Plat Alert");

    int randomAdvert = random(1, NB_MAX_MP3_ADVERT + 1);
    mp3Player.advertise(randomAdvert);
  }
  if ( volumePot.changed() ) {
    Serial.print("volumePot = ");
    Serial.println(volumePot.value());

    mp3Player.volume(volumePot.value());
  }

  lightManager.loop();

  if (mp3Player.available()) {
    printDetail(mp3Player.readType(), mp3Player.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      if(MP3_START_AFTER_FAIL && mp3Play) {
        mp3Player.start();
      }
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      if(MP3_START_AFTER_FAIL && mp3Play) {
        mp3Player.start();
      }
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      if(MP3_START_AFTER_FAIL && mp3Play) {
        mp3Player.start();
      }
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          if(MP3_START_AFTER_FAIL && mp3Play) {
            mp3Player.start();
          }
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
