# Yuna Lights

Initially develop to construct a light and a MP3 player for my baby.  
Use: 
- Wemos D1 D2 mini.
- RGB Leds
- DFPlayerMini
- Speaker
- Buttons with resistors (Optional)
- Potentiometer (Optional)
- WebSocket (Optional)  
## Features
- MP3
  - Allow to play MP3 tracks in multiple directories
  - Allow to play an arbitrary alert
  - Allow to change loop mode

- Lights
  - Allow to change the lights mode
  - Multiple mode are provided:
    - OFF
    - A single color
      - Allow change the colors
    - Gradient between 2 colors
      - Allow change speed
    - FadeIn / FadeOut
      - Allow change the colors
      - Allow change speed
    - Random colors
      - Allow change speed

- Auto Shut down
  - Stop MP3 and lights after 15 (default) minutes if enabled

- Control from hardware buttons if USE_ELECTRONICS is defined
- Control from a WebApp through WebSocket if USE_WIFI is defined 
 
## Buttons actions
- Prev button
  - tapped : previous track
  - held : previous directory
- Play/Pause button
  - tapped : switch Play/Pause
  - held : switch Loop directory / Loop track
- Next button
  - tapped : next track
  - held : next directory
- Light button
  - tapped : change light mode to next mode (off all lights before change mode)
  - held : choose a random color1 for the current light mode
- Alert button
  - tapped : play a random alert
  - held : switch on/off the timer to stop MP3 and lights (shut down)
- Potentiometer to adjust volume

## WebSocket actions
- setLightsModeColor1 : value format = r,g,b
- setLightsModeColor2 : value format = r,g,b
- setLightsModeParam : value should be an int
- nextLightsMode
- changeLightsMode : value should be an int and should be is managed by LightManager
- playPreviousTrack
- playPreviousDirectory
- switchPlayPause
- switchLoopMode
- playNextTrack
- playNextDirectory
- playRandomAlert
- setVolume : value should be an int [0, 30]
- switchShutDown
- shutDownMinutes : value should be an int
- shutDown
 
## Dependencies
Base :
- Adafruit_NeoPixel
- DFRobotDFPlayerMini

With hardware buttons
- Akta3d_Potentiometer
- ButtonEvents

With WebSockets
- ESP8266WiFi
- ESPAsyncWebServer

## SD Card directories
- root  
  - 001  
    - 001.name1.mp3
    - 002.name2.mp3
    - ...
    - 999.mp3 
  - 002
    - 001.name1.mp3
    - 002.name2.mp3
    - ...
    - 999.mp3 
  - ...
    - ...
  - ALERT
    - 001.name1.mp3
    - 002.name2.mp3  
    - ...  
    - 999.mp3  
     
Need adjust NB_MAX_MP3_FOLDER and NB_MAX_MP3_ADVERT  

## Create your lights mode
It's easy to add different light mode:  
1- Write a new class which extend of LightType.  

2- In lightManager.h  

 - adjust MAX_LIGHT_MODES (don't take in account LightTypeOff mode)
 - add your light type in the enum lightMode
 - define your light type instance in private variable
 
3- In lightManager.cpp  
 - instanciate your lightMode in constructor
 - add your lights mode in the switch of LightManager::changeMode

## Web App
A sample web app is available. It allow to control "all" main feature directly from a browser.  
Just need set the gateway ip in code or in connection gui. During save, the ip is stored in LocalStorage.

### Progressive Web App
Allow to install the webApp on smartphone.  
We need start a webserver, for the first time ton serve index.html then install the web app.  
- cd WebApp
- npm i
- npm start
- go to 127.0.0.1:8080 or youtIp:8080 then install the WebApp