/*
TODO :
- LightMode in WebApp (shutdown should send OFF)

- Start and stop WIFI from button
- Add hardware connection (with buttons and without buttons)
- Add TFT Touchscreen connection with Arduino MEGA
*/
#define USE_WIFI          // if define, allow to control mp3 and lights from wifi. See nodeJs repo to have the webServer
#define USE_ELECTRONICS   // if define, allow to control mp3 and lights from hardware buttons and potentiometer

#include "light-manager.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#ifdef USE_WIFI
  #include <ESP8266WiFi.h>
  #include <ESPAsyncWebServer.h>

  #define SERVER_PORT 80
  #define WEBSOCKET_PATH    "/ws"  

  #define WIFI_SSID         "{ NEED CHANGE WITH YOUR WIFI SSID NAME }"
  #define WIFI_PASSWORD     "{ NEED CHANGE WITH YOUR WIFI PASSWORD }"
#endif

#ifdef USE_ELECTRONICS
  #include <ButtonEvents.h>
  #include <Akta3d_Potentiometer.h>
#endif 

// ----- LEDS ----------
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
bool mp3Started = false; // needed to know if mp3 player have already start play track since arduino start
bool mp3Playing = false; // needed to switch between "play" / "pause"
bool loopTrack = false;  // needed to switch between "loop current track" / "loop all track of the current directory"
uint16_t currentMp3Folder = 1;
uint16_t volume = 10;

// ----- shut down -------
bool shutDownActive = false;
uint16_t shutDownInMinutes = 15;
uint16_t shutDownTimerMillis = 0;

#ifdef USE_ELECTRONICS
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

  // ----- POTENTIOMETER ----------
  #define VOLUME_POT_PIN A0
  #define MAX_VOLUME 30
  Akta3d_Potentiometer volumePot(VOLUME_POT_PIN, 0, MAX_VOLUME);
#endif

  // ----- WIFI & WEBSOCKET ----------
#ifdef USE_WIFI
  AsyncWebServer server(SERVER_PORT);
  AsyncWebSocket ws(WEBSOCKET_PATH);
#endif

void setup() {
  // Used for debugging messages
  Serial.begin(9600); 

  Serial.println("");
  Serial.println("");
  Serial.println("Setup Start");

  lightManager.setup();

#ifdef USE_WIFI
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Init WebSocket
  Serial.println("Init WebSocket");
  initWebSocket();

  // Start server
  Serial.println("Start server");
  server.begin();
#endif

#ifdef USE_ELECTRONICS  
  // Attach Pin to buttons
  Serial.println("Attach Pin to buttons");
  prevButton.attach(PREV_BUTTON_PIN, INPUT );
  playButton.attach(PLAY_BUTTON_PIN, INPUT );
  nextButton.attach(NEXT_BUTTON_PIN, INPUT );
  lightModeButton.attach(LIGHT_MODE_BUTTON_PIN, INPUT );
  alertButton.attach(ALERT_BUTTON_PIN, INPUT );
#endif

  //Use softwareSerial to communicate with mp3.
  Serial.println("Start MP3");
    delay(500);
  mp3Serial.begin(9600);
  if (!mp3Player.begin(mp3Serial)) {
    Serial.println(F("Unable to begin mp3 player :"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  
  mp3Player.setTimeOut(500); //Set serial communictaion time out 500ms
  mp3Player.outputDevice(DFPLAYER_DEVICE_SD); 
  mp3Player.volume(volume);

  Serial.println("Setup OK");
}

void loop() {  
#ifdef USE_WIFI
  ws.cleanupClients(); 
#endif

#ifdef USE_ELECTRONICS  
  // update all buttons
  prevButton.update();  
  playButton.update();
  nextButton.update();
  lightModeButton.update();
  alertButton.update();
  volumePot.update();
  
  // PREV BUTTON
  // Tapped: prev track
  if (prevButton.tapped()) {
    playPreviousTrack();   
  }
  
  // PREV BUTTON
  // Held: prev folder
  if (prevButton.held()) {    
    playPreviousDirectory();
  }
  
  // PLAY/PAUSE BUTTON
  // Tapped: switch Play/pause
  if (playButton.tapped()) {
    switchPlayPause();     
  }
  
  // PLAY/PAUSE BUTTON
  // Held: change loop mode (loop single, loop directory)
  if (playButton.held()) {
    switchLoopMode();    
  }

  // NEXT BUTTON
  // Tapped: next track  
  if (nextButton.tapped()) {
    playNextTrack();   
  }
  
  // NEXT BUTTON
  // Held: next folder
  if (nextButton.held()) {
    playNextDirectory();     
  }
  
  // LIGHT BUTTON
  // Tapped: change lights mode 
  if (lightModeButton.tapped()) {
    nextLightsMode();
  }
  
  // LIGHT BUTTON
  // Held: chosse a radom color
  if ( lightModeButton.held() ) {
    setLightsModeRandomColor1();
  }

  // ALERT BUTTON
  // Tapped: play a random alert
  if (alertButton.tapped()) {
    playRandomAlert();        
  }
  
  // ALERT BUTTON
  // Held: Set timer to pause mp3 and set Light Off
  if (alertButton.held()) {
    switchShutDown();
  }

  // VOLUME
  if (volumePot.changed()) {
    setVolume(volumePot.value());
  }
#endif

  // LightManager need loop to change colors according lights mode
  lightManager.loop();

  // Print MP3 details
  if (mp3Player.available()) {
    printDetail(mp3Player.readType(), mp3Player.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

  // Auto shut down
  if(shutDownActive && (shutDownTimerMillis + (shutDownInMinutes * 1000 * 60) < millis())) {
    shutDownActive = false;    
    shutDown();
  }
}

/**
 Print MP3 details event
 
 On some error, we force play mp3
 On low powered MP3 player with some lights mode power can decrease and mp3 player fired error
*/
void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      if(MP3_START_AFTER_FAIL && mp3Playing) {
        mp3Player.start();
      }
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      if(MP3_START_AFTER_FAIL && mp3Playing) {
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
      if(MP3_START_AFTER_FAIL && mp3Playing) {
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
          if(MP3_START_AFTER_FAIL && mp3Playing) {
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

void notifyAllWsClients(String data) {
  #ifdef USE_WIFI
    // send data to all connected clients
    ws.textAll(data);
  #endif
}

#ifdef USE_WIFI
  void notifyWsClient(uint32_t clientId, String data) {
    ws.text(clientId, data);
  }

  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;

      String dataStr((char*)data);

      String action = dataStr;
      String value = "";
      int index = dataStr.indexOf(":");
      if(-1 != index) {
        action = dataStr.substring(0, index);
        value = dataStr.substring(index + 1);
      }

      Serial.print("Received event. Action: " + action);
      Serial.println(", Value: " + value);
      
      if(action == "setLightsModeColor1") { // value format = r,g,b
        int red   = 0;
        int green = 0;
        int blue  = 0;

        int pos = value.indexOf(',');
        if(pos != -1) {
          red = value.substring(0, pos).toInt() / 255.0 * 100;
          value = value.substring(pos+1);
        }
        pos = value.indexOf(',');
        if(pos != -1) {
          green = value.substring(0, pos).toInt() / 255.0 * 100;
          blue = value.substring(pos+1).toInt() / 255.0 * 100;
        }
        setLightsModeColor1({red, green, blue});
      }
      else if(action == "setLightsModeColor2") { // value format = r,g,b
        int red   = 0;
        int green = 0;
        int blue  = 0;

        int pos = value.indexOf(',');
        if(pos != -1) {
          red = value.substring(0, pos).toInt() / 255.0 * 100;
          value = value.substring(pos+1);
        }
        pos = value.indexOf(',');
        if(pos != -1) {
          green = value.substring(0, pos).toInt() / 255.0 * 100;
          blue = value.substring(pos+1).toInt() / 255.0 * 100;
        }
        setLightsModeColor2({red, green, blue});
      }
      else if(action == "setLightsModeParam") { // value should be an int
        setLightsModeParam(value.toInt());
      }
      else if(action == "nextLightsMode") { // no value
        nextLightsMode();
      }
      else if(action == "changeLightsMode") { // value should be an int and should be is managed by LightManager
        changeLightsMode(value.toInt());
      }
      else if(action == "playPreviousTrack") { // no value
        playPreviousTrack();
      }
      else if(action == "playPreviousDirectory") { // no value
        playPreviousDirectory();
      }
      else if(action == "switchPlayPause") { // no value
        switchPlayPause();
      }
      else if(action == "switchLoopMode") { // no value
        switchLoopMode();
      }
      else if(action == "playNextTrack") { // no value
        playNextTrack();
      }
      else if(action == "playNextDirectory") { // no value
        playNextTrack();
      }
      else if(action == "playRandomAlert") { // no value
        playRandomAlert();
      }
      else if(action == "setVolume") { // value should be an int [0, 30]
        setVolume(value.toInt());
      }
      else if(action == "switchShutDown") { // no value
        switchShutDown();
      }
      else if(action == "shutDown") { // no value
        shutDown();
      }
      else if(action == "shutDownMinutes") { // value should be an int
        setShutDownMinutes(value.toInt());
      }
      else {
        Serial.print("UNKNOW action : ");
        Serial.print(action);
        Serial.print(", value : ");
        Serial.println(value);
      } 
    }
  }

  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
              void *arg, uint8_t *data, size_t len) {
      uint32_t clientId = -1;
      RGB color1, color2;
      switch (type) {
        case WS_EVT_CONNECT:
          clientId = client->id();
          Serial.printf("WebSocket client #%u connected from %s\n", clientId, client->remoteIP().toString().c_str());
          
          // send all data to the new connected client
          color1 = lightManager.getColor1();
          color2 = lightManager.getColor2();
          notifyWsClient(clientId, "mp3Playing:" + String(mp3Playing));
          notifyWsClient(clientId, "currentMp3Folder:" + String(currentMp3Folder));
          notifyWsClient(clientId, "loopTrack:" + String(loopTrack));
          notifyWsClient(clientId, "lightsModeColor1:" + String(color1.r) + "," + String(color1.g) + "," + String(color1.b));
          notifyWsClient(clientId, "lightsModeColor2:" + String(color2.r) + "," + String(color2.g) + "," + String(color2.b));
          notifyWsClient(clientId, "lightsModeParam:" + String(lightManager.getParam()));
          notifyWsClient(clientId, "volume:" + String(volume));
          notifyWsClient(clientId, "shutDownActive:" + String(shutDownActive));
          notifyWsClient(clientId, "shutDownInMinutes:" + String(shutDownInMinutes));
          notifyWsClient(clientId, "shutDownTimerMillis:" + String(shutDownTimerMillis));
          break;
        case WS_EVT_DISCONNECT:
          Serial.printf("WebSocket client #%u disconnected\n", client->id());
          break;
        case WS_EVT_DATA:
          handleWebSocketMessage(arg, data, len);
          break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
          break;
    }
  }

  void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
  }
#endif

void playPreviousTrack() {
  Serial.println("Play previous"); 
  mp3Player.previous(); 
} 

void playPreviousDirectory() {
  currentMp3Folder -= 1;
  if(currentMp3Folder < 1) {
    currentMp3Folder = NB_MAX_MP3_FOLDER;
  }

  playOrLoopFirstTrack();

  Serial.print("Play folder ");
  Serial.println(currentMp3Folder);

  notifyAllWsClients("currentMp3Folder:" + String(currentMp3Folder));
}

void switchPlayPause() {
  mp3Playing = !mp3Playing;
  if(mp3Playing) {
    if(!mp3Started) {
      mp3Started = true;
      playOrLoopFirstTrack();
    } else {
      mp3Player.start();       
    }
    Serial.println("Play");
  } else {
    mp3Player.pause();
    Serial.println("Pause");
  }

  notifyAllWsClients("mp3Playing:" + String(mp3Playing));
}   

void switchLoopMode() {
  loopTrack = !loopTrack;
  if(loopTrack) {
    mp3Player.enableLoop();     
  } else { 
    playOrLoopFirstTrack();      
  }
  Serial.print("Loop mode : ");
  Serial.println(loopTrack ? "Loop track" : "Loop Directory");
  lightManager.displayAlert({0, 0, 255});

  notifyAllWsClients("loopTrack:" + String(loopTrack));
}

void playNextTrack() {
  Serial.println("Play next");
  mp3Player.next();
}

void playNextDirectory() {
  currentMp3Folder += 1;
  if(currentMp3Folder > NB_MAX_MP3_FOLDER) {
    currentMp3Folder = 1;
  }
  
  playOrLoopFirstTrack();
  
  Serial.print("Play folder ");
  Serial.println(currentMp3Folder);

  notifyAllWsClients("currentMp3Folder:" + String(currentMp3Folder));
}   

void nextLightsMode() {
  Serial.println("Next lightMode");
  lightManager.nextMode();
}

void changeLightsMode(int mode) {
  Serial.print("Change lightMode : ");
  Serial.println(mode);
  lightManager.changeMode(mode);
}

void setLightsModeRandomColor1() {
  Serial.println("Random color 1");   
  RGB color =  {random(255), random(255), random(255)};
  lightManager.setColor1(color);

  notifyAllWsClients("lightsModeColor1:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeColor1(RGB color) {
  Serial.print("Set color 1 : {");  
  Serial.println(color.r);
  Serial.print(", ");
  Serial.println(color.g);
  Serial.print(", ");
  Serial.print(color.b);
  Serial.println("}");
  
  lightManager.setColor1(color);

  notifyAllWsClients("lightsModeColor1:" + String(color.r) + String(color.g) + String(color.b));
}

void setLightsModeColor2(RGB color) {
  Serial.print("Set color 2 : {");  
  Serial.println(color.r);
  Serial.print(", ");
  Serial.println(color.g);
  Serial.print(", ");
  Serial.print(color.b);
  Serial.println("}");
  
  lightManager.setColor2(color);

  notifyAllWsClients("lightsModeColor2:" + String(color.r) + String(color.g) + String(color.b));
}

void setLightsModeParam(int value) {
  Serial.print("Set lights mode parameter : ");
  Serial.println(value);
  
  lightManager.setParam(value);

  notifyAllWsClients("lightsModeParam:" + String(value));
}

void playRandomAlert() {
  Serial.println("Play Alert");

  int randomAdvert = random(1, NB_MAX_MP3_ADVERT + 1); 
  mp3Player.advertise(randomAdvert); 
}  

void setVolume(uint16_t newVolume) {
  if(newVolume < 0) newVolume = 0;
  if(newVolume > 30) newVolume = 30;

  volume = newVolume;
  Serial.print("volumePot = ");
  Serial.println(volume);

  mp3Player.volume(volume);

  notifyAllWsClients("volume:" + String(volume));
}

void switchShutDown() {
  shutDownActive = !shutDownActive;
  Serial.print("Shut down : ");
  Serial.println(shutDownActive);

  if(shutDownActive) {
    shutDownTimerMillis = millis();
    lightManager.displayAlert({255, 0, 0});
  }
  else {      
    lightManager.displayAlert({0, 255, 0});
  }

  notifyAllWsClients("shutDownActive:" + String(shutDownActive));
  notifyAllWsClients("shutDownTimerMillis:" + String(shutDownTimerMillis));
}

void setShutDownMinutes(int value) {
  shutDownInMinutes = value;
  notifyAllWsClients("shutDownInMinutes:" + String(shutDownInMinutes));
}

void shutDown() {
  mp3Playing = false;
  mp3Player.pause();

  lightManager.changeMode(OFF);

  notifyAllWsClients("mp3Playing:" + String(mp3Playing));
}

/*
  According loop mode, play the first track of the current folder
*/
void playOrLoopFirstTrack() {
  if(loopTrack) {      
    mp3Player.playFolder(currentMp3Folder, 1);  
    mp3Player.enableLoop();        
  } else {
    mp3Player.loopFolder(currentMp3Folder);           
  }
}
