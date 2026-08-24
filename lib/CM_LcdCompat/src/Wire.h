#ifndef CM_LCD_COMPAT_WIRE_H
#define CM_LCD_COMPAT_WIRE_H

#if defined(__AVR__)
// Production Uno no longer instantiates Arduino Wire/TwoWire. CM_Pcf8574Lcd
// owns hardware TWI directly and keeps no persistent I2C buffers in SRAM.
class CmWireCompat
{
public:
    void begin() const {}
};

#define Wire CmWireCompat()
#else
// Do not shadow the real framework Wire implementation on ESP32 or other
// targets that own actual TwoWire devices such as the RTC.
#include_next <Wire.h>
#endif

#endif
