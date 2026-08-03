/*
==========================================================
CoilMaster OS
CM_System
==========================================================
*/

#include "CM_System.h"

CM_SystemState CM_System::_state = CM_STATE_BOOT;

void CM_System::begin()
{
    initialize();
}

void CM_System::initialize()
{
    Serial.begin(CM_UART_BAUDRATE);

    _state = CM_STATE_INIT;

    selfTest();

    _state = CM_STATE_READY;
}

void CM_System::loop()
{
    update();
}

void CM_System::update()
{
    switch (_state)
    {
        case CM_STATE_READY:
            break;

        case CM_STATE_RUN:
            break;

        case CM_STATE_PAUSE:
            break;

        case CM_STATE_ERROR:
            break;

        default:
            break;
    }
}

void CM_System::selfTest()
{
    Serial.println();
    Serial.println(F("================================"));
    Serial.println(F(" CoilMaster OS"));
    Serial.println(F(" System Boot"));
    Serial.println(F("================================"));
}

void CM_System::shutdown()
{
    _state = CM_STATE_SHUTDOWN;
}

CM_SystemState CM_System::getState()
{
    return _state;
}

void CM_System::setState(CM_SystemState state)
{
    _state = state;
}