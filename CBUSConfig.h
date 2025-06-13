#ifndef CBUSCONFIG_H
#define CBUSCONFIG_H

#include <SD.h>

#include <Arduino.h>
#include <SD.h>

enum { CBUS_CFG_INIT_OK, CBUS_CFG_INIT_FAIL };

#define DEFAULT_TRACK -1

class CBUSConfig {
private:
    int nodeNumber = 0;
    int relayEventNumber = 0;

    static const int MAX_EVENTS = 20;
    static const int MAX_KEY_LEN = 16; // enough for keys like "steam"
    
    char keys[MAX_EVENTS][MAX_KEY_LEN];
    int values[MAX_EVENTS];
    int eventCount = 0;

public:
    CBUSConfig(){}

		int getMappedSoundEvents(){
			return eventCount;
		}

		int getMappedSoundEvent(int index){
			if(index < 0 || index >= eventCount) return -1;
			return values[index];
		}

		const char * getMappedSoundTrack(int index){
			if(index < 0 || index >= eventCount) return nullptr;
			return keys[index];
		}
		
		int init(const char* filename){
			File file = SD.open(filename);
			if(!file) {
				trace.log("CBUSConfig", "Failed to open config file");
				return CBUS_CFG_INIT_FAIL;
			}

			char line[64];  // buffer for reading lines
			while(file.available()) {
				readLine(file, line, sizeof(line));

				if(line[0] == '#' || line[0] == '\0') {
					continue; // comment or empty line
				}

				trim(line);
				char * equalSign = strchr(line, '=');
				if(!equalSign) continue;

				*equalSign = '\0';
				char* key = line;
				char* valStr = equalSign + 1;
				trim(key);
				trim(valStr);

				int value = atoi(valStr);

				if(strcmp(key, "NN") == 0){
					trace.log("CBUSConfig", "Listening to Node Number: ", value);
					nodeNumber = value;
				}else if(strcmp(key, "RELAY_EN") == 0){
					trace.log("CBUSConfig", "Relay Event Number: ", value);
					relayEventNumber = value;
				}else if(eventCount < MAX_EVENTS){
					strncpy(keys[eventCount], key, MAX_KEY_LEN - 1);
					keys[eventCount][MAX_KEY_LEN - 1] = '\0'; // Ensure null termination
					values[eventCount] = value;
					trace.log("CBUSConfig", keys[eventCount], values[eventCount]);
					eventCount++;
				}
			}

			file.close();
			return CBUS_CFG_INIT_OK;
    }

    int getNodeNumber() {
        return nodeNumber;
    }

    int getRelayEventNumber() {
      return relayEventNumber;
    }

    char * getAudioByEventNumber(int eventNumber) {
			for(int i = 0; i < eventCount; i++){
				if(values[i] == eventNumber){
					return keys[i];
				}
			}
			return nullptr;
    }

	char * getDefaultAudio(){
		return getAudioByEventNumber(DEFAULT_TRACK);
	}

private:
    // Read line from file into buffer, null-terminated
    void readLine(File& file, char* buffer, int maxLen){
			int index = 0;
			while(file.available() && index < maxLen - 1){
				char c = file.read();
				if(c == '\n') break;
				buffer[index++] = c;
			}
			buffer[index] = '\0';
    }

    // Trim leading and trailing whitespace (in place)
    void trim(char* str){
			// Left trim
			while (isspace(*str)) ++str;

			// Right trim
			char* end = str + strlen(str) - 1;
			while (end > str && isspace(*end)) --end;
			*(end + 1) = '\0';

			// Shift to front if needed
			if (str != bufferStart(str)) {
					memmove(bufferStart(str), str, strlen(str) + 1);
			}
    }

    // Get the start of the buffer to copy back trimmed data
    char * bufferStart(char* str){
      return str - (str - &str[0]); // pointer math trick
    }
};

#endif