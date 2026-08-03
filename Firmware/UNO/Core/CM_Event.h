/*
==========================================================
CoilMaster OS
CM_Event
Platform : Arduino UNO
==========================================================
*/

#ifndef CM_EVENT_H
#define CM_EVENT_H

#include <Arduino.h>

enum CM_EventID
{
    //------------------------------------------------------
    // System
    //------------------------------------------------------

    CM_EVENT_NONE = 0,

    CM_EVENT_SYSTEM_BOOT,
    CM_EVENT_SYSTEM_READY,
    CM_EVENT_SYSTEM_SHUTDOWN,

    //------------------------------------------------------
    // Communication
    //------------------------------------------------------

    CM_EVENT_UART_CONNECTED,
    CM_EVENT_UART_DISCONNECTED,

    //------------------------------------------------------
    // Hall Sensor
    //------------------------------------------------------

    CM_EVENT_HALL_TRIGGER,

    //------------------------------------------------------
    // Winding
    //------------------------------------------------------

    CM_EVENT_WINDING_START,
    CM_EVENT_WINDING_STOP,
    CM_EVENT_WINDING_FINISH,

    //------------------------------------------------------
    // Storage
    //------------------------------------------------------

    CM_EVENT_SAVE,
    CM_EVENT_LOAD,

    //------------------------------------------------------
    // Error
    //------------------------------------------------------

    CM_EVENT_ERROR
};

class CM_Event
{
public:

    static void begin();

    static void send(CM_EventID event);

    static CM_EventID lastEvent();

private:

    static CM_EventID _lastEvent;

};

#endif