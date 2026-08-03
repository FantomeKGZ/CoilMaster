/*
==========================================================
CoilMaster OS
CM_Logger
Platform : Arduino UNO
==========================================================
*/

#ifndef CM_LOGGER_H
#define CM_LOGGER_H

#include <Arduino.h>

enum CM_LogLevel
{
    CM_LOG_INFO = 0,
    CM_LOG_WARNING,
    CM_LOG_ERROR,
    CM_LOG_DEBUG
};

class CM_Logger
{
public:

    static void begin();

    static void info(const char* text);

    static void warning(const char* text);

    static void error(const char* text);

    static void debug(const char* text);

private:

    static void write(CM_LogLevel level, const char* text);

};

#endif