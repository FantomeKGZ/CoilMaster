/*
==========================================================
CoilMaster OS
CM_Settings
==========================================================
*/

#include "CM_Settings.h"

uint16_t CM_Settings::hallThreshold = 500;
bool CM_Settings::buzzerEnabled = true;
uint8_t CM_Settings::brightness = 100;

void CM_Settings::begin()
{
    load();
}

void CM_Settings::load()
{
    // Пока используются значения по умолчанию.
    // В будущем:
    // Arduino -> EEPROM
    // ESP32   -> NVS / SD Card
}

void CM_Settings::save()
{
    // Будет реализовано в следующих пакетах.
}