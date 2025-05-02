#ifndef AUDIO_PRINTER_H
#define AUDIO_PRINTER_H

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_VS1053.h>

#include "Logger.h"
#include "wd.h"

extern ConsoleLogger trace;
extern ConsoleLogger info;
extern FileLogger error;

#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

enum AudioBoardInit { AUDIOBOARD_INIT_OK = 0, AUDIOBOARD_INIT_FAIL };

class AudioBoard {

  Adafruit_VS1053_FilePlayer audioPlayer;   //Pinout: https://learn.adafruit.com/adafruit-music-maker-featherwing/pinouts
  
public:
 
  AudioBoard() : audioPlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS){
  }

  int init(){
    auto ret = audioPlayer.begin();
    if(!ret){
      error.log("AudioBoard", "Error initializing audio board");
      return AUDIOBOARD_INIT_FAIL;
    }
    trace.log("AudioBoard", "Board initialized");
    audioPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
    audioPlayer.setVolume(0, 0);  //MAX Volume

    if(!SD.begin(CARDCS)) {
      error.log("AudioBoard", "SD card initialization failed. Check a card is inserted.");
      return AUDIOBOARD_INIT_FAIL;
    }

    return AUDIOBOARD_INIT_OK;
  }

  void play(const char * track){
  
    if(audioPlayer.playingMusic){
      trace.log("AudioBoard", "A track is already playing");
      return;
    }

    trace.log("AudioBoard", "Playing track: ", track);

    char audioFile[15];     // {track}.mp3
    snprintf(audioFile, sizeof(audioFile), "%s.mp3", track);
    audioPlayer.startPlayingFile(audioFile);
  }

  int isPlaying(){
    return audioPlayer.playingMusic;
  }

  void stopPlaying(){
    trace.log("AudioBoard", "Stop playing");
    audioPlayer.stopPlaying();
  }

  void test(){
    trace.log("AudioBoard", "Testing board");
    audioPlayer.setVolume(1,1);
    audioPlayer.sineTest(0x44, 500);
    return;
  }
};

#endif
