#include "CM_RepairRegistry.h"

#include "CM_FlatJsonObjectValidator.h"
#include "CM_WindingProgramParser.h"

namespace CM
{
bool RepairRegistry::updateMotor(uint32_t motorId, const NewMotor& motor)
{
    String canonicalProgram;
    bool found = false;
    if (!ready() || motorId == 0UL || motor.name.length() == 0U ||
        motor.repeatTarget == 0U ||
        !WindingProgramParser::canonicalize(motor.coilProgram, canonicalProgram) ||
        !motorExists(motorId, found) || !found)
    {
        return false;
    }

    File file = m_storage.open(MotorRevisionsPath, FILE_APPEND);
    if (!file)
    {
        m_ready = false;
        return false;
    }

    String line;
    line.reserve(1440U);
    line = F("{\"motor_id\":"); line += motorId;
    line += F(",\"name\":\""); line += jsonEscape(motor.name);
    line += F("\",\"model\":\""); line += jsonEscape(motor.model);
    line += F("\",\"manufacturer\":\""); line += jsonEscape(motor.manufacturer);
    line += F("\",\"tags\":\""); line += jsonEscape(motor.tags);
    line += F("\",\"coil_program\":\""); line += canonicalProgram;
    line += F("\",\"repeat_target\":"); line += motor.repeatTarget;
    line += F(",\"status\":\"ACTIVE\"");

    if (motor.ratedPowerW > 0UL)
    {
        line += F(",\"rated_power_w\":"); line += motor.ratedPowerW;
    }
    if (motor.ratedVoltageV > 0U)
    {
        line += F(",\"rated_voltage_v\":"); line += motor.ratedVoltageV;
    }
    if (motor.ratedCurrentMa > 0UL)
    {
        line += F(",\"rated_current_ma\":"); line += motor.ratedCurrentMa;
    }
    if (motor.ratedSpeedRpm > 0U)
    {
        line += F(",\"rated_speed_rpm\":"); line += motor.ratedSpeedRpm;
    }
    if (motor.frequencyHz > 0U)
    {
        line += F(",\"frequency_hz\":"); line += motor.frequencyHz;
    }
    if (motor.phases > 0U)
    {
        line += F(",\"phases\":"); line += motor.phases;
    }
    if (motor.slotCount > 0U)
    {
        line += F(",\"slot_count\":"); line += motor.slotCount;
    }
    if (motor.poleCount > 0U)
    {
        line += F(",\"pole_count\":"); line += motor.poleCount;
    }
    if (motor.coilPitch > 0U)
    {
        line += F(",\"coil_pitch\":"); line += motor.coilPitch;
    }
    if (motor.turnsPerCoil > 0U)
    {
        line += F(",\"turns_per_coil\":"); line += motor.turnsPerCoil;
    }
    if (motor.wireDiameterHundredthsMm > 0U)
    {
        line += F(",\"wire_diameter_hundredths_mm\":");
        line += motor.wireDiameterHundredthsMm;
    }
    if (motor.parallelStrands > 0U)
    {
        line += F(",\"parallel_strands\":"); line += motor.parallelStrands;
    }
    if (motor.statorBoreMm > 0U)
    {
        line += F(",\"stator_bore_mm\":"); line += motor.statorBoreMm;
    }
    if (motor.statorCoreLengthMm > 0U)
    {
        line += F(",\"stator_core_length_mm\":"); line += motor.statorCoreLengthMm;
    }

    const char* optionalNames[] = {
        "connection", "winding_type", "wire_material", "source_type",
        "source_url", "source_title", "source_retrieved_at", "confidence"
    };
    const String* optionalValues[] = {
        &motor.connection, &motor.windingType, &motor.wireMaterial,
        &motor.sourceType, &motor.sourceUrl, &motor.sourceTitle,
        &motor.sourceRetrievedAt, &motor.confidence
    };
    for (uint8_t i = 0U; i < 8U; ++i)
    {
        if (optionalValues[i]->length() == 0U) continue;
        line += F(",\""); line += optionalNames[i]; line += F("\":\"");
        line += jsonEscape(*optionalValues[i]); line += '"';
    }
    if (motor.calculatedFields)
        line += F(",\"calculated_fields\":true");
    if (motor.comment.length() > 0U)
    {
        line += F(",\"comment\":\""); line += jsonEscape(motor.comment); line += '"';
    }
    line += F("}\n");

    const size_t written = file.print(line);
    file.flush();
    file.close();
    if (written != line.length())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RepairRegistry::latestMotorRevisionLine(uint32_t motorId,
                                             String& line,
                                             bool& found) const
{
    line = String();
    found = false;
    if (!ready() || motorId == 0UL) return false;
    if (!m_storage.exists(MotorRevisionsPath)) return true;

    File file = m_storage.open(MotorRevisionsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL ||
        (rawSize > 0U &&
         (!file.seek(static_cast<uint32_t>(rawSize - 1U)) ||
          file.read() != '\n' || !file.seek(0U))))
    {
        file.close();
        return false;
    }

    while (file.available())
    {
        const String candidate = file.readStringUntil('\n');
        if (candidate.length() == 0U) continue;
        uint32_t candidateId = 0UL;
        if (!FlatJsonObjectValidator::valid(candidate) ||
            !findUnsigned(candidate, "motor_id", candidateId) ||
            candidateId == 0UL)
        {
            file.close();
            return false;
        }
        if (candidateId != motorId) continue;
        line = candidate;
        found = true;
    }

    file.close();
    return true;
}
}
