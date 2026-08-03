/*
==========================================================
CoilMaster OS
Error Codes
==========================================================
*/

#ifndef CM_ERROR_H
#define CM_ERROR_H

enum CM_ErrorCode
{
    CM_ERROR_NONE = 0,

    CM_ERROR_UNKNOWN,

    CM_ERROR_LCD,

    CM_ERROR_KEYPAD,

    CM_ERROR_HALL,

    CM_ERROR_SSR,

    CM_ERROR_SD,

    CM_ERROR_WIFI,

    CM_ERROR_RTC,

    CM_ERROR_UART,

    CM_ERROR_DATABASE,

    CM_ERROR_PROTOCOL
};

#endif