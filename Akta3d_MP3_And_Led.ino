/*
TODO :
- Start and stop WIFI from button
- README : Add hardware connection (with buttons and without buttons)
- README : Add TFT Touchscreen connection with Arduino MEGA
*/

/*
 * Send html, js, .. to the arduino
 * Send all file from the data directory
 * - Close Monitor
 * - Tool => ESP8266 Sketch Data Upload
 */
#define USE_WIFI          // if define, allow to control mp3 and lights from wifi. See nodeJs repo to have the webServer
#define USE_ELECTRONICS   // if define, allow to control mp3 and lights from hardware buttons and potentiometer
 #define USE_MP3
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
  #include <ESPAsyncTCP.h>
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
  #define LED_PIN D4
  #define NB_LED 24
  // LightsManager _lightsManager(LED_PIN, NB_LED, NEO_BGR + NEO_KHZ800); // Circle LED 
  LightsManager _lightsManager(LED_PIN, NB_LED, NEO_GRB + NEO_KHZ400); // Line LED
#endif

// ----- MP3 ----------
bool _mp3Started = false; // needed to know if mp3 player have already start play track since arduino start
bool _mp3Playing = false; // needed to switch between "play" / "pause"
bool _loopTrack = false;  // needed to switch between "loop current track" / "loop all track of the current directory"
uint8_t _currentMp3Folder = 1;
uint8_t _volume = 10;
#define NB_MAX_MP3_ADVERT 3
#define NB_MAX_MP3_FOLDER 2
#ifdef USE_MP3
  #define MP3_RX_PIN D1
  #define MP3_TX_PIN D2
  const bool MP3_START_AFTER_FAIL = true; // allow to force start after a fail from MP3 player
  SoftwareSerial _mp3Serial(MP3_RX_PIN, MP3_TX_PIN);
  DFRobotDFPlayerMini _mp3Player;
#endif
// ----- shut down -------
bool _shutDownActive = false;
uint8_t _shutDownInMinutes = 15;
uint8_t _shutDownTimerMillis = 0;

#ifdef USE_ELECTRONICS
  // ----- BUTTONS ----------
  #ifdef USE_MP3_BUTTONS
    #define PREV_BUTTON_PIN D6
    ButtonEvents _prevButton;

    #define PLAY_BUTTON_PIN D5
    ButtonEvents _playButton;

    #define NEXT_BUTTON_PIN D0
    ButtonEvents _nextButton;

    #define ALERT_BUTTON_PIN D7
    ButtonEvents _alertButton;

    // ----- POTENTIOMETER ----------
    #define VOLUME_POT_PIN A0
    #define MAX_VOLUME 30    
    //Akta3d_Potentiometer _volumePot(VOLUME_POT_PIN, 0, MAX_VOLUME, 0, 1024, 1 /* bCoeff */);
    Akta3d_Potentiometer _volumePot(VOLUME_POT_PIN, 0, MAX_VOLUME, 0, 1024, 0.3 /* bCoeff */, 100 /* updateValueEachMs */);
  #endif 

  #ifdef USE_LIGHTS_BUTTONS
    #define LIGHTS_MODE_BUTTON_PIN D8
    ButtonEvents _lightsModeButton;
  #endif
#endif

  // ----- WIFI & WEBSOCKET ----------
#ifdef USE_WIFI
  AsyncWebServer _server(SERVER_PORT);
  AsyncWebSocket _ws(WEBSOCKET_PATH);
  // message to send to websockets
  // separator is "|"
  String _wsMessages = ""; 
  unsigned long _lastSendMessageMillis = 0;
  unsigned long _sendWsMessageEachMs = 200;
#endif

void setup() {
  // Used for debugging messages
  Serial.begin(9600); 

  Serial.println("");
  Serial.println("");
  Serial.println("Setup Start");

  #ifdef USE_LIGHTS
    _lightsManager.setup();
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
     _lastSendMessageMillis =  millis();
    initWebSocket();

    // Start server
    Serial.println("Start server on port : " + String(SERVER_PORT));
    
    // Route for root / web page
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html");
    });  

    // CAUTION : with this method all files on Arduino filesystem can be send
    // do not store confidential data in filesystem or use specific route to serve only desired files
    _server.onNotFound([](AsyncWebServerRequest *request) {
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

    _server.begin();
  #endif

  #ifdef USE_ELECTRONICS  
    // Attach Pin to buttons
    Serial.println("Attach Pin to buttons");
    #ifdef USE_MP3_BUTTONS
      _prevButton.attach(PREV_BUTTON_PIN, INPUT );
      _playButton.attach(PLAY_BUTTON_PIN, INPUT );
      _nextButton.attach(NEXT_BUTTON_PIN, INPUT );
      _alertButton.attach(ALERT_BUTTON_PIN, INPUT );
      _prevButton.activeHigh();
      _playButton.activeHigh();
      _nextButton.activeHigh();
      _alertButton.activeHigh();
    #endif

    #ifdef USE_LIGHTS_BUTTONS
      _lightsModeButton.attach(LIGHTS_MODE_BUTTON_PIN, INPUT );      
      _lightsModeButton.activeHigh();
    #endif
  #endif

  #ifdef USE_MP3
    //Use softwareSerial to communicate with mp3.
    Serial.println("Start MP3");
    delay(500);
    _mp3Serial.begin(9600);
    if (!_mp3Player.begin(_mp3Serial)) {
      Serial.println(F("Unable to begin mp3 player :"));
      Serial.println(F("1.Please recheck the connection!"));
      Serial.println(F("2.Please insert the SD card!"));
      while(true);
    }
    
    _mp3Player.setTimeOut(500); //Set serial communictaion time out 500ms
    _mp3Player.outputDevice(DFPLAYER_DEVICE_SD); 
    _mp3Player.volume(_volume);
  #endif

  Serial.println("Setup OK");
}

void loop() {  
  #ifdef USE_WIFI
    _ws.cleanupClients(); 
    sendWsMessages();
  #endif

  #ifdef USE_ELECTRONICS  
    // update all buttons
    #ifdef USE_MP3_BUTTONS
      _prevButton.update();  
      _playButton.update();
      _nextButton.update();
      _alertButton.update();
      _volumePot.update();
    #endif
    #ifdef USE_LIGHTS_BUTTONS
      _lightsModeButton.update();
    #endif
    
    #ifdef USE_MP3_BUTTONS
      // PREV BUTTON
      // Tapped: prev track
      if (_prevButton.tapped()) {
        playPreviousTrack();   
      }
      
      // PREV BUTTON
      // Held: prev folder
      if (_prevButton.held()) {    
        playPreviousDirectory();
      }
      
      // PLAY/PAUSE BUTTON
      // Tapped: switch Play/pause
      if (_playButton.tapped()) {
        switchPlayPause();     
      }
    
      // PLAY/PAUSE BUTTON
      // Held: change loop mode (loop single, loop directory)
      if (_playButton.held()) {
        switchLoopMode();    
      }

      // NEXT BUTTON
      // Tapped: next track  
      if (_nextButton.tapped()) {
        playNextTrack();   
      }
      
      // NEXT BUTTON
      // Held: next folder
      if (_nextButton.held()) {
        playNextDirectory();     
      }
    
      // ALERT BUTTON
      // Tapped: play a random alert
      if (_alertButton.tapped()) {
        playRandomAlert();        
      }
      
      // ALERT BUTTON
      // Held: Set timer to pause mp3 and set Lights Off
      if (_alertButton.held()) {
        switchShutDown();
      }

      // VOLUME
      if (_volumePot.changed()) {
        setVolume(_volumePot.value());
      }
    #endif

    #ifdef USE_LIGHTS_BUTTONS
      // LIGHTS BUTTON
      // Tapped: change lights mode 
      if (_lightsModeButton.tapped()) {
        nextLightsMode();
      }
      
      // LIGHTS BUTTON
      // Held: chosse a radom color
      if ( _lightsModeButton.held() ) {
        setLightsModeRandomColor1();
      }  
    #endif
  #endif

  #ifdef USE_LIGHTS
    // _lightsManager need loop to change colors according lights mode
    _lightsManager.loop();
  #endif

  #ifdef USE_MP3
    // Print MP3 details
    if (_mp3Player.available()) {
      printDetail(_mp3Player.readType(), _mp3Player.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
  #endif

  // Auto shut down
  if(_shutDownActive && (_shutDownTimerMillis + (_shutDownInMinutes * 1000 * 60) < millis())) {
    _shutDownActive = false;    
    shutDown();
  }
}

#ifdef USE_MP3
  /**
  Print MP3 details event
  
  On some error, we force play mp3
  On low powered MP3 player with some lights mode power can decrease and mp3 player fired error
  */
  void printDetail(uint8_t type, int value){
    switch (type) {
      case TimeOut:
        Serial.println(F("Time Out!"));
        if(MP3_START_AFTER_FAIL && _mp3Playing) {
          _mp3Player.start();
        }
        break;
      case WrongStack:
        Serial.println(F("Stack Wrong!"));
        if(MP3_START_AFTER_FAIL && _mp3Playing) {
          _mp3Player.start();
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
        if(MP3_START_AFTER_FAIL && _mp3Playing) {
          _mp3Player.start();
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
            if(MP3_START_AFTER_FAIL && _mp3Playing) {
              _mp3Player.start();
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
    //Serial.println("notifyAllWsClients : " + data);
    // _ws.textAll(data);

    _wsMessages += data + "|";
  #endif
}

#ifdef USE_WIFI
  void sendWsMessages() {
    unsigned long now = millis();

    if(now - _lastSendMessageMillis >= _sendWsMessageEachMs) {
      if(_wsMessages != "") {
        _ws.textAll(_wsMessages);
        _wsMessages = "";
      }
      _lastSendMessageMillis = now;
    }
  }

  void notifyWsClient(uint32_t clientId, String data) {
    //Serial.println("notifyWsClient : " + data);
    // _ws.text(clientId, data);

    // we can't send too many message in same time
    // we need buffer them
    // to simplify code, we decide to notify all clients and buffer message for all client
    notifyAllWsClients(data);
  }

  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String dataStr = "";
      for(int i = 0 ; i < len ; i+=1) {
        dataStr += (char)data[i];
      }
      
      String action;
      String value;

      uint8_t index = dataStr.indexOf(':');
      if(-1 != index) {
        action = dataStr.substring(0, index);
        value = dataStr.substring(index + 1);
      }

      Serial.println("Received event. Action: " + action + ", Value: " + value);

      if(action == "setLightsModeColor1") { // value format = r,g,b
        RGB color = RGBStrToRGB(value);
        setLightsModeColor1(color);         
      }
      else if(action == "setLightsModeColor2") { // value format = r,g,b
        RGB color = RGBStrToRGB(value);
        setLightsModeColor2(color);         
      }
      else if(action == "setLightsModeParam") { // value should be an int
        setLightsModeParam(value.toInt());
      }
      else if(action == "nextLightsMode") { // no value
        nextLightsMode();
      }
      else if(action == "changeLightsMode") { // value should be an int and should be is managed by _lightsManager
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
          color1 = _lightsManager.getColor1();
          color2 = _lightsManager.getColor2();
          notifyWsClient(clientId, "mp3Playing:" + String(_mp3Playing));
          notifyWsClient(clientId, "currentMp3Folder:" + String(_currentMp3Folder));
          notifyWsClient(clientId, "loopTrack:" + String(_loopTrack));
          notifyWsClient(clientId, "changeLightsMode:" + String(_lightsManager.getCurrentMode()));
          notifyWsClient(clientId, "lightsModeColor1:" + String(color1.r) + "," + String(color1.g) + "," + String(color1.b));
          notifyWsClient(clientId, "lightsModeColor2:" + String(color2.r) + "," + String(color2.g) + "," + String(color2.b));
          notifyWsClient(clientId, "lightsModeParam:" + String(_lightsManager.getParam()));
          notifyWsClient(clientId, "volume:" + String(_volume));
          notifyWsClient(clientId, "shutDownActive:" + String(_shutDownActive));
          notifyWsClient(clientId, "shutDownInMinutes:" + String(_shutDownInMinutes));
          notifyWsClient(clientId, "shutDownTimerMillis:" + String(_shutDownTimerMillis));
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
    _ws.onEvent(onEvent);
    _server.addHandler(&_ws);
  }
#endif

RGB RGBStrToRGB(String value) {
  uint8_t red   = 0;
  uint8_t green = 0;
  uint8_t blue  = 0;

  uint8_t pos = value.indexOf(',');
  if(pos != -1) {
    red = value.substring(0, pos).toInt();
    value = value.substring(pos+1);
  }
  pos = value.indexOf(',');
  if(pos != -1) {
    green = value.substring(0, pos).toInt();
    blue = value.substring(pos+1).toInt();
  }

  return {red, green, blue};
}

void playPreviousTrack() {
  Serial.println("Play previous"); 
  #ifdef USE_MP3
    _mp3Player.previous(); 
  #endif
} 

void playPreviousDirectory() {
  _currentMp3Folder -= 1;
  if(_currentMp3Folder < 1) {
    _currentMp3Folder = NB_MAX_MP3_FOLDER;
  }

  playOrLoopFirstTrack();

  Serial.print("Play folder ");
  Serial.println(_currentMp3Folder);

  notifyAllWsClients("currentMp3Folder:" + String(_currentMp3Folder));
}

void switchPlayPause() {
  _mp3Playing = !_mp3Playing;
  if(_mp3Playing) {
    if(!_mp3Started) {
      _mp3Started = true;
      playOrLoopFirstTrack();
    } else {

    #ifdef USE_MP3      
      _mp3Player.start();       
    #endif      
    }
    Serial.println("Play");
  } else {
    #ifdef USE_MP3    
      _mp3Player.pause();
    #endif
    Serial.println("Pause");
  }

  notifyAllWsClients("mp3Playing:" + String(_mp3Playing));
}   

void switchLoopMode() {
  _loopTrack = !_loopTrack;
  if(_loopTrack) {
    #ifdef USE_MP3    
      _mp3Player.enableLoop();     
    #endif    
  } else { 
    playOrLoopFirstTrack();      
  }
  Serial.print("Loop mode : ");
  Serial.println(_loopTrack ? "Loop track" : "Loop Directory");
  _lightsManager.displayAlert({0, 0, 255});

  notifyAllWsClients("loopTrack:" + String(_loopTrack));
}

void playNextTrack() {
  Serial.println("Play next");
  #ifdef USE_MP3  
    _mp3Player.next();
  #endif  
}

void playNextDirectory() {
  _currentMp3Folder += 1;
  if(_currentMp3Folder > NB_MAX_MP3_FOLDER) {
    _currentMp3Folder = 1;
  }
  
  playOrLoopFirstTrack();
  
  Serial.print("Play folder ");
  Serial.println(_currentMp3Folder);

  notifyAllWsClients("currentMp3Folder:" + String(_currentMp3Folder));
}   

void nextLightsMode() {
  Serial.println("Next lightsMode");
  _lightsManager.nextMode();

  notifyAllWsClientsLightsModeParameters();
}

void changeLightsMode(int mode) {
  Serial.print("Change lightsMode : ");
  Serial.println(mode);
  _lightsManager.changeMode(mode);

  notifyAllWsClientsLightsModeParameters();
}

void setLightsModeRandomColor1() {
  Serial.println("Random color 1");   
  RGB color =  {random(255), random(255), random(255)};
  _lightsManager.setColor1(color);

  notifyAllWsClients("lightsModeColor1:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeColor1(RGB color) {
  Serial.print("Set color 1 : {");  
  Serial.print(color.r);
  Serial.print(", ");
  Serial.print(color.g);
  Serial.print(", ");
  Serial.print(color.b);
  Serial.println("}");
  
  _lightsManager.setColor1(color);
  
  notifyAllWsClients("lightsModeColor1:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeColor2(RGB color) {
  Serial.print("Set color 2 : {");  
  Serial.print(color.r);
  Serial.print(", ");
  Serial.print(color.g);
  Serial.print(", ");
  Serial.print(color.b);
  Serial.println("}");
  
  _lightsManager.setColor2(color);

  notifyAllWsClients("lightsModeColor2:" + String(color.r) + "," + String(color.g) + "," + String(color.b));
}

void setLightsModeParam(int value) {
  Serial.print("Set lights mode parameter : ");
  Serial.println(value);
  
  _lightsManager.setParam(value);

  notifyAllWsClients("lightsModeParam:" + String(value));
}

void playRandomAlert() {
  Serial.println("Play Alert");

  uint8_t randomAdvert = random(1, NB_MAX_MP3_ADVERT + 1); 
  #ifdef USE_MP3  
    _mp3Player.advertise(randomAdvert); 
  #endif  
}  

void setVolume(uint8_t newVolume) {    
  if(newVolume < 0) newVolume = 0;
  if(newVolume > 30) newVolume = 30;

  _volume = newVolume;
  Serial.println("volumePot = " + String(newVolume));  
  
  #ifdef USE_MP3 
    _mp3Player.volume(_volume);
  #endif

  notifyAllWsClients("volume:" + String(_volume));    
}

void switchShutDown() {
  _shutDownActive = !_shutDownActive;
  Serial.print("Shut down : ");
  Serial.println(_shutDownActive);

  if(_shutDownActive) {
    _shutDownTimerMillis = millis();
    _lightsManager.displayAlert({255, 0, 0});
  }
  else {      
    _lightsManager.displayAlert({0, 255, 0});
  }

  notifyAllWsClients("shutDownActive:" + String(_shutDownActive));
  notifyAllWsClients("shutDownTimerMillis:" + String(_shutDownTimerMillis));
}

void setShutDownMinutes(int value) {
  _shutDownInMinutes = value;
  notifyAllWsClients("shutDownInMinutes:" + String(_shutDownInMinutes));
}

void shutDown() {
  _mp3Playing = false;
  #ifdef USE_MP3  
    _mp3Player.pause();
  #endif
  _lightsManager.changeMode(OFF);

  notifyAllWsClients("mp3Playing:" + String(_mp3Playing));
  notifyAllWsClientsLightsModeParameters();
}

void notifyAllWsClientsLightsModeParameters() {
  RGB color1 = _lightsManager.getColor1();
  RGB color2 = _lightsManager.getColor2();
  notifyAllWsClients("changeLightsMode:" + String(_lightsManager.getCurrentMode()));
  notifyAllWsClients("lightsModeColor1:" + String(color1.r) + "," + String(color1.g) + "," + String(color1.b));
  notifyAllWsClients("lightsModeColor2:" + String(color2.r) + "," + String(color2.g) + "," + String(color2.b));
  notifyAllWsClients("lightsModeParam:" + String(_lightsManager.getParam()));
}

/*
  According loop mode, play the first track of the current folder
*/
void playOrLoopFirstTrack() {
  #ifdef USE_MP3  
    if(_loopTrack) {      
      _mp3Player.playFolder(_currentMp3Folder, 1);  
      _mp3Player.enableLoop();        
    } else {
      _mp3Player.loopFolder(_currentMp3Folder);           
    }
  #endif
}
