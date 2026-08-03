/*
==========================================================
CoilMaster OS
Common Definitions
----------------------------------------------------------
File      : CM_Common.h
Module    : Shared/Core
Release   : 0.1.0
Build     : 002A
Package   : 01.1
Author    : CoilMaster Project
==========================================================
*/

#ifndef CM_COMMON_H
#define CM_COMMON_H

//----------------------------------------------------------
// Project Information
//----------------------------------------------------------

#define CM_PROJECT_NAME        "CoilMaster OS"
#define CM_RELEASE             "0.1.0"
#define CM_BUILD               "002A"
#define CM_PACKAGE             "01.1"

#define CM_PROTOCOL_NAME       "CMP"
#define CM_PROTOCOL_VERSION    "1.0"

//----------------------------------------------------------
// Platform
//----------------------------------------------------------

#define CM_PLATFORM_UNO        1
#define CM_PLATFORM_ESP32      2

//----------------------------------------------------------
// Device Role
//----------------------------------------------------------

#define CM_ROLE_REALTIME       1
#define CM_ROLE_SERVICE        2

//----------------------------------------------------------
// Device Name
//----------------------------------------------------------

#define CM_DEVICE_UNO          "Arduino UNO R3"
#define CM_DEVICE_ESP32        "ESP32 DevKit V1"

//----------------------------------------------------------
// Build Information
//----------------------------------------------------------

#define CM_BUILD_DATE          __DATE__
#define CM_BUILD_TIME          __TIME__

//----------------------------------------------------------
// General Definitions
//----------------------------------------------------------

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//----------------------------------------------------------
// System Status
//----------------------------------------------------------

enum CM_SystemState
{
    CM_STATE_BOOT = 0,
    CM_STATE_INIT,
    CM_STATE_READY,
    CM_STATE_RUN,
    CM_STATE_PAUSE,
    CM_STATE_ERROR,
    CM_STATE_SHUTDOWN
};

//----------------------------------------------------------
// Device Type
//----------------------------------------------------------

enum CM_DeviceType
{
    CM_DEVICE_UNKNOWN = 0,
    CM_DEVICE_UNO_BOARD,
    CM_DEVICE_ESP32_BOARD
};

//----------------------------------------------------------
// Module Result
//----------------------------------------------------------

enum CM_Result
{
    CM_OK = 0,
    CM_WARNING,
    CM_FAILED,
    CM_NOT_SUPPORTED,
    CM_TIMEOUT
};

#endif