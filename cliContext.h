#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H

#include "Defaults.h"
#include "dispatcher.h"
#include "CBUSConfig.h"

class Relay;
class AudioBoard;
class Actions;

class CliContext {
public:
  Relay * relay;
  AudioBoard * audio;
  Dispatcher<Actions> * dispatcher;
  CBUSConfig * config;
  void (*keepAlive)();
};

#endif
