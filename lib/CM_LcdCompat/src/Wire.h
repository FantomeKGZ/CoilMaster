#ifndef CM_LCD_COMPAT_WIRE_H
#define CM_LCD_COMPAT_WIRE_H

// Compatibility shim for the production entrypoint. CoilMaster no longer
// instantiates Arduino Wire/TwoWire on Uno; CM_Pcf8574Lcd owns hardware TWI
// directly and keeps no persistent I2C buffers in SRAM.

#endif
