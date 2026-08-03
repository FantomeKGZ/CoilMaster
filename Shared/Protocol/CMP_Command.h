/*
==========================================================
CoilMaster OS
CMP (CoilMaster Protocol)

File      : CMP_Command.h
Module    : Shared/Protocol

Description:
Protocol command identifiers.
==========================================================
*/

#ifndef CMP_COMMAND_H
#define CMP_COMMAND_H

#include <stdint.h>

enum class CMP_Command : uint16_t
{
    //------------------------------------------------------
    // 0x0000 - System
    //------------------------------------------------------

    NONE                = 0x0000,

    PING                = 0x0001,
    PONG                = 0x0002,

    BOOT                = 0x0003,
    READY               = 0x0004,

    RESET               = 0x0005,
    SHUTDOWN            = 0x0006,

    VERSION_REQUEST     = 0x0010,
    VERSION_RESPONSE    = 0x0011,

    STATUS_REQUEST      = 0x0012,
    STATUS_RESPONSE     = 0x0013,

    //------------------------------------------------------
    // 0x0100 - Configuration
    //------------------------------------------------------

    SETTINGS_READ       = 0x0100,
    SETTINGS_WRITE      = 0x0101,

    CONFIG_READ         = 0x0102,
    CONFIG_WRITE        = 0x0103,

    //------------------------------------------------------
    // 0x0200 - Winding
    //------------------------------------------------------

    WINDING_START       = 0x0200,
    WINDING_STOP        = 0x0201,
    WINDING_PAUSE       = 0x0202,
    WINDING_RESUME      = 0x0203,

    TURN_INCREMENT      = 0x0210,
    TURN_DECREMENT      = 0x0211,

    WINDING_FINISHED    = 0x0212,

    //------------------------------------------------------
    // 0x0300 - Hall Sensor
    //------------------------------------------------------

    HALL_TRIGGER        = 0x0300,

    HALL_CALIBRATION    = 0x0301,

    //------------------------------------------------------
    // 0x0400 - Display
    //------------------------------------------------------

    LCD_CLEAR           = 0x0400,

    LCD_PRINT           = 0x0401,

    LCD_BACKLIGHT       = 0x0402,

    //------------------------------------------------------
    // 0x0500 - Logger
    //------------------------------------------------------

    LOG_MESSAGE         = 0x0500,

    LOG_CLEAR           = 0x0501,

    //------------------------------------------------------
    // 0x0600 - Storage
    //------------------------------------------------------

    STORAGE_SAVE        = 0x0600,

    STORAGE_LOAD        = 0x0601,

    BACKUP_CREATE       = 0x0602,

    BACKUP_RESTORE      = 0x0603,

    //------------------------------------------------------
    // 0x0700 - Network
    //------------------------------------------------------

    WIFI_CONNECT        = 0x0700,

    WIFI_DISCONNECT     = 0x0701,

    FTP_START           = 0x0702,

    FTP_STOP            = 0x0703,

    //------------------------------------------------------
    // 0x0800 - Diagnostics
    //------------------------------------------------------

    SELF_TEST           = 0x0800,

    ERROR_REPORT        = 0x0801,

    WARNING_REPORT      = 0x0802,

    //------------------------------------------------------
    // Reserved
    //------------------------------------------------------

    RESERVED            = 0xFFFE,

    UNKNOWN             = 0xFFFF
};

#endif