#ifndef _LOGGER_H
#define _LOGGER_H

#include <RTCZero.h>
#include <SD.h>

#include "Defaults.h"
#include "StringStream.h"
#include "Utils.h"

//Simple logger to console (Serial)
class ConsoleLogger {
  const char * level;

  void logHeader(const char * module, const char * msg){
    Serial.print(level);
    Serial.print("|");
    Serial.print(module);
    Serial.print("|");
    Serial.print(msg);
  }
  
public:
  ConsoleLogger(const char * level){
    this->level = level;
  };

  void log(const char * module, const char * msg, const char * msg2 = nullptr){
    logHeader(module, msg);
    if(msg2){
      Serial.print("|");
      Serial.print(msg2);
    }
    Serial.println();
  };

  void log(const char * module, const char * msg, const unsigned long value){
    logHeader(module, msg);
    Serial.print("|");
    Serial.println(value);
  };

  void logHex(const char * module, const char * msg, const char value){
    logHeader(module, msg);
    Serial.print("|");
    Serial.println(value, HEX);
  };

  void logHex(const char * module, const char * msg, const char * buffer, size_t length){
    logHeader(module, msg);
    Serial.println();
    Utils::dumpHex(&Serial, buffer, length);
  }
};

typedef enum { MODE_MSG, MODE_VALUE, MODE_ENCODED } MSG_MODE;

class FileLogger {

  ConsoleLogger verboseLogger;
  RTCZero rtc;
  const char * level;
  int verbose;

  void printHeader(Stream & out, const char * module){
    out.print(rtc.getEpoch());
    out.print("|");
    out.print(level);
    out.print("|");
    out.print(module);
    out.print("|");
  }

  //Mode defines if msg2 is a string or a number:
  // 1: string
  // 2: number
  void log(const char * module, MSG_MODE mode, const char * msg, const char * contentType, const char * msg2, unsigned long value){

    File f;

    if(!SD.begin(SD_CS)){
      return;
    }

    char fileName[30];  // 123456789012345678901234567890 
                        // LOG/MMDDHHMM.LOG
    snprintf(fileName, sizeof(fileName), "LOG/%02d%02d%02d%02d.LOG", rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes());
    
    f = SD.open(fileName, FILE_WRITE);
    if(!f) return;
    f.seek(f.size()); //Go to the end to append
    
    printHeader(f, module);
    f.print(msg);
    f.print("|");
    f.print(contentType);
    f.print("|");
      
    switch(mode){
      case MODE_MSG:
        if(msg2){ 
          f.println(msg2); 
        }
        break;

      case MODE_VALUE:
        f.println(value);
        break;

      case MODE_ENCODED:
        urlEncodedStream encoded(&f);
        for(int i=0,j=strlen(msg2); i<j; i++){
          int c = msg2[i];
          encoded.write(c);
          Serial.write(c);
        }
        f.println();
        break;
    }
    
    f.close();
  }

public:

  FileLogger(const char * level, int verbose=0) : level(level), verbose(verbose), verboseLogger("ERRDBG"){
    this->level = level;
  }
  
  void log(const char * module, const char * msg, const char * msg2 = nullptr){
    log(module, MODE_MSG, msg, "text/plain", msg2, 0);
    if(verbose){
      verboseLogger.log(module, msg, msg2);
    }
  };
  
  void log(const char * module, const char * msg, const unsigned long value){ 
    log(module, MODE_VALUE, msg, "text/plain", nullptr, value);
    if(verbose){
      verboseLogger.log(module, msg, value);
    }
  };

  void logEncodedBody(const char * module, const char * msg, const char * contentType, const char * body){
    log(module, MODE_ENCODED, msg, contentType, body, 0);
    if (verbose){
      verboseLogger.logHex(module, msg, body, strlen(body));
    }
    
  };
};

typedef enum { LOG_OK, LOG_NO_SD } LOG_MGR_STATUS;
typedef enum { STREAM_MODE_CONTENT, STREAM_MODE_CHUNK } STREAM_MODES;

/*
  Class to manage logs in the SD card.
*/
class LogManager {

  void (*keepAlive)();
  const char * rootFolder;
  File root;

  void printDirectory(Stream & out, File dir, int numTabs){
    while(1){
      (*keepAlive)();
      File entry =  dir.openNextFile();
      if(!entry){
        break;
      }
      for(int i = 0; i < numTabs; i++){
        out.print('\t');
      }
      out.print(entry.name());
      if(entry.isDirectory()){
        out.println("/");
        printDirectory(out, entry, numTabs + 1);
      }else{
        // files have sizes, directories do not
        out.print("\t\t");
        out.println(entry.size(), DEC);
      }
      entry.close();
    } 
  };

  void dumpLog(File & log, Stream & out){
    while(log.available()){
      (*keepAlive)();
      char b[512];
      int r = log.readBytes(b, sizeof(b));
      out.write(b, r);
    }
  };

public:

  // kA -> Watchdog keepAlive callback
  LogManager(void (*kA)(), const char * rootFolder = "LOG"){
    this->rootFolder = rootFolder;
    keepAlive = kA;
  };

  int ensureLogFolderExists(){
    if(!SD.begin(SD_CS)){
      return -1;
    }

    if(SD.exists(rootFolder)){
      return 1;
    }

    return SD.mkdir("LOG");
  }

  void listLogs(Stream & out){
    this->root = SD.open(this->rootFolder);
    printDirectory(out, root, 0);
    root.close();
  };

  int startLogFilesIterator(File & logFile){
    this->root = SD.open(this->rootFolder);
    if(!this->root) return 0;
    logFile = this->root.openNextFile();
    return (logFile==1);
  };

  int openNextLogFile(File & logFile){
    logFile = this->root.openNextFile();
    return (logFile==1);
  }

  void closeLogFilesIterator(){
    this->root.close();
  }

  void dumpAllLogs(Stream & out){
    File logs = SD.open(this->rootFolder);
    if(!logs){
      logs.close();
      return;
    }

    File logFile = logs.openNextFile();
    while(logFile){
      dumpLog(logFile, out);
      logFile.close();
      logFile = logs.openNextFile();
    }

    logs.close();
    return;
  };

  void dumpLog(Stream & out, const char * logName){
    char fileName[30];  // 123456789012345678901234567890 
                        // LOG/MMDDHHMM.LOG
    snprintf(fileName, sizeof(fileName), "%s/%s", rootFolder, logName);
    File logFile = SD.open(fileName);
    dumpLog(logFile, out);
    logFile.close();
    return;  
  };

  void streamLog(File & log, Stream & out){
    dumpLog(log, out);
    return;  
  };

  int removeAll(){
    int removed = 0;
    root = SD.open(this->rootFolder);
    if(!root){ return removed; }
    File logfile = root.openNextFile();
    while(logfile){
      (*keepAlive)();
      if(this->remove(logfile.name())){ removed++; }
      logfile.close();
      logfile = root.openNextFile();
    }
    root.close();    
    return removed;
  };

  int remove(const char * name){
    char fileName[30];  // 123456789012345678901234567890 
                        // LOG/MMDDHHMM.LOG
    snprintf(fileName, sizeof(fileName), "%s/%s", rootFolder, name);
    return SD.remove(fileName);
  };
};

#endif
