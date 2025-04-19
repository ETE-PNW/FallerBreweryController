#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H

#include "Defaults.h"
//#include "Actions.h"
#include "dispatcher.h"

class Relay;
class AudioBoard;
class Actions;

class CliContext {
public:
  Relay * relay;
  AudioBoard * audio;
  Dispatcher<Actions> * dispatcher;
  void (*keepAlive)();
};

#endif
