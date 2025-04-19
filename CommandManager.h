#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <ArduinoJson.h>

#include "HTTPRequest.h"
#include "Authenticator.h"
#include "NetworkManager.h"
#include "utils.h"
#include "cliDevice.h"
#include "QuotesManager.h"
#include "Actions.h"

typedef struct {
  int quotesReceived;
  int commandsExecuted;
  int errors;
} COMMAND_RESULT;

class CommandManager{

    HTTPRequest * request;
    Authenticator * auth;
    Printer * printer;
    CliContext * cliContext;    //Context to be used for CLI commands
    QuotesManager * quotes;

    // Builds the response ACK message directly on the response buffer
    void buildAckMessage(const char * ackId, int encoded, const char * message){
        FixedSizeCharStream out(request->dataBuffer(), MAX_HTTP_RESPONSE);
        out.print("{\"m\":");
        out.print(Utils::freeMemory());
        out.print(",\"ack\":\"");
        out.print(ackId);
        out.print("\",\"response\":{\"encoded\":");
        out.print(encoded ? "true" : "false");
        out.print(",\"text\":\"");
        out.print(message);
        out.print("\"}}");
        out.end();
    };

        //Expects request->DataBuffer() to be ready
    int ackCommand(COMMAND_RESULT * result){
        int count = 0;
        /*
            * Send back response:
            * {
            *    response: {
            *      encoded: true | false (whether the text is URLEncoded or not
            *      text: "data..."
            *    },
            *    ack: ""    //The same as the ack sent in the request
            * }
            * 
            * Response from server is number of outstanding commands (CLI or PRINT) i the queue
            */

        //trace.logHex("CommandManager", "Ack Message", request->dataBuffer(), strlen(request->dataBuffer()));
        NetworkManager network(request, auth);
        auto res = network.postJSON(Config::SERVER(), Config::TELEMETRY_COMMAND_PATH(), nullptr);
        if(res){
            trace.log("CommandManager", "Remote Command. Response sent and acknowledged: ", res->statusCode);
            if(res->statusCode == 200){
                count = atoi(res->data);
                result->commandsExecuted++;
            } else {
                //Any return code != 200 - cancel
                error.log("CommandManager", "Remote Command. ACK failed. StatusCode: ", res->statusCode);
                result->errors++;
            }
        } else {
            error.log("CommandManager", "Remote Command. No response received. Sent might have failed.");
            result->errors++;
        }

        return count;
    };

    /*
    * {
    *   "response": {
    *      encoded: true,
    *      text: urlencode("URL ENCODED result"),
    *    }
    *   "ack":"ACK ID"
    * }
    */
    int processCLICommand(COMMAND_RESULT * result, JsonDocument & json){

        const char * cmd = (json)["msg"]["body"]["cmd"] | "";
        const char * ackId = (json)["ack"] | "";
        trace.log("CommandManager", "New Command: ", cmd);
        trace.log("CommandManager", "Ack: ", ackId);

        char buffer[CLI_LINE_BUF_SIZE];
        FixedSizeCharStream input(buffer, sizeof(buffer));
        input.print(cmd);
        urlEncodedStream in(&input);

        FixedSizeCharStream out(request->dataBuffer(), MAX_HTTP_RESPONSE);

        /*
            Notice that in this case, the ACK response is built in 3 parts:
            1. The "header"
            2. The CLI response (which is encoded)
            3. the "footer" (closing the JSON object)
        */
        //Header of the response
        out.print("{\"m\":");
        out.print(Utils::freeMemory());
        out.print(",\"ack\":\"");
        out.print((const char *)(json)["ack"]);
        out.print("\",\"response\":{\"encoded\":true,\"text\":\"");

        urlEncodedStream cmd_result(&out);

        //Run the CLI (result will be appended to "response.text")
        //cmdContextAckId is used to send progress messages in "async" actions.
        strncpy(cliContext->ackId, (const char *)(json)["ack"], sizeof(cliContext->ackId));
        CliDevice cli(&in, &cmd_result, cliContext);
        cli.run();

        //Close JSON and send
        out.print("\"}}");
        out.end();

        trace.logHex("CommandManager", "CLI Response", request->dataBuffer(), strlen(request->dataBuffer()));

        return ackCommand(result);
    };

    unsigned long lastPrint;  // keep track of last time we printed

    /*
        Simplish damper to prevent runaway printings (on paper printers specially). We only allow printing a new quote 
        after 5 min
        Notice we don't really take into account LOOONG waits. 
        Audio devices don't damp
    */

    int shouldDampPrinting(){
        #if defined(AUDIO_PRINTER)
            return 0;
        #else
            unsigned long now = millis();
            if(now > lastPrint){
            return (now-lastPrint) < (5 *60 * 1000);  //5 min damper    
            }
            return ((4294967295L - lastPrint) + now) < (5 *60 * 1000);
        #endif
    }

    /*
        * print commands will be of this shape:
        * {
        *  ack: "ack id", 
        *  msg: {
        *   body: {
                text: "the quote text",
                author: "the author",
                id: "the id", 
            }
        *  }
        * }
    */
    int processPrintCommand(COMMAND_RESULT * result, JsonDocument & json){

        const char * statusMsg;

        //We allow printing every 10 min or so to extend life of the displays
        if(!shouldDampPrinting()){         
          JsonObject quote = (json)["msg"]["body"];
          auto q = quotes->setCurrentQuote(quote["text"], quote["author"], quote["id"]);
          if(q){
            this->printer->computePages(q);
            q->save(LAST_QUOTE_FILE);
            result->quotesReceived++;
            statusMsg = "Quote printed";
            lastPrint = millis();
          } else {
            statusMsg = "Quote skipped";
          }
        } else {
            statusMsg = "Skipping quote. Too soon";
        }

        const char * ackId = (json)["ack"];
        buildAckMessage(ackId, 0, statusMsg);
            
        int count = ackCommand(result);

        return count;
    };

    /*
    * This function processes a "Life Progress" command with JSON input and:
    * 1. Parses required fields from the JSON structure.
    * 2. Calls the printer to display the life progress bar.
    * 3. Constructs a response message and appends it to the data buffer.
    * 4. Acknowledges the command.
    *
    * JSON input example:
    * {
    *   "ack": "ack id",
    *   "msg": {
    *     "body": {
    *       "now": "December 17, 2024",
    *       "lived": { "weeks": 2859 },
    *       "left": { "weeks": 1419 },
    *       "percentage": { "lived": "66.84", "left": "33.62" }
    *     }
    *   }
    * }
    */
    int processLifeProgressCommand(COMMAND_RESULT *result, JsonDocument &json) {

        // 1. Debug print: Received data
        //Serial.println(request->dataBuffer());

        // 2. Safely parse required fields from JSON (ArduinoJson provides an "|" operator:
        // https://arduinojson.org/v6/api/jsonvariantconst/or/
        const char *dateNow = json["msg"]["body"]["now"] | "";    // Default to empty string if missing
        float percentageLived = atof(json["msg"]["body"]["percentage"]["lived"] | "0.0");

        // Retrieve weeks lived and calculate total weeks
        int weeksLived = json["msg"]["body"]["lived"]["weeks"] | 0;
        int weeksLeft = json["msg"]["body"]["left"]["weeks"] | 0;
        int weeksTotal = weeksLived + weeksLeft;

        // 3. Print progress bar using parsed data
        printer->printLifeProgressBar(percentageLived, weeksLived, weeksTotal, dateNow);

        // 4. Prepare and send response
        buildAckMessage(json["ack"] | "", 0, "Printed Life Progress");
        FixedSizeCharStream out(request->dataBuffer(), MAX_HTTP_RESPONSE);

        // 5. Acknowledge command and return result
        return ackCommand(result);
    };

    /*
    * WordsClock commands will be of this shape:
    * {
    *  ack: "ack id", 
    *  msg: {
    *    body: {
            wordsclock: "Ten past five",
            language: "english"
        }
    *  }
    * }
    */
    int processWordsClockCommand(COMMAND_RESULT * result, JsonDocument & json){

        //Save the ACK for later (in case we loose the message downloading the audio
        char ack[20];
        strcpy(ack, json["ack"]);

        //If this is an AUDIO DEVICE, before "printing" we need to download the audio
        //Remember the HTTPRequest is a singleton and we reuse the buffer for send and receiving
        #ifdef AUDIO_PRINTER
            NetworkManager network(request, auth);
            auto res = network.get(Config::SERVER(), AUDIO_WORDSCLOCK_PATH, nullptr, 4);
            if(!res){
                error.log("CommandManager", "processWordsClockCommand. Get Wordsclock audio failed. No response");
                result->errors++;
                return 0;
            }

            if(res->statusCode != 200){
                error.log("CommandManager", "processWordsClockCommand. Failed to download audio", res->statusCode);
                result->errors++;
                return 0;
            }
        #endif
        //In non audio devices we simply print the wordsclock. In an audio device, 
        //we first download the audio file before "printing"
        this->printer->printWordsClock((const char *)(json)["msg"]["body"]["wordsclock"]);  

        buildAckMessage(ack, 0, "Printed wordsclock");
            
        int count = ackCommand(result);

        return count;
    };

    int processCommand(COMMAND_RESULT * result, HTTPResponse * res){
        JsonDocument json;
        DeserializationError err = deserializeJson(json, res->data);

        if(err){
            metrics.TelJsoSerErrInc();
            error.log("CommandManager", "Remote Command - ParseJSON. Error: ", err.c_str());
            result->errors++;
            return 0;
        }

        /*
            * Command objects are of the form:
            * {
            *  msg: {
            *    commandType: "cli" | "print" | 'wordsclock' | others...,
            *    body: {
            *      text: urlencoded(cmd) |
            *      or other payloads depending on the commandType
            *    }
            *  tries: n
            *  ack: 'ack id'
            * }
            */

        const char * cmdType = json["msg"]["commandType"] | "";
        const char * cmdTypes[] = { "cli", "print", "wordsclock", "lifeprogress", nullptr };
        int (CommandManager::*processFuncs[])(COMMAND_RESULT*, JsonDocument&) = {
            &CommandManager::processCLICommand,
            &CommandManager::processPrintCommand,
            &CommandManager::processWordsClockCommand,
            &CommandManager::processLifeProgressCommand,
            nullptr
        };

        for (int i = 0; cmdTypes[i] != nullptr; ++i) {
            if (!strcmp(cmdType, cmdTypes[i])) {
                return (this->*processFuncs[i])(result, json);
            }
        }

        error.log("CommandManager", "Remote Command - ProcessCommand. Unkown commandType:", cmdType);
        return 0;       // No more docs in the queues
    };

public:
    CommandManager(HTTPRequest * request, Authenticator * auth, 
                    Printer * printer, CliContext * ctx,
                     QuotesManager * quotes) : 
        request(request), auth(auth), printer(printer), cliContext(ctx), quotes(quotes){
    };

    /*
        Checks command queue for new commands for the device.
    */
    void processRemoteCommands(COMMAND_RESULT * result) {
        int msgCount = 0;

        do {
            NetworkManager network(request, auth);
            auto res = network.get(Config::SERVER(), Config::TELEMETRY_COMMAND_PATH(), nullptr);
            
            if(!res){
                error.log("CommandManager", "processRemoteCommands. No response");
                result->errors++;
                break;
            }
            
            if(res->statusCode == 404){
                trace.log("CommandManager", "processRemoteCommands. No more commands to execute.");
                break;
            }

            if(res->statusCode != 200){
                error.log("CommandManager", "processRemoteCommands", *res);
                result->errors++;
                break;
            }
            msgCount = processCommand(result, res);
        }while(msgCount > 0);

        if(result->quotesReceived > 0) {
            //We retrieve the audio for the last quote
            auto q = quotes->getCurrentQuote();
            q->getQuoteAudio();
        }
    };

    /*
        This function is used for commands that provide 
    */
    int sendCommandResponse(const char * ackId, const char * message){
        snprintf(request->dataBuffer(), MAX_HTTP_RESPONSE, "{\"m\":%d,\"ack\":\"%s\",\"response\":{\"encoded\":false,\"text\":\"%s\"}}", Utils::freeMemory(), ackId, message);
        NetworkManager network(request, auth);
        auto res = network.putJSON(Config::SERVER(), Config::TELEMETRY_COMMAND_PATH(), nullptr);
        if(res){
            trace.log("CommandManager", "Response sent and acknowledged: ", res->statusCode);
            if(res->statusCode == 200){
                return 0;
            } else {
                //Any return code != 200 - cancel
                error.log("CommandManager", "ACK failed. StatusCode: ", res->statusCode);
                return -1;
            }
        } else {
            error.log("CommandManager", "No response received. Sent might have failed.");
            return -1;
        }
    };
};

#endif
