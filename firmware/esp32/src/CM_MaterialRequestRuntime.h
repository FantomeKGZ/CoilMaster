#ifndef CM_MATERIAL_REQUEST_RUNTIME_H
#define CM_MATERIAL_REQUEST_RUNTIME_H

#include <WebServer.h>

#include "CM_RepairRegistry.h"

namespace CM
{
bool beginMaterialRequestRuntime(WebServer& server, RepairRegistry& repairs);
}

#endif // CM_MATERIAL_REQUEST_RUNTIME_H
