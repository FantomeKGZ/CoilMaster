#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "CM_WindingJournalQuery.h"

namespace CM
{
class WindingJournalWeb
{
public:
    WindingJournalWeb(WebServer& server,
                      WindingJournalQuery& query);

    void begin();

private:
    static constexpr uint16_t DefaultLimit = 50U;
    static constexpr uint16_t MaximumLimit = 100U;

    void handleHistory();
    static bool parseCanonicalUint32(const String& text,
                                     uint32_t& value);
    static bool parseLimit(const String& text,
                           uint16_t& value);

    WebServer& m_server;
    WindingJournalQuery& m_query;
};
}
