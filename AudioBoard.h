#ifndef AUDIO_PRINTER_H
#define AUDIO_PRINTER_H

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_VS1053.h>

#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

#include "Logger.h"
#include "wd.h"

extern ConsoleLogger trace;
extern ConsoleLogger info;
extern FileLogger error;

#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

/*
  The audio board includes an OLED display that can be used to print messages.
  OLEDPrinter is just a DisplayPrinter that uses the Adafruit_SSD1306 library.
  OLEDPrinter will override some functions to accomodate for the tiny screen.
  For example, there's no space for Author/id/clock. We use all real estate for 
  the quote.
  We simply pass on the print calls to the OLEDPrinter in most cases IN ADDITION
  to playing an audio files.
*/
class AudioBoard {

  Adafruit_VS1053_FilePlayer audioPlayer;   //Pinout: https://learn.adafruit.com/adafruit-music-maker-featherwing/pinouts
  
public:
 
  AudioBoard() : audioPlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS){
  }

  //Only available on AUDIO PRINTERS
  void play(const char * track){
    if(!audioPlayer.begin()){
      error.log("audioPrinter", "play: Error initializing audio board", track);
      return;
    }
     
    audioPlayer.setVolume(0, 0);

    WD d;
      char audioFile[15];     // {track}.mp3
      snprintf(audioFile, sizeof(audioFile), "%s.mp3", track);
      audioPlayer.playFullFile(audioFile); 
  };

  void test(){
    audioPlayer.begin();
    audioPlayer.setVolume(1,1);
    audioPlayer.sineTest(0x44, 500);
    return;
  }

  void reset(){
    audioPlayer.reset();
  }
};

#endif
