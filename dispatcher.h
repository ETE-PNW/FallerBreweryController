#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <array>
#include "Logger.h"

#define MAX_ACTIONS 15

extern ConsoleLogger trace;
extern ConsoleLogger info;
extern FileLogger error;

template<typename T>
class DispatcherAction {
public:
    const char *name;          // A four letter name that can be used for execution
    const char *long_name;     // Name of the action
    void (T::*handler)();      // The handler member function of T
    int ticks;                 // Number of ticks (1 tick = 1 min)
    int count;                 // Used to keep track of counts for the action
    int once;                  // Schedules a one-time execution when equal to 0
};

template<typename T>
class Dispatcher {
private:
    std::array<DispatcherAction<T>, MAX_ACTIONS> actions;
    unsigned long lastMillis;
    int disabled;
    int len;
    T *instance;

    void logMemoryUsage(const char *context) {
        trace.log("Dispatcher", context, Utils::freeMemory());
    }

    void executeAction(DispatcherAction<T> &action) {
        logMemoryUsage("Memory before: ");
        (instance->*action.handler)();
        logMemoryUsage("Memory after: ");
    }

public:
    Dispatcher(T *instance) : lastMillis(0), disabled(0), len(0), instance(instance) {}

    int add(const char *name, const char *long_name, void (T::*handler)(), int _ticks) {
        if (len == MAX_ACTIONS) {
            error.log("Dispatcher", "Max number of actions added.", MAX_ACTIONS);
            return -1;
        }
        actions[len].name = name;
        actions[len].long_name = long_name;
        actions[len].handler = handler;
        actions[len].ticks = _ticks;
        actions[len].count = 0;
        actions[len].once = -1;
        len++;
        return len;
    }

    void updateActionTicks(int actionIndex, int _ticks) {
        if (actionIndex >= 0 && actionIndex < len && _ticks >= -1) {
            actions[actionIndex].ticks = _ticks;
        }
    }

    void runActionOnce(const char *name, int _ticks) {
        if (_ticks < 0) return;
        for (int x = 0; x < len; x++) {
            if (strcmp(name, actions[x].name) == 0) {
                actions[x].once = _ticks;
                return;
            }
        }
    }

    void disableAllActions() {
        disabled = 1;
    }

    void enableAllActions() {
        disabled = 0;
    }

    int status() const {
        return !disabled;
    }

    void disableAction(int actionIndex) {
        updateActionTicks(actionIndex, -1);
    }

    void dispatch() {
        if (disabled) return;

        unsigned long current = millis();
        if ((current >= lastMillis && (current - lastMillis) >= TICK_IN_MILLIS) ||
            (lastMillis > current && (lastMillis - current) >= TICK_IN_MILLIS)) {
            lastMillis = current;
            step();
        }
    }

    void step() {
        for (int x = 0; x < len; x++) {
            DispatcherAction<T> &action = actions[x];

            if (action.once == 0) {
                action.once = -1;
                trace.log("Dispatcher", "Running once ready for: ", action.long_name);
                executeAction(action);
                continue;
            } else if (action.once > 0) {
                trace.log("Dispatcher", "Decrementing running once: ", action.long_name);
                action.once--;
            }

            if (action.ticks < 0) continue;

            action.count++;
            if (action.count >= action.ticks) {
                trace.log("Dispatcher", "Action ready: ", action.long_name);
                executeAction(action);
                action.count = 0;
            }
        }
    }

    void execute(const char *name) {
        trace.log("Dispatcher", "Looking to execute action: ", name);
        for (int x = 0; x < len; x++) {
            if (strcmp(name, actions[x].name) == 0) {
                execute(x);
                return;
            }
        }
        error.log("Dispatcher", "Invalid action: ", name);
    }

    void execute(int actionIndex) {
        if (actionIndex >= 0 && actionIndex < len) {
            trace.log("Dispatcher", "Executing action ", actions[actionIndex].name);
            executeAction(actions[actionIndex]);
        } else {
            error.log("Dispatcher", "Invalid action index: ", actionIndex);
        }
    }

    int getActionsLength() const {
        return len;
    }

    const DispatcherAction<T>* getAction(int actionIndex) const {
        if (actionIndex >= 0 && actionIndex < len) {
            return &actions[actionIndex];
        }
        return nullptr;
    }

    int secondsToNextRun(int actionIndex) const {
        if (actionIndex >= 0 && actionIndex < len) {
            if (actions[actionIndex].ticks < 0) {
                return -1;
            }
            return (actions[actionIndex].ticks - actions[actionIndex].count) * 1000 / TICK_IN_MILLIS;
        } else {
            error.log("Dispatcher", "Invalid action index: ", actionIndex);
            return -1;
        }
    }

    int scheduleForImmediateExecution(int actionIndex) {
        if (actionIndex >= 0 && actionIndex < len) {
            trace.log("Dispatcher", "Set count = ticks for immediate execution for action ", actions[actionIndex].name);
            actions[actionIndex].count = actions[actionIndex].ticks;
            return 0;
        } else {
            error.log("Dispatcher", "Invalid action index: ", actionIndex);
            return -1;
        }
    }
};

#endif // DISPATCHER_H
