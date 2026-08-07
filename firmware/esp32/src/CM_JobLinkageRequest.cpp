#include "CM_JobLinkageRequest.h"

#include <errno.h>
#include <stdlib.h>

namespace CM
{
JobLinkageRequestResult JobLinkageRequest::parse(bool hasRepairId,
                                                 const String& repairIdText,
                                                 bool hasMotorId,
                                                 const String& motorIdText,
                                                 JobLinkage& linkage)
{
    linkage = JobLinkage();

    if (!hasRepairId && !hasMotorId)
        return JobLinkageRequestResult::Unlinked;

    if (hasRepairId != hasMotorId)
        return JobLinkageRequestResult::Partial;

    uint32_t repairId = 0UL;
    uint32_t motorId = 0UL;
    if (!parseCanonicalId(repairIdText, repairId) ||
        !parseCanonicalId(motorIdText, motorId))
    {
        return JobLinkageRequestResult::Invalid;
    }

    linkage.linked = true;
    linkage.repairId = repairId;
    linkage.motorId = motorId;
    return JobLinkageRequestResult::Linked;
}

bool JobLinkageRequest::parseCanonicalId(const String& text,
                                         uint32_t& value)
{
    value = 0UL;
    if (text.length() == 0U || text[0] == '0') return false;

    for (size_t index = 0U; index < text.length(); ++index)
    {
        if (!isDigit(text[index])) return false;
    }

    errno = 0;
    char* parseEnd = nullptr;
    const unsigned long parsed = strtoul(text.c_str(), &parseEnd, 10);
    if (errno == ERANGE || parseEnd == nullptr || *parseEnd != '\0' ||
        parsed == 0UL || parsed > 0xFFFFFFFFUL)
    {
        return false;
    }

    value = static_cast<uint32_t>(parsed);
    return String(value) == text;
}
}
