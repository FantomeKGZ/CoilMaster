/*
==========================================================
CoilMaster OS
Status Flags
==========================================================
*/

#ifndef CM_STATUS_H
#define CM_STATUS_H

enum CM_ModuleStatus
{
    CM_STATUS_OFFLINE = 0,

    CM_STATUS_STARTING,

    CM_STATUS_READY,

    CM_STATUS_RUNNING,

    CM_STATUS_WARNING,

    CM_STATUS_ERROR
};

#endif