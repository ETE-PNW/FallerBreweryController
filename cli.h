#ifndef CLI_H
#define CLI_H

#include <Stream.h>

#include "cliContext.h"
#include "defaults.h"

typedef enum { CMD_OK, CMD_ERROR, CMD_EXIT, CMD_SKIP, CMD_HELP } CMD_RESULT;

class Cli;

typedef int (Cli::*commandHandler)();
typedef void (Cli::*helpHandler)();

typedef struct {
    const char * cmd_name;
    void (Cli::*help_printer)(); // Function that outputs helps for the command
    int (Cli::*cmd_handler)();   // Handler for the command
    const char ** aliases;       // Array of aliases this command will respond to
} CMD;

typedef struct {
    int length;
    CMD * cmds;
} CMDS;

class Cli {

  const char ** getHelpCommands() {
    static const char * help_commands[] = {"help", "h", "hlep", "HELP", "Help", "HeLp", "hlp", nullptr};
    return help_commands;
  }

protected:

    CliContext * ctx;
    CMDS * cmds;

    Stream * in;
    Stream * out;

    int args_length;
    char line[CLI_LINE_BUF_SIZE];
    char * args[CLI_MAX_NUM_ARGS];

    Cli(Stream * in, Stream * out, CliContext * ctx) : in(in), 
                                                out(out),
                                                ctx(ctx){
    };                                            

    char * readLine(char * line, int max){
      if(!in->available()){
        return nullptr;
      }
      int r = in->readBytesUntil('\n', line, max);
      if (r < max) {
        line[r] = '\0';
        return rtrim(line);
      }
      out->println("Input string too long.");
      return nullptr;
    };

    char * readLine(){
      return readLine(this->line, sizeof(this->line));
    };
    
    static int isSpace(char c) {
      return c == '\t' || c == ' ' || c == '\r' || c == '\n';
    };

    static char * rtrim(char * line){
      int i = strlen(line) - 1;
      for (; i >= 0; i--) {
        if (isSpace(line[i])) {
          line[i] = '\0';
        } else {
          break;
        }
      }
      return line;
    };

    int parseLine(){
      char * argument;
      args_length = 0;
        
      memset(args, 0, sizeof(args));

      if(!line || strlen(line) == 0){
        return 0;
      }
        
      argument = strtok(line, " ");
      while(argument != nullptr) {
        if(args_length < CLI_MAX_NUM_ARGS){
          args[args_length++] = argument;
          argument = strtok(nullptr, " ");
        } else {
          break;
        }
      }
      return args_length;
    };

    CMD * findCommand(const char * command) {
      if (!command || *command == '\0') {
        return nullptr;
      }
      for (int i = 0; i < cmds->length; i++) {
        CMD &current = cmds->cmds[i];
        if (strcmp(command, current.cmd_name) == 0) {
          return &current;
        }
        if (current.aliases) {
          for (int j = 0; current.aliases[j] != nullptr; j++) {
            if (strcmp(command, current.aliases[j]) == 0) {
              return &current;
            }
            // For case-insensitive comparison uncomment below:
            // if (strcasecmp(command, current.aliases[j]) == 0) {
            //     return &current;
            // }
          }
        }
      }
      return nullptr;
    }

    bool isSubcommand(const char * subcommand, const char * options[]) const {
      if(subcommand == nullptr)
      return false;
      for (size_t i = 0; options[i] != nullptr; i++) {
        if (strcmp(subcommand, options[i]) == 0) {
          return true;
        }
      }
      return false;
    }

    int noArguments(){
      return args_length == 1;
    };

    //If no argument is passed, we assume it is args[1]
    int isSubcommand(const char * options[]){
      if(args_length>1){
        return isSubcommand(args[1], options);
      }

      return 0;
    };

    // Prints detailed help info for a given command.
    void printCommandHelp(CMD* c) {
      if (c == nullptr) {
        out->println("Invalid command.");
        return;
      }
      out->print("Command [");
      out->print(c->cmd_name);
      out->print("]. ");
      if (c->help_printer) {
        (this->*c->help_printer)();
      } else {
        out->println("No help available");
      }
    };

    void cmdHelp(char * args[]){

      const char ** help_commands = getHelpCommands();

      if(args == nullptr || args[1] == nullptr){
          out->println("The following commands are available:");
          for(int i = 0; i < cmds->length; i++){
            auto cmd = &cmds->cmds[i];
            out->print("  ");
            out->print(cmd->cmd_name);
            if(cmd->aliases){
              out->print("  (");
              int j = 0;
              while(cmd->aliases[j]){
                out->print(cmd->aliases[j]);
                if (cmd->aliases[j + 1]) {
                  out->print(", ");
                }
                j++;
              }
              out->print(")");
            }
            out->println("");
        }
        out->println("");
        return;
      } else {
        if (isSubcommand(help_commands)) {
          out->println("Displays help. You can enter `help {command}`");
          return;
        }
        CMD * c = findCommand(args[1]);
        if(c == nullptr) {
          out->println("Command not found");
          cmdHelp(nullptr);
        } else {
          printCommandHelp(c);
        }
        return;
      }
    };

    int executeCommand(){

        const char ** help_commands = getHelpCommands();

        CMD * c = findCommand(args[0]);
        if(c){
          if(isSubcommand(help_commands)) {
            printCommandHelp(c);
            return CMD_OK;
          }

          auto r = (this->*c->cmd_handler)();
          if(r == CMD_HELP) {
            printCommandHelp(c);
            return CMD_HELP;
          }
          return r;
        }

        //If the command name is "help", show help
        if(isSubcommand(args[0], help_commands)) {
          cmdHelp(args);
          return CMD_OK;
        }

        //If neither help or valid command, return error
        out->println("Invalid command. Type \"help\" for more.");
        return 0;
    };

public:

  int run(){
    int ret = CMD_OK;
    if (!in->available()){
      return CMD_SKIP;
    }
    out->print("> ");
    if(readLine()){
      out->println(line);
      //Serial.println(line);
      if(parseLine()){
        ret = executeCommand();
      }
      out->println("\r\n> ");
    }
    //Reset buffers for next run
    memset(line, 0, sizeof(line));
    memset(args, 0, sizeof(args));
    args_length = 0;
    return ret;
  };
};

#endif
