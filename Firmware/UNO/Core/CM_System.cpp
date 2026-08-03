/*
==========================================================
CoilMaster OS
CM_System
==========================================================
*/

#include "CM_System.h"
#include "CM_Version.h"
#include "CM_Config.h"
#include "CM_Logger.h"


CM_SystemState CM_System::_state = CM_STATE_BOOT;

void CM_System::begin()
{
    initialize();
}

void CM_System::initialize()
{
    Serial.begin(CM_Config::UART_BAUDRATE);
    CM_Logger::begin();

	CM_Logger::info("System initialization...");

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
	CM_Version::print();
	CM_Logger::info("Self test started.");
	CM_Logger::info("Core OK.");
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