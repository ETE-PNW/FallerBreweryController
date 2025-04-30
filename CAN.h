#ifndef CAN_H
#define CAN_H

#include <Adafruit_MCP2515.h>

#define CAN_INT 13
#define CAN_CS 12
#define CAN_BAUDRATE 500000    

class CAN {

	char sendId;
	char nodeId;
	char lastRxId;

  Adafruit_MCP2515 mcp;	

public:
  CAN() : mcp(CAN_CS) {
		sendId = 0x01;
  };

  void init(char nodeId){
    if(!mcp.begin(CAN_BAUDRATE)){
			return;
		};
		this->nodeId = nodeId;
  }

	void sendCommand(char command, char * args){
		mcp.beginPacket(sendId++);
		mcp.printf("%c%c%s", nodeId, command, args);
		mcp.endPacket();   
	};

	char getCommand(char * params, int * dataLength){
		char command;
		int packetLength = mcp.parsePacket();
		*dataLength = packetLength ? packetLength - 1 : 0;

		trace.log("CAN", "Rx packet:", packetLength);
		
		//No message
		if(packetLength == 0){
			trace.log("CAN", "No message");
			return 0;
		};

		if(mcp.packetRtr()){
			trace.log("CAN", "Message is RTR - Ignoring");
			return 0;
		}

		trace.log("CAN", "Message received. Length: ", packetLength);

		lastRxId = mcp.packetId();
		
		/*
			In this implementation the packets over CAN follow this format:
			{cmd} {nodeId} {params...}
			Examples:

				M 0xEE 1				Start Motor (relay)
				M 0XEE 0				Stop Motor
				P 0xEE 001			Play track "001" (that maps to 001.mp3 on the filesystem)
				S	0XEE					Stop playback

		*/
		command = mcp.read();
		mcp.readBytes(params, packetLength - 1);

		return command;
	};
};

#endif