#ifndef CANBUS_H
#define CANBUS_H

#include <Adafruit_MCP2515.h>

#include "Defaults.h"

#define CAN_INT 13
#define CAN_CS 12  
#define CAN_BAUDRATE 125000

// CBUS opcodes
enum CBUS_OPC { NOOP = 0x00, ACON = 0x90, ACOF = 0x91 };

enum CBUSInit { CBUS_INIT_OK = 0, CBUS_INIT_FAIL };

typedef struct {
	byte opcode;
	byte nodeNumberHigh;
	byte nodeNumberLow;
	byte eventNumberHigh;
	byte eventNumberLow;
	byte param1;
	byte param2;
	byte param3;
} __attribute__((packed)) CBUSPacket;

class CBUS {

	Adafruit_MCP2515 mcp;	

public:
  CBUS() : mcp(CAN_CS) {
  };

  int init(){
    if(!mcp.begin(CAN_BAUDRATE)){
			return CBUS_INIT_FAIL;
		};

		return CBUS_INIT_OK;
  }

	char getEvent(int * nodeNumber, int * eventNumber){
		
		CBUSPacket packet;
		int id;
		int packetLength = mcp.parsePacket();
		*nodeNumber = *eventNumber = 0;


		trace.log("CBUS", "Rx packet:", packetLength);
		
		//No message
		if(packetLength == 0){
			trace.log("CBUS", "No message");
			return 0;
		};

		if(mcp.packetRtr()){
			trace.log("CBUS", "Message is RTR - Ignoring");
			return 0;
		}

		trace.logHex("CBUS", "Message received", (char *)&packet, sizeof(packet));

		id = mcp.packetId();
		
		mcp.readBytes((char *)&packet, packetLength <= sizeof(packet) ? packetLength : sizeof(packet));

		if(packet.opcode != ACOF && packet.opcode != ACON){
			trace.logHex("CBUS", "Opcode not supported: ", packet.opcode);
			return 0;
		};

		trace.log("CBUS", "Opcode: ", (packet.opcode == ACON ? "ACON" : "ACOF"));
		*nodeNumber = (packet.nodeNumberHigh << 8) | packet.nodeNumberLow;
		*eventNumber = (packet.eventNumberHigh << 8) | packet.eventNumberLow;
		trace.log("CBUS", "Node Number: ", *nodeNumber);
		trace.log("CBUS", "Event Number: ", *eventNumber);

		return packet.opcode;
	};
};

#endif