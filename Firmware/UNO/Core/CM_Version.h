/*
==========================================================
CoilMaster OS
CM_Version
----------------------------------------------------------
Platform : Arduino UNO
==========================================================
*/

#ifndef CM_VERSION_H
#define CM_VERSION_H

#include <Arduino.h>

#include "../../../Shared/Core/CM_Common.h"
#include "../../../Shared/Core/CM_Types.h"

class CM_Version
{
public:

    static void print();

    static const char* project();
    static const char* release();
    static const char* build();
    static const char* package();
    static const char* protocol();

    static const char* compileDate();
    static const char* compileTime();

};

#endif