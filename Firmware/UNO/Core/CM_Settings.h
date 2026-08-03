/*
==========================================================
CoilMaster OS
CM_Settings
Platform : Arduino UNO
==========================================================
*/

#ifndef CM_SETTINGS_H
#define CM_SETTINGS_H

#include <Arduino.h>

class CM_Settings
{
public:

    static void begin();

    static void load();

    static void save();

    //------------------------------------------------------
    // Hall Sensor
    //------------------------------------------------------

    static uint16_t hallThreshold;

    //------------------------------------------------------
    // Sound
    //------------------------------------------------------

    static bool buzzerEnabled;

    //------------------------------------------------------
    // Display
    //------------------------------------------------------

    static uint8_t brightness;

};

#endif