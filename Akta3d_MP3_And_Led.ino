/*
TODO :
- Start and stop WIFI from button
- README : Add hardware connection (with buttons and without buttons)
- README : Add TFT Touchscreen connection with Arduino MEGA
*/
#define USE_WIFI          // if define, allow to control mp3 and lights from wifi. See nodeJs repo to have the webServer
#define USE_ELECTRONICS   // if define, allow to control mp3 and lights from hardware buttons and potentiometer
#define MP3
#define USE_MP3_BUTTONS
#define USE_LIGHTS
#define USE_LIGHTS_BUTTONS

#include "SoftwareSerial.h"

#ifdef USE_LIGHTS
  #include "lights-manager.h"
#endif

#ifdef USE_MP3
  #include "DFRobotDFPlayerMini.h"
#endif

#ifdef USE_WIFI
  #include <ESP8266WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <FS.h>

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
#ifdef USE_LIGHTS
  #define LED_PIN D8
  #define NB_LED 24
  LightsManager lightsManager(LED_PIN, NB_LED, NEO_BGR + NEO_KHZ800);
#endif

// ----- MP3 ----------
bool mp3Started = false; // needed to know if mp3 player have already start play track since arduino start
bool mp3Playing = false; // needed to switch between "play" / "pause"
bool loopTrack = false;  // needed to switch between "loop current track" / "loop all track of the current directory"
uint16_t currentMp3Folder = 1;
uint16_t volume = 10;
#define NB_MAX_MP3_ADVERT 3
#define NB_MAX_MP3_FOLDER 2
#ifdef MP3
  #define MP3_RX_PIN D7
  #define MP3_TX_PIN D6
  const bool MP3_START_AFTER_FAIL = true; // allow to force start after a fail from MP3 player
  SoftwareSerial mp3Serial(MP3_RX_PIN, MP3_TX_PIN);
  DFRobotDFPlayerMini mp3Player;
#endif
// ----- shut down -------
bool shutDownActive = false;
uint16_t shutDownInMinutes = 15;
uint16_t shutDownTimerMillis = 0;

#ifdef USE_ELECTRONICS
  // ----- BUTTONS ----------
  #ifdef USE_MP3_BUTTONS
    #define PREV_BUTTON_PIN D0
    ButtonEvents prevButton;

    #define PLAY_BUTTON_PIN D1
    ButtonEvents playButton;

    #define NEXT_BUTTON_PIN D2
    ButtonEvents nextButton;

    #define ALERT_BUTTON_PIN D4
    ButtonEvents alertButton;

    // ----- POTENTIOMETER ----------
    #define VOLUME_POT_PIN A0
    #define MAX_VOLUME 30
    Akta3d_Potentiometer volumePot(VOLUME_POT_PIN, 0, MAX_VOLUME, 0, 1024, 1 /* bCoeff */);
  #endif 

  #ifdef USE_LIGHTS_BUTTONS
    #define LIGHTS_MODE_BUTTON_PIN D3
    ButtonEvents lightsModeButton;
  #endif
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

  #ifdef USE_LIGHTS
    lightsManager.setup();
  #endif

  #ifdef USE_WIFI
    // Connect to Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    // Initialize SPIFFS
    if(!SPIFFS.begin()) {
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    // Init WebSocket
    Serial.println("Init WebSocket");
    initWebSocket();

    // Start server
    Serial.println("Start server on port : " + String(SERVER_PORT));
    
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html");
    });  

    // CAUTION : with this method all files on Arduino filesystem can be send
    // do not store confidential data in filesystem or use specific route to serve only desired files
    server.onNotFound([](AsyncWebServerRequest *request) {
      String path = request->url();

      if(!SPIFFS.exists(path)) {
        request->send(404);
      }
      
      String dataType = "text/plain";
      if(path.endsWith("/")) path += "index.htm";

      if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
      else if(path.endsWith(".html")) dataType = "text/html";
      else if(path.endsWith(".css")) dataType = "text/css";
      else if(path.endsWith(".xml")) dataType = "text/xml";
      else if(path.endsWith(".json")) dataType = "application/json";
      else if(path.endsWith(".js")) dataType = "application/javascript";
      else if(path.endsWith(".png")) dataType = "image/png";
      else if(path.endsWith(".ico")) dataType = "image/x-icon";

      request->send(SPIFFS, request->url(), dataType);
    });

    server.begin();
  #endif

  #ifdef USE_ELECTRONICS  
    // Attach Pin to buttons
    Serial.println("Attach Pin to buttons");
    #ifdef USE_MP3_BUTTONS
      prevButton.attach(PREV_BUTTON_PIN, INPUT );
      playButton.attach(PLAY_BUTTON_PIN, INPUT );
      nextButton.attach(NEXT_BUTTON_PIN, INPUT );
      alertButton.attach(ALERT_BUTTON_PIN, INPUT );
    #endif

    #ifdef USE_LIGHTS_BUTTONS
      lightsModeButton.attach(LIGHTS_MODE_BUTTON_PIN, INPUT );
    #endif
  #endif

  #ifdef MP3
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
  #endif

  Serial.println("Setup OK");
}

void loop() {  
  #ifdef USE_WIFI
    ws.cleanupClients(); 
  #endif

  #ifdef USE_ELECTRONICS  
    // update all buttons
    #ifdef USE_MP3_BUTTONS
      prevButton.update();  
      playButton.update();
      nextButton.update();
      alertButton.update();
      volumePot.update();
    #endif
    #ifdef USE_LIGHTS_BUTTONS
      lightsModeButton.update();
    #endif
    
    #ifdef USE_MP3_BUTTONS
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
    
      // ALERT BUTTON
      // Tapped: play a random alert
      if (alertButton.tapped()) {
        playRandomAlert();        
      }
      
      // ALERT BUTTON
      // Held: Set timer to pause mp3 and set Lights Off
      if (alertButton.held()) {
        switchShutDown();
      }

      // VOLUME
      if (volumePot.changed()) {
        setVolume(volumePot.value());
      }
    #endif

    #ifdef USE_LIGHTS_BUTTONS
      // LIGHTS BUTTON
      // Tapped: change lights mode 
      if (lightsModeButton.tapped()) {
        nextLightsMode();
      }
      
      // LIGHTS BUTTON
      // Held: chosse a radom color
      if ( lightsModeButton.held() ) {
        setLightsModeRandomColor1();
      }  
    #endif
  #endif

  #ifdef USE_LIGHTS
    // lightsManager need loop to change colors according lights mode
    lightsManager.loop();
  #endif

  #ifdef MP3
    // Print MP3 details
    if (mp3Player.available()) {
      printDetail(mp3Player.readType(), mp3Player.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
  #endif

  // Auto shut down
  if(shutDownActive && (shutDownTimerMillis + (shutDownInMinutes * 1000 * 60) < millis())) {
    shutDownActive = false;    
    shutDown();
  }
}

#ifdef MP3
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
#endif

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
          red = value.substring(0, pos).toInt();
          value = value.substring(pos+1);
        }
        pos = value.indexOf(',');
        if(pos != -1) {
          green = value.substring(0, pos).toInt();
          blue = value.substring(pos+1).toInt();
        }
        setLightsModeColor1({red, green, blue});
      }
      else if(action == "setLightsModeColor2") { // value format = r,g,b
        int red   = 0;
        int green = 0;
        int blue  = 0;

        int pos = value.indexOf(',');
        if(pos != -1) {
          red = value.substring(0, pos).toInt();
          value = value.substring(pos+1);
        }
        pos = value.indexOf(',');
        if(pos != -1) {
          green = value.substring(0, pos).toInt();
          blue = value.substring(pos+1).toInt();
        }
        setLightsModeColor2({red, green, blue});
      }
      else if(action == "setLightsModeParam") { // value should be an int
        setLightsModeParam(value.toInt());
      }
      else if(action == "nextLightsMode") { // no value
        nextLightsMode();
      }
      else if(action == "changeLightsMode") { // value should be an int and should be is managed by lightsManager
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
        playNextDirectory();
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
          color1 = lightsManager.getColor1();
          color2 = lightsManager.getColor2();
          notifyWsClient(clientId, "mp3Playing:" + String(mp3Playing));
          notifyWsClient(clientId, "currentMp3Folder:" + String(currentMp3Folder));
          notifyWsClient(clientId, "loopTrack:" + String(loopTrack));
          notifyWsClient(clientId, "changeLightsMode:" + String(lightsManager.getCurrentMode()));
          notifyWsClient(clientId, "lightsModeColor1:" + String(color1.r) + "," + String(color1.g) + "," + String(color1.b));
          notifyWsClient(clientId, "lightsModeColor2:" + String(color2.r) + "," + String(color2.g) + "," + String(color2.b));
          notifyWsClient(clientId, "lightsModeParam:" + String(lightsManager.getParam()));
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
  #ifdef MP3
    mp3Player.previous(); 
  #endif
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

    #ifdef MP3      
      mp3Player.start();       
    #endif      
    }
    Serial.println("Play");
  } else {
    #ifdef MP3    
      mp3Player.pause();
    #endif
    Serial.println("Pause");
  }

  notifyAllWsClients("mp3Playing:" + String(mp3Playing));
}   

void switchLoopMode() {
  loopTrack = !loopTrack;
  if(loopTrack) {
    #ifdef MP3    
      mp3Player.enableLoop();     
    #endif    
  } else { 
    playOrLoopFirstTrack();      
  }
  Serial.print("Loop mode : ");
  Serial.println(loopTrack ? "Loop track" : "Loop Directory");
  lightsManager.displayAlert({0, 0, 255});

  notifyAllWsClients("loopTrack:" + String(loopTrack));
}

void playNextTrack() {
  Serial.println("Play next");
  #ifdef MP3  
    mp3Player.next();
  #endif  
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
  Serial.println("Next lightsMode");
  lightsManager.nextMode();

  notifyAllWsClientsLightsModeParameters();
}

void changeLightsMode(int mode) {
  Serial.print("Change lightsMode : ");
  Serial.println(mode);
  lightsManager.changeMode(mode);

  notifyAllWsClientsLightsModeParameters();
}

void setLightsModeRandomColor1() {
  Serial.println("Random color 1");   
  RGB color =  {random(255), random(255), random(255)};
  lightsManager.setColor1(color);

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
  
  lightsManager.setColor1(color);

  notifyAllWsClients("lightsModeColor1:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeColor2(RGB color) {
  Serial.print("Set color 2 : {");  
  Serial.println(color.r);
  Serial.print(", ");
  Serial.println(color.g);
  Serial.print(", ");
  Serial.print(color.b);
  Serial.println("}");
  
  lightsManager.setColor2(color);

  notifyAllWsClients("lightsModeColor2:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeParam(int value) {
  Serial.print("Set lights mode parameter : ");
  Serial.println(value);
  
  lightsManager.setParam(value);

  notifyAllWsClients("lightsModeParam:" + String(value));
}

void playRandomAlert() {
  Serial.println("Play Alert");

  int randomAdvert = random(1, NB_MAX_MP3_ADVERT + 1); 
  #ifdef MP3  
    mp3Player.advertise(randomAdvert); 
  #endif  
}  

void setVolume(uint16_t newVolume) {
  if(newVolume < 0) newVolume = 0;
  if(newVolume > 30) newVolume = 30;

  volume = newVolume;
  Serial.print("volumePot = ");
  Serial.println(volume);
  #ifdef MP3
    mp3Player.volume(volume);
  #endif

  notifyAllWsClients("volume:" + String(volume));
}

void switchShutDown() {
  shutDownActive = !shutDownActive;
  Serial.print("Shut down : ");
  Serial.println(shutDownActive);

  if(shutDownActive) {
    shutDownTimerMillis = millis();
    lightsManager.displayAlert({255, 0, 0});
  }
  else {      
    lightsManager.displayAlert({0, 255, 0});
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
  #ifdef MP3  
    mp3Player.pause();
  #endif
  lightsManager.changeMode(OFF);

  notifyAllWsClients("mp3Playing:" + String(mp3Playing));
  notifyAllWsClientsLightsModeParameters();
}

void notifyAllWsClientsLightsModeParameters() {
  RGB color1 = lightsManager.getColor1();
  RGB color2 = lightsManager.getColor2();
  notifyAllWsClients("changeLightsMode:" + String(lightsManager.getCurrentMode()));
  notifyAllWsClients("lightsModeColor1:" + String(color1.r) + "," + String(color1.g) + "," + String(color1.b));
  notifyAllWsClients("lightsModeColor2:" + String(color2.r) + "," + String(color2.g) + "," + String(color2.b));
  notifyAllWsClients("lightsModeParam:" + String(lightsManager.getParam()));
}

/*
  According loop mode, play the first track of the current folder
*/
void playOrLoopFirstTrack() {
  #ifdef MP3  
    if(loopTrack) {      
      mp3Player.playFolder(currentMp3Folder, 1);  
      mp3Player.enableLoop();        
    } else {
      mp3Player.loopFolder(currentMp3Folder);           
    }
  #endif
}
