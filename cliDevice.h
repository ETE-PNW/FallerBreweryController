#ifndef CLI_DEVICE_H
#define CLI_DEVICE_H

#include <ctype.h>

class Actions;

#include "cli.h"
#include "WD.h"
#include "Dispatcher.h"
#include "AudioBoard.h"


class CliDevice : public Cli {

    void help_audio(){
      out->println("Controls audio playback.");
      out->println("Options:");
      out->println("[list|ls|L]: lists all audio files in the SD card.");
      out->println("[play|p|P] {file}: plays the file {file}.");
    };
    
    int cmd_audio(){
        if(noArguments()){
          return CMD_HELP;
        }

        const char * lsc[] = {"list", "ls", "L", nullptr};
        if(isSubcommand(lsc)){
          auto root = SD.open("/");
          while(true){
            (*ctx->keepAlive)();
            File entry = root.openNextFile();
            if(!entry) break;
            const char * name = entry.name();
            //Serial.println(name);
            if(name && strlen(name) > 4 && strcmp(name + strlen(name) - 4, ".MP3") == 0){
              out->println(name);
            }
            entry.close();
          }
          root.close();
          return CMD_OK;
        }

        const char * play[] = {"p", "play", "P", nullptr};
        if(isSubcommand(play)){
            if(strlen(args[2]) == 0){
                out->println("Please enter the track to play.");
                return CMD_ERROR;
            }

            auto audio = ctx->audio;
            audio->play(args[2]);
            return CMD_OK;
        }

        const char * stop[] = {"s", "stop", "halt", nullptr};
        if(isSubcommand(stop)){
            if(!ctx->audio->isPlaying()){
                out->println("Sound is not playing");
                return CMD_OK;    
            }
            ctx->audio->stopPlaying();
            return CMD_OK;
        }

        const char * test[] = {"t", "test", "tst", nullptr};
        if(isSubcommand(test)){
            ctx->audio->test();
            return CMD_OK;
        }

        out->println("Invalid parameter.");
        return CMD_ERROR;
    };

    //Memory
    void help_mem(){
      out->println("Displays free memory of device.");
    };

    int cmd_mem(){
      out->print("Free memory: ");
      out->println(Utils::freeMemory());
      return CMD_OK;
    };

    //About
    void help_about(){
      out->println("Displays information about this device");
    };

    int cmd_about(){
      const char * msg = "This is the Faller Brewery Kit audio/animation module built by ETE PNW";
      out->println(msg);
      return CMD_OK;
    };

    void help_dispatcher(){
        out->println("Displays all scheduled Actions, with expected trigger counts.");
        out->println("With no options, displays information of all Actions.");
        out->println("Options:");
        out->println("[execute|x|exe|exec|run|X {index|name}]: executes action with index {index} or name {name}. Default is 0.");
        out->println("[tick|T {index} {ticks}]: sets ticks for action {index}. {ticks} must be >= -1. -1 means disabled.");
        out->println("[schedule|sch|immediate|s|S {index}]: sets Action {index} for immediate execution."); 
        out->println("[step|stpe|ste|st]: calls dispatcher and steps over as a TICK had happened (regardless if dispatcher is disabled)."); 
        out->println("[disable|dis|D]: disables all Actions.");
        out->println("[enable|ena|E]: enables all Actions.");
    };

    int cmd_dispatcher(){

        if(noArguments()){
            const int len = ctx->dispatcher->getActionsLength(); 
            if(len == 0){
                out->println("No actions registered in Disptacher. (Should not happen).");
                return CMD_OK;
            }

            out->print(len);
            out->print(" actions registered with the Dispatcher. ");
            out->println(ctx->dispatcher->status() ? " ENABLED" : " DISABLED");
            for(int j = 0; j < len; j++){
                const DispatcherAction<Actions> * a = ctx->dispatcher->getAction(j);
                if(a){
                    out->print(j);
                    out->print(". Action [");
                    out->print(a->name);
                    out->print("]. Ticks: ");
                    out->print(a->ticks);
                    out->print(", Count: ");
                    out->print(a->count);
                    out->print(", Runs in ");
                    out->print(ctx->dispatcher->secondsToNextRun(j));
                    out->print(" seconds, Once in ");
                    out->println(a->once);
                }
            }
            return CMD_OK;
        }

        const char * tsc[] = {"x", "exe", "exec", "execute", "run", "X", nullptr};
        if(isSubcommand(tsc)){
            bool isNum = 1;
            const char* s = args[2];
            while (*s) {
                if (!isdigit(*s)) {
                    isNum = 0;
                    break;
                }
                s++;
            }
            if(isNum){
                int index = atoi(args[2]);
                ctx->dispatcher->execute(index);
            } else {
                ctx->dispatcher->execute(args[2]);
            }
            return CMD_OK;
        }

        const char * tic[] = {"t", "tick", "T", nullptr };
        if(isSubcommand(tic)){
            int index = atoi(args[2]);
            int ticks = atoi(args[3]);
            ctx->dispatcher->updateActionTicks(index, ticks);
            out->print("Action [");
            out->print(ctx->dispatcher->getAction(index)->name);
            out->print("] updated with [");
            out->print(ticks);
            out->println(" ticks");
            return CMD_OK;
        }

        const char * dis[] = {"dis", "disable", "D", nullptr};
        if(isSubcommand(dis)){
            ctx->dispatcher->disableAllActions();
            out->println("All actions disabled."); 
            return CMD_OK;
        }

        const char * ena[] = {"ena", "enable", "E", nullptr};
        if(isSubcommand(ena)){
            ctx->dispatcher->enableAllActions();
            out->println("All actions enabled."); 
            return CMD_OK;
        }

        const char * sch[] = {"sch", "schedule", "immediate", "s", "S", nullptr};
        if(isSubcommand(sch)){
            int index = atoi(args[2]);
            if(ctx->dispatcher->scheduleForImmediateExecution(index)<0){
                out->println("Invalid action");
                return CMD_ERROR;
            } else {
                out->print("Action ");
                out->print(index);
                out->println(" will execute immediately"); 
                return CMD_OK;
            }
        }

        const char * step[] = {"step", "stpe", "ste", "st", nullptr};
        if(isSubcommand(step)){
            out->println("Step");
            ctx->dispatcher->step();
            return CMD_OK;  
        }
        
        out->println("Invalid parameter.");
        return CMD_HELP;
    };

    void help_version(){
        out->println("Prints version of device.");
    }

    int cmd_version(){
        out->println();
        out->println(DEVICE_VERSION);
        return CMD_OK;
    };

    void help_relay(){
        out->println("Manages motor relay.");
        out->println("Options:");
        out->println("[on|1|start|s]: Turns on motor.");
        out->println("[off|0|stop|st]: Turns off motor.");
    };

    int cmd_relay(){

        if(noArguments()){
          return CMD_HELP;
        }

        const char * on[] = {"on", "1", "start", "s", nullptr };
        if(isSubcommand(on)){
            ctx->relay->on();
            out->println("Motor started.");
            return CMD_OK;
        }

        const char * off[] = {"off", "0", "stop", "st", nullptr };
        if(isSubcommand(off)){
            ctx->relay->off();
            out->println("Motor stopped.");
            return CMD_OK;
        }

        out->println("Invalid parameter.");
        return CMD_HELP;
    };

    void help_wdt(){

    /*
        * z Bit 7 – Reserved
        This bit is unused and reserved for future use. For compatibility with future devices, always write this bit to zero
        when this register is written. This bit will always return zero when read.
        z Bit 6 – SYST: System Reset Request
        This bit is set if a system reset request has been performed. Refer to the Cortex processor documentation for more
        details.
        z Bit 5 – WDT: Watchdog Reset
        This flag is set if a Watchdog Timer reset occurs.
        z Bit 4 – EXT: External Reset
        This flag is set if an external reset occurs.
        z Bit 3 – Reserved
        This bit is unused and reserved for future use. For compatibility with future devices, always write this bit to zero
        when this register is written. This bit will always return zero when read.
        z Bit 2 – BOD33: Brown Out 33 Detector Reset
        This flag is set if a BOD33 reset occurs.
        z Bit 1 – BOD12: Brown Out 12 Detector Reset
        This flag is set if a BOD12 reset occurs.
        z Bit 0 – POR: Power On Reset
        This flag is set if a POR occurs.
        */
       out->println("Controls WDT.");
       out->println("None: returns the RCAUSE register that shows the reason of last reset.");
       out->println("   0x10: external reset");
       out->println("   0x20: reset occured through the WDT.");
       out->println("   0x40: system reset.");
       out->println("   0x01: power on reset.");
       out->println("Options:");
       out->println("[test|t|tst|T]: enters a blocking loop, simulating a hang and triggering the WDT to reset the board.");
    };

    int cmd_wdt(){

        unsigned char causes[] = {0x10, 0x20, 0x40, 0x01};
        const char * causeDescr[] = {"External reset", "Reset occured through the WDT.", "System reset.", "Power on reset."};

        if(noArguments()){
            auto cause = Watchdog.resetCause();
            out->print("Reset cause: ");
            out->print(cause, HEX);
            out->print(" (");
            out->print(cause);
            out->println(")");
            for(int i = 0; i < sizeof(causes)/sizeof(unsigned char); i++){
                if(cause == causes[i]){
                    out->print("   ");
                    out->println(causeDescr[i]);
                    return CMD_OK;
                }
            }
            
            out->println("Unknown reset cause.");
            return CMD_ERROR;
        }

        const char * tsc[] = {"test", "t", "tst", "T", nullptr };
        if(isSubcommand(tsc)){
            out->println("WDT. System will now enter an infinite loop and reset.");
            while(1){} 
        }

        out->println("Invalid parameter.");
        return CMD_HELP;
    };


    void help_reset(){
        out->println("Resets the board.");
        out->println("Options:");
        out->println("[ETE]: resets the board. This is a safeguard to prevent accidental resets.");
    };

    //This command will HARD reset the board. Notice that if executed via a remote command, it will NOT ACK it.
    int cmd_reset(){

        if(noArguments()){
            out->println("Resetting the board needs the special parameter 'EPISTULAS' as a safeguard.");
            return CMD_OK;
        }

        //Just a simple protection to prevent accidentally reseting the board
        if(!strcmp("ETE", args[1])){
            NVIC_SystemReset();
            //Will never get here!
            return CMD_OK;
        }

        out->println("Invalid parameter.");
        return CMD_ERROR;
    };

    void help_fs(){
        out->println("Manages the SD card file system.");
        out->println("Options:");
        out->println("[ls|l|dir|L]: lists all files and directories in the root.");
        out->println("[mkdir|md|D {name}]: creates a directory with name {name}.");
        out->println("[cat {file} {hex}]: prints the content of the file {file}. If {hex} is present, prints in hex.");
        out->println("[rm|del {file}]: removes the file {file}.");
    };

    int cmd_fs(){

        if(ctx->audio->isPlaying()){
            out->println("Filesystem commands are disabled while playing sound");
            return CMD_OK;
        }

        if(!SD.begin(SD_CS)) {
            out->println("SD card initialization failed. Check a card is inserted.");
            return CMD_ERROR;
        }

        if(noArguments()){
            return CMD_HELP;
        }

        const char * ls[] = {"ls", "l", "dir", "L", nullptr};
        if(isSubcommand(ls)){
            File root = SD.open("/");
            printDirectory(out, root, 0, ctx->keepAlive);
            root.close();
            return CMD_OK;
        } 

        const char * md[] = {"mkdir", "md", "D", nullptr};
        if(isSubcommand(md)){
            if(SD.mkdir(args[2])){
                out->print(args[2]);
                out->println(" succeeded.");
                return CMD_OK;
            } else {
                out->println("Create directory failed.");
                return CMD_ERROR;
            }
        } 

        //This command works differently if run from a remote TTY
        const char * cat[] = {"cat", "C", nullptr };
        if(isSubcommand(cat)){
            if(!SD.exists(args[2])){
                out->print("File [");
                out->print(args[2]);
                out->println("] not found.");
                return CMD_OK;
            }

            int hex = 0;
            const char * hexc[] = {"hex", "x"};
            if(isSubcommand(args[3], hexc)){
                hex = 1;
            }

            File f = SD.open(args[2], O_READ);
            f.seek(0);
            while(f.available()){
                (*ctx->keepAlive)();
                char b[256];    //Magic buffer - note that if command is not called on the local TTY, it will only print a subsegment
                int r = f.readBytes(b, sizeof(b));
                if(hex){
                    Utils::dumpHex(out, b, r);   
                } else {
                    out->write(b, r);
                }
            }
            f.close();
            return CMD_OK;
        }

        const char * rm[] = {"rm", "del", nullptr};
        if(isSubcommand(rm)){
            if(SD.exists(args[2])){
                File f = SD.open(args[2]);
                if(f.isDirectory()){
                    SD.rmdir(args[2]);
                    out->println("Directory deleted.");
                } else {
                    SD.remove(args[2]);
                    out->println("File deleted.");  
                }
                f.close();
                return CMD_OK;
            } else {
                out->println("File not found.");
                return CMD_ERROR;
            }
            return CMD_OK;
        }

        out->println("Invalid subcommand. See help.");
        return CMD_HELP;
    };

    void help_logs(){
        out->println("Manages logs.");
        out->println("Options:");
        out->println("[ls|l|dir|L]: lists all log files.");
        out->println("[dump|d|cat {file}]: prints the content of the log file {file}.");
        out->println("[remove|del|rm {file}]: removes the log file {file}. Enter 'all' to remove all logs.");
    };

    int cmd_logs(){

        if(!SD.begin(SD_CS)) {
            out->println("SD card initialization failed. Check a card is inserted.");
            return CMD_ERROR;
        }

        if(noArguments()){
            return CMD_HELP;
        }

        LogManager lm(ctx->keepAlive, "LOG");

        const char * ls[] = {"ls", "l", "dir", nullptr };
        if(isSubcommand(ls)){
            lm.listLogs(*out);
            return CMD_OK;
        }

        const char * dump[] = {"dump", "d", "cat", nullptr };
        if(isSubcommand(dump)){
            if(strcmp("all", args[2]) == 0){
                lm.dumpAllLogs(*out);
                return CMD_OK;
            }
            
            lm.dumpLog(*out, args[2]);
            return CMD_OK;
        }

        const char * remove[] = {"remove", "del", "rm", nullptr };
        if(isSubcommand(remove)){
            if(strlen(args[2]) == 0){
                out->println("Please specify the log to remove. Enter \"all\" to remove them all");
                return CMD_ERROR;
            }

            if(strcmp("all", args[2])==0){
                int r = lm.removeAll();
                if(r==0){
                    out->println("No files to remove");
                } else {
                    out->print("Removed ");
                    out->print(r);
                    out->println(" files.");
                }
                return CMD_OK;
            }
            
            int r = lm.remove(args[2]);
            if(r){
                out->println("Log file removed");
            } else {
                out->println("Log file not found or failed to remove.");
            }
            return CMD_OK;
        };

        out->println("Invalid subcommand. See help.");
        return CMD_HELP;
    };

    CMDS * buildCmds(){
        //All aliases for commands
        static const char * a_dispatcher[] = {"dispatcher", "disp", nullptr };
        static const char * a_about[] = {"ab", nullptr};
        static const char * a_wdt[] = {"wdt", nullptr};
        static const char * a_mem[] = {"m", "mem", "memory", nullptr};
        static const char * a_reset[] = {"r", "res", "reset", "rst", nullptr};
        static const char * a_test[] = {"test", "tset", "tst", nullptr};
        static const char * a_fs[] = {"fs", "files", nullptr};
        static const char * a_version[] = {"v", "ver", "version", nullptr};
        static const char * a_logs[] = {"log", nullptr};
        static const char * a_audio[] = {"audio", "aud", nullptr};
        static const char * a_relay[] = {"relay", "rly", nullptr};

        #define CLI_COMMAND_ENTRY(name, alias) \
            { #name, static_cast<helpHandler>(&CliDevice::help_##name), static_cast<commandHandler>(&CliDevice::cmd_##name), alias }

        static CMD cmd_defs[] = {
            CLI_COMMAND_ENTRY(dispatcher, a_dispatcher),
            CLI_COMMAND_ENTRY(wdt, a_wdt),
            CLI_COMMAND_ENTRY(mem, a_mem), 
            CLI_COMMAND_ENTRY(reset, a_reset),
            CLI_COMMAND_ENTRY(about, a_about),
            CLI_COMMAND_ENTRY(relay, a_relay),
            CLI_COMMAND_ENTRY(version, a_version),
            CLI_COMMAND_ENTRY(fs, a_fs),
            CLI_COMMAND_ENTRY(logs, a_logs),
            CLI_COMMAND_ENTRY(audio, a_audio)
        };
        static CMDS commands = {
            sizeof(cmd_defs)/sizeof(CMD),
            &cmd_defs[0]
        };
        this->cmds = &commands;
        return &commands;
    };

public:
  CliDevice(Stream * in, Stream * out, CliContext * ctx) : Cli(in, out, ctx) {
    buildCmds();
  };

private:

    //Helper fiunctions for various cmds

    void printInvalidParameter(Stream * out, const char * p){
        out->print("Error. Parameter [");
        out->print(p);
        out->println("] doesn't exist.");
    };

    void printDirectory(Stream * out, File dir, int numTabs, void (*keepAlive)()) {    
        while(1){
            (*keepAlive)();
            File entry =  dir.openNextFile();
            if (!entry) {
                // no more files
                break;
            }
            for(uint8_t i = 0; i < numTabs; i++) {
                out->print('\t');
            }
            out->print(entry.name());
            if(entry.isDirectory()) {
                out->println("/");
                printDirectory(out, entry, numTabs + 1, keepAlive);
            } else {
                // files have sizes, directories do not
                out->print("\t\t");
                out->println(entry.size(), DEC);
            }
            entry.close();
        }
    };

    // int promptForNumber(int min, int max, const char * prompt, const char * errorMsg){
    //     static char num[10];
    //     int n;
    //     int done = 0;
    //     do{
    //         out->print(prompt);
    //         readLine(num, sizeof(num));
    //         n = atoi(num);
    //         if(n >= min && n <= max){
    //             done = 1;
    //         } else {
    //             out->print("Invalid input. Must be a number between ");
    //             out->print(min);
    //             out->print(" and ");
    //             out->println(max);
    //             out->println(errorMsg);
    //     }
    //     }while(!done);

    //     return n;
    // };
};

#endif
