/*
==========================================================
CoilMaster OS
CM_Logger
==========================================================
*/

#include "CM_Logger.h"

void CM_Logger::begin()
{
}

void CM_Logger::info(const char* text)
{
    write(CM_LOG_INFO, text);
}

void CM_Logger::warning(const char* text)
{
    write(CM_LOG_WARNING, text);
}

void CM_Logger::error(const char* text)
{
    write(CM_LOG_ERROR, text);
}

void CM_Logger::debug(const char* text)
{
    write(CM_LOG_DEBUG, text);
}

void CM_Logger::write(CM_LogLevel level, const char* text)
{
    switch(level)
    {
        case CM_LOG_INFO:
            Serial.print(F("[INFO] "));
            break;

        case CM_LOG_WARNING:
            Serial.print(F("[WARN] "));
            break;

        case CM_LOG_ERROR:
            Serial.print(F("[ERROR] "));
            break;

        case CM_LOG_DEBUG:
            Serial.print(F("[DEBUG] "));
            break;
    }

    Serial.println(text);
}