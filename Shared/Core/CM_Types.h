/*
==========================================================
CoilMaster OS
Common Types
----------------------------------------------------------
File      : CM_Types.h
Module    : Shared/Core
Release   : 0.1.0
Build     : 002A
Package   : 01.1
==========================================================
*/

#ifndef CM_TYPES_H
#define CM_TYPES_H

#include <stdint.h>

//----------------------------------------------------------
// Common Types
//----------------------------------------------------------

typedef uint8_t     CM_U8;
typedef int8_t      CM_S8;

typedef uint16_t    CM_U16;
typedef int16_t     CM_S16;

typedef uint32_t    CM_U32;
typedef int32_t     CM_S32;

typedef uint64_t    CM_U64;
typedef int64_t     CM_S64;

typedef float       CM_F32;
typedef double      CM_F64;

typedef bool        CM_BOOL;

//----------------------------------------------------------
// Version
//----------------------------------------------------------

struct CM_VersionInfo
{
    const char* project;
    const char* release;
    const char* build;
    const char* package;
    const char* protocol;
};

//----------------------------------------------------------
// Build Information
//----------------------------------------------------------

struct CM_BuildInfo
{
    const char* compileDate;
    const char* compileTime;
};

//----------------------------------------------------------
// Device Information
//----------------------------------------------------------

struct CM_DeviceInfo
{
    const char* board;
    const char* firmware;
    CM_U8 role;
};

//----------------------------------------------------------
// UART Packet Header
//----------------------------------------------------------

struct CM_PacketHeader
{
    CM_U16 start;
    CM_U16 length;
    CM_U16 command;
    CM_U16 crc;
};

#endif