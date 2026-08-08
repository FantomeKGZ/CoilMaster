#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class WindingSessionCompletionCheck : uint8_t
{
    Completed,
    NotCompleted,
    StorageUnavailable,
    IntegrityFailed
};

class WindingSessionCompletionAudit
{
public:
    static WindingSessionCompletionCheck check(fs::FS& storage,
                                               uint32_t sessionId);
};
}
