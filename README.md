# Yuna Lights

Initially develop to construct a light and a MP3 player for my baby.  
Use Wemos D1 D2 mini.
  
## Features
- MP3
  - Allow to play MP3 song in multiple directories
  - Allow to play an arbitrary alert from a pressed button
  - Allow to change loop mode

- Lights
  - Allow to change the light mode from a pressed button
  - Multiple mode are provided:
    - OFF
    - A single color
      - Allow change the colors from a pressed button
    - Gradient between 2 colors
    - Random colors

- Auto-off
  - Stop MP3 and light after 15 minutes if active
 
## Button actions
- Prev button
  - tapped : previous song
  - held : previous directory
- Play/Pause button
  - tapped : switch Play/Pause
  - held : switch Loop directory / Loop song
- Next button
  - tapped : next song
  - held : next directory
- Light button
  - tapped : change light mode to next mode (off all lights before change mode)
  - held : choose a random color1 for the current light mode
- Alert button
  - tapped : play a random alert
  - held : switch on/off the timer to stop MP3 and lights
 
## Dependencies
- Adafruit_NeoPixel
- Akta3d_Potentiometer
- ButtonEvents
- DFRobotDFPlayerMini

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
