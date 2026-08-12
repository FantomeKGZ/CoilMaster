#include "CM_MotorSimilarityWeb.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
MotorSimilarityWeb::MotorSimilarityWeb(WebServer& server, RepairRegistry& registry)
    : m_server(server), m_registry(registry) {}

void MotorSimilarityWeb::begin()
{
    m_server.on("/api/motors/similar", HTTP_GET,
                [this]() { handleLookup(); });
}

void MotorSimilarityWeb::handleLookup()
{
    if (!m_registry.ready())
    {
        m_server.send(503, "application/json; charset=utf-8",
                      "{\"error\":\"repair_registry_unavailable\"}");
        return;
    }
    if (!m_server.hasArg("coil_program") ||
        m_server.arg("coil_program").length() == 0U)
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"coil_program_required\"}");
        return;
    }
    if (!WindingProgramParser::valid(m_server.arg("coil_program")))
    {
        m_server.send(400, "application/json; charset=utf-8",
                      "{\"error\":\"invalid_coil_program\"}");
        return;
    }

    NewMotor candidate;
    candidate.name = m_server.arg("name");
    candidate.model = m_server.arg("model");
    candidate.manufacturer = m_server.arg("manufacturer");
    candidate.coilProgram = m_server.arg("coil_program");

    String response = F("{\"items\":[");
    response.reserve(4096U);
    uint16_t sameProgramCount = 0U;
    uint16_t identityMatchCount = 0U;
    uint8_t returnedCount = 0U;
    bool itemsTruncated = false;
    if (!m_registry.appendSimilarMotorsJson(response, candidate,
                                            sameProgramCount,
                                            identityMatchCount,
                                            returnedCount,
                                            itemsTruncated))
    {
        m_server.send(500, "application/json; charset=utf-8",
                      "{\"error\":\"similarity_lookup_failed\"}");
        return;
    }
    response += F("],\"same_program_count\":");
    response += sameProgramCount;
    response += F(",\"identity_match_count\":");
    response += identityMatchCount;
    response += F(",\"returned_count\":");
    response += static_cast<unsigned int>(returnedCount);
    response += F(",\"max_items\":");
    response += static_cast<unsigned int>(RepairRegistry::MaxListPageSize);
    response += F(",\"items_truncated\":");
    response += itemsTruncated ? F("true") : F("false");
    response += F(",\"creation_blocked\":false}");
    m_server.send(200, "application/json; charset=utf-8", response);
}
}
