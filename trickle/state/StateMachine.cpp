#include <assert.h>
#include "StateMachine.h"

StateMachine::StateMachine(unsigned char maxStates) :
    _maxStates(maxStates),
    currentState(0),
    _eventGenerated(false),
    _pEventData(0)
{
}

// generates an external event. called once per external event
// to start the state machine executing
void StateMachine::ExternalEvent(unsigned char newState,
                                 uint32_t pData)
{
    // if we are supposed to ignore this event
    if (newState == EVENT_IGNORED) {
        // just delete the event data, if any
        pData = 0;
    }
    else {
		// TODO - capture software lock here for thread-safety if necessary

        // generate the event and execute the state engine
        InternalEvent(newState, pData);
        StateEngine();

		// TODO - release software lock here
    }
}

// generates an internal event. called from within a state
// function to transition to a new state
void StateMachine::InternalEvent(unsigned char newState,
                                 uint32_t pData)
{
    _pEventData = pData;
    _eventGenerated = true;
    currentState = newState;
}

// the state engine executes the state machine states
void StateMachine::StateEngine(void)
{
    uint32_t pDataTemp = 0;

    // while events are being generated keep executing states
    while (_eventGenerated) {
        pDataTemp = _pEventData;  // copy of event data pointer
        _pEventData = 0;          // event data used up, reset ptr
        _eventGenerated = false;  // event used up, reset flag

        assert(currentState < _maxStates);

		// get state map
        const StateStruct* pStateMap = GetStateMap();

        // execute the state passing in event data, if any
        (this->*pStateMap[currentState].pStateFunc)(pDataTemp);

        // if event data was used, then delete it
        if (pDataTemp) {
            pDataTemp = 0;
        }
    }
}