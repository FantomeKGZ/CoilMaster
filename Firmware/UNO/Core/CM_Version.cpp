/*
==========================================================
CoilMaster OS
CM_Version
==========================================================
*/

#include "CM_Version.h"

void CM_Version::print()
{
    Serial.println(F("--------------------------------"));

    Serial.print(F("Project : "));
    Serial.println(project());

    Serial.print(F("Release : "));
    Serial.println(release());

    Serial.print(F("Build   : "));
    Serial.println(build());

    Serial.print(F("Package : "));
    Serial.println(package());

    Serial.print(F("Protocol: "));
    Serial.println(protocol());

    Serial.print(F("Compile : "));
    Serial.print(compileDate());
    Serial.print(F(" "));
    Serial.println(compileTime());

    Serial.println(F("--------------------------------"));
}

const char* CM_Version::project()
{
    return CM_PROJECT_NAME;
}

const char* CM_Version::release()
{
    return CM_RELEASE;
}

const char* CM_Version::build()
{
    return CM_BUILD;
}

const char* CM_Version::package()
{
    return CM_PACKAGE;
}

const char* CM_Version::protocol()
{
    return CM_PROTOCOL_VERSION;
}

const char* CM_Version::compileDate()
{
    return CM_BUILD_DATE;
}

const char* CM_Version::compileTime()
{
    return CM_BUILD_TIME;
}