#pragma once

#include <stdio.h>
#include <stdint.h>

struct StateStruct;

// base class for state machines
// modified to take uint32_t instead of EventData *
class StateMachine
{
public:
    StateMachine(unsigned char maxStates);
    virtual ~StateMachine() {}
protected:
    enum { EVENT_IGNORED = 0xFE, CANNOT_HAPPEN };
    unsigned char currentState;
    void ExternalEvent(unsigned char, uint32_t = 0);
    void InternalEvent(unsigned char, uint32_t = 0);
    virtual const StateStruct* GetStateMap() = 0;
private:
    const unsigned char _maxStates;
    bool _eventGenerated;
    uint32_t _pEventData;
    void StateEngine(void);
};

typedef void (StateMachine::*StateFunc)(uint32_t);
struct StateStruct
{
    StateFunc pStateFunc;
};

#define BEGIN_STATE_MAP \
public:\
const StateStruct* GetStateMap() {\
    static const StateStruct StateMap[] = {

#define STATE_MAP_ENTRY(stateFunc)\
    { reinterpret_cast<StateFunc>(stateFunc) },

#define END_STATE_MAP \
    }; \
    return &StateMap[0]; }

#define BEGIN_TRANSITION_MAP() \
    static const unsigned char TRANSITIONS[] = {\

#define TRANSITION_MAP_ENTRY(entry)\
    entry,

#define END_TRANSITION_MAP(data) \
    0 };\
    ExternalEvent(TRANSITIONS[currentState], data);
