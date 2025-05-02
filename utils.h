#ifndef UTILS_H
#define UTILS_H

#include <Stream.h>

#ifdef __arm__
  // should use uinstd.h to define sbrk but Due causes a conflict
  extern "C" char* sbrk(int incr);
#else  // __ARM__
  extern char *__brkval;
#endif  // __arm__

typedef enum encodingstate { NORMAL, TWO_BYTE, THREE_BYTE_1, THREE_BYTE_2 } ENCODING_STATE;

class Utils {
public:

  static void blink(long time){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(time);  
    digitalWrite(LED_BUILTIN, LOW);
    delay(time);   
  }

  static void dumpHex(Stream * out, const void * data, size_t size){
    char ascii[17];
    ascii[16] = '\0'; // Null-terminate the ASCII buffer
    size_t i, j;

    for (i = 0; i < size; ++i) {
      unsigned char c = ((unsigned char*)data)[i];

      // Print the hexadecimal representation
      if (c < 0x10) {
        out->print('0');
      }
      out->print(c, HEX);
      out->print(' ');

      // Store the ASCII representation
      ascii[i % 16] = (c >= ' ' && c <= '~') ? c : '.';

      // Print the ASCII representation every 16 bytes
      if ((i + 1) % 8 == 0 || i + 1 == size) {
        out->print(" ");
        if ((i + 1) % 16 == 0) {
          out->print("|  ");
          out->print(ascii);
          out->print(" \r\n");
        } else if (i + 1 == size) {
          ascii[(i + 1) % 16] = '\0';
          if ((i + 1) % 16 <= 8) {
            out->print(" ");
          }
          for (j = (i + 1) % 16; j < 16; ++j) {
            out->print("   ");
          }
          out->print("|  ");
          out->print(ascii);
          out->print(" \r\n");
        }
      } 
    }
  };

  // Quotes might contain non-printable characters. This function filters anything outside of the normal
  // ASCII range 0x20 (SP) - 0x7E (~), replacing some with equivalents were possible.
  // http://lwp.interglacial.com/appf_01.htm
  static int filterNonASCIIText(char * out, const char * input, size_t max){
    unsigned int j = 0;
    char three_byte_encoding_2;
    char three_byte_encoding_1;
    ENCODING_STATE state = NORMAL;
    unsigned int len = strlen(input);

    if( len >= max ){
      len = max - 1;
    }
    
    for(unsigned int i = 0; i < len; i++){
      unsigned char c = input[i];
      switch(state){
        case NORMAL:
          if(c>=0x20 && c<=0x7E){
            out[j] = input[i];
            j++;
            continue;
          } 
          if(c >= 0xC2 && c <= 0xCF){
            state = TWO_BYTE;
            continue;
          }
          if( c == 0xE2 ){
            state = THREE_BYTE_1;
            continue;
          }
          //Ignore
          break;
        
        case TWO_BYTE: //All 2 byte encodings we ignore.
          state = NORMAL;
          break;
          
        case THREE_BYTE_1:  //Get 1st char in 3 byte encodings
          three_byte_encoding_1 = c;
          state = THREE_BYTE_2;
          break;

        case THREE_BYTE_2: //Get 2nd char in 3 byte encodings
          three_byte_encoding_2 = c;
          char replace = ' ';
          // We only care about 0x80 encodigs to find ", ' , etc
          if(three_byte_encoding_1 == 0x80){
            switch(three_byte_encoding_2){
              case 0x82:
              case 0x83:
              case 0x89:
                replace = ' ';
                break;
              case 0x93:
              case 0x94:
                replace = '-';
                break;
              case 0x98:
              case 0x99:
              case 0xB2:
                replace = '\'';
                break;
              case 0x9C:
              case 0x9D:
              case 0xB3:
                replace = '\"';
                break;
              default:
                replace = ' ';
                break;
            }
          }
          out[j] = replace;
          j++;
          state = NORMAL;
          break;
        }
      }
    out[j] = '\0';
    return j;
  };

  static int freeMemory() {
    char top;
    #ifdef __arm__
      return &top - reinterpret_cast<char*>(sbrk(0));
    #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
      return &top - __brkval;
    #else  // __arm__
      return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif  // __arm__
  };
};

#endif
