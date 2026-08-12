#include "CM_BackupBusinessDataIntegrityAudit.h"
#include "CM_FlatJsonObjectValidator.h"
#include <Arduino.h>
#include "CM_WindingProgramParser.h"

namespace CM
{
namespace
{
constexpr const char* ClientsPath = "/data/workshop/clients.ndjson";
constexpr const char* MotorsPath = "/data/workshop/motors.ndjson";
constexpr const char* RepairsPath = "/data/workshop/repairs.ndjson";
constexpr const char* RepairStatusPath = "/data/workshop/repair-status.ndjson";
constexpr const char* PricingPath = "/data/repairs/pricing.ndjson";
constexpr uint8_t ReferenceBatchSize = 32U;

struct IdReference
{
    uint32_t id;
    bool found;

    IdReference() : id(0UL), found(false) {}
};

struct UniqueIdReference
{
    uint32_t id;
    uint8_t matches;
    bool targetFound;

    UniqueIdReference() : id(0UL), matches(0U), targetFound(false) {}
};

bool findUnsigned64(const String& line, const char* key, uint64_t& value)
{
    value = 0ULL;
    const String marker = String("\"") + key + F("\":");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    if (cursor >= line.length() || !isDigit(line[cursor])) return false;
    if (line[cursor] == '0' && cursor + 1 < line.length() && isDigit(line[cursor + 1]))
        return false;
    uint64_t parsed = 0ULL;
    while (cursor < line.length() && isDigit(line[cursor]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
        if (parsed > (0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++cursor;
    }
    if (cursor >= line.length() || (line[cursor] != ',' && line[cursor] != '}'))
        return false;
    value = parsed;
    return true;
}

bool findUnsigned(const String& line, const char* key, uint32_t& value)
{
    uint64_t wide = 0ULL;
    if (!findUnsigned64(line, key, wide) || wide > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(wide);
    return true;
}

bool findString(const String& line, const char* key, String& value)
{
    value = String();
    const String marker = String("\"") + key + F("\":\"");
    const int pos = line.indexOf(marker);
    if (pos < 0 || line.indexOf(marker, pos + marker.length()) >= 0) return false;
    int cursor = pos + marker.length();
    while (cursor < line.length())
    {
        const char ch = line[cursor++];
        if (ch == '"')
            return cursor < line.length() && (line[cursor] == ',' || line[cursor] == '}');
        if (ch == '\\')
        {
            if (cursor >= line.length()) return false;
            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\') value += escaped;
            else if (escaped == 'n') value += '\n';
            else if (escaped == 'r') value += '\r';
            else if (escaped == 't') value += '\t';
            else return false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}

bool incrementRecordCount(uint32_t& recordCount)
{
    if (recordCount == 0xFFFFFFFFUL) return false;
    ++recordCount;
    return true;
}

bool prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}

bool validateMonotonicIds(fs::FS& storage,
                          const char* path,
                          const char* key,
                          uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(path)) return true;
    File file = storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, key, id) || id == 0UL ||
            id <= previousId || !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }
        previousId = id;
    }
    file.close();
    return true;
}

bool resolveReferences(fs::FS& storage,
                       const char* path,
                       const char* key,
                       IdReference* references,
                       uint8_t count)
{
    if (count == 0U) return true;
    if (!storage.exists(path)) return false;
    File file = storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint8_t unresolved = count;
    while (file.available() && unresolved > 0U)
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!findUnsigned(line, key, candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t index = 0U; index < count; ++index)
        {
            IdReference& reference = references[index];
            if (!reference.found && reference.id == candidate)
            {
                reference.found = true;
                --unresolved;
            }
        }
    }
    file.close();
    return unresolved == 0U;
}

bool validateMotors(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(MotorsPath)) return true;
    File file = storage.open(MotorsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t motorId = 0UL;
        String program;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            motorId <= previousId ||
            !findString(line, "coil_program", program) ||
            !WindingProgramParser::valid(program) ||
            !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }
        previousId = motorId;
    }
    file.close();
    return true;
}

bool validateRepairs(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(RepairsPath)) return true;
    File file = storage.open(RepairsPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousRepairId = 0UL;
    IdReference clientReferences[ReferenceBatchSize];
    IdReference motorReferences[ReferenceBatchSize];
    uint8_t batchCount = 0U;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t repairId = 0UL;
        uint32_t clientId = 0UL;
        uint32_t motorId = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            repairId <= previousRepairId ||
            !findUnsigned(line, "client_id", clientId) || clientId == 0UL ||
            !findUnsigned(line, "motor_id", motorId) || motorId == 0UL ||
            !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }
        previousRepairId = repairId;

        clientReferences[batchCount].id = clientId;
        clientReferences[batchCount].found = false;
        motorReferences[batchCount].id = motorId;
        motorReferences[batchCount].found = false;
        ++batchCount;

        if (batchCount == ReferenceBatchSize)
        {
            if (!resolveReferences(storage, ClientsPath, "client_id",
                                   clientReferences, batchCount) ||
                !resolveReferences(storage, MotorsPath, "motor_id",
                                   motorReferences, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U &&
        (!resolveReferences(storage, ClientsPath, "client_id",
                            clientReferences, batchCount) ||
         !resolveReferences(storage, MotorsPath, "motor_id",
                            motorReferences, batchCount)))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool validateStatusBatch(fs::FS& storage,
                         UniqueIdReference* references,
                         uint8_t count)
{
    if (count == 0U) return true;

    IdReference repairReferences[ReferenceBatchSize];
    for (uint8_t index = 0U; index < count; ++index)
    {
        repairReferences[index].id = references[index].id;
        repairReferences[index].found = false;
        references[index].matches = 0U;
        references[index].targetFound = false;
    }
    if (!resolveReferences(storage, RepairsPath, "repair_id",
                           repairReferences, count))
    {
        return false;
    }
    for (uint8_t index = 0U; index < count; ++index)
        references[index].targetFound = repairReferences[index].found;

    if (!storage.exists(RepairStatusPath)) return false;
    File file = storage.open(RepairStatusPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t candidate = 0UL;
        if (!findUnsigned(line, "repair_id", candidate) || candidate == 0UL)
        {
            file.close();
            return false;
        }
        for (uint8_t index = 0U; index < count; ++index)
        {
            if (references[index].id == candidate)
            {
                if (references[index].matches == 0xFFU)
                {
                    file.close();
                    return false;
                }
                ++references[index].matches;
            }
        }
    }
    file.close();

    for (uint8_t index = 0U; index < count; ++index)
    {
        if (!references[index].targetFound || references[index].matches != 1U)
            return false;
    }
    return true;
}

bool validateRepairStatus(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(RepairStatusPath)) return true;
    File file = storage.open(RepairStatusPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    UniqueIdReference references[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t repairId = 0UL;
        String status;
        String closedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findString(line, "status", status) || status != "CLOSED" ||
            !findString(line, "closed_at", closedAt) || closedAt.length() < 10U ||
            !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }

        references[batchCount].id = repairId;
        references[batchCount].matches = 0U;
        references[batchCount].targetFound = false;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!validateStatusBatch(storage, references, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U && !validateStatusBatch(storage, references, batchCount))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool validatePricingBatch(fs::FS& storage,
                          IdReference* references,
                          uint8_t count)
{
    return resolveReferences(storage, RepairsPath, "repair_id", references, count);
}

bool validatePricing(fs::FS& storage, uint32_t& recordCount)
{
    recordCount = 0UL;
    if (!storage.exists(PricingPath)) return true;
    File file = storage.open(PricingPath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    IdReference references[ReferenceBatchSize];
    uint8_t batchCount = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t repairId = 0UL;
        uint64_t labour = 0ULL;
        uint64_t client = 0ULL;
        String currency;
        String timestamp;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "repair_id", repairId) || repairId == 0UL ||
            !findUnsigned64(line, "labour_cost_minor", labour) ||
            !findUnsigned64(line, "client_price_minor", client) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "timestamp", timestamp) || timestamp.length() < 10U ||
            !incrementRecordCount(recordCount))
        {
            file.close();
            return false;
        }

        references[batchCount].id = repairId;
        references[batchCount].found = false;
        ++batchCount;
        if (batchCount == ReferenceBatchSize)
        {
            if (!validatePricingBatch(storage, references, batchCount))
            {
                file.close();
                return false;
            }
            batchCount = 0U;
        }
    }

    if (batchCount > 0U && !validatePricingBatch(storage, references, batchCount))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}
}

bool BackupBusinessDataIntegrityAudit::checkWorkshopRegistry(
    fs::FS& storage,
    BackupBusinessDataAuditMetrics& metrics)
{
    metrics.clientRecordCount = 0UL;
    metrics.motorRecordCount = 0UL;
    metrics.repairRecordCount = 0UL;
    metrics.repairStatusRecordCount = 0UL;
    metrics.pricingRecordCount = 0UL;

    if (!validateMonotonicIds(storage, ClientsPath, "client_id",
                              metrics.clientRecordCount) ||
        !validateMotors(storage, metrics.motorRecordCount) ||
        !validateRepairs(storage, metrics.repairRecordCount) ||
        !validateRepairStatus(storage, metrics.repairStatusRecordCount))
    {
        return false;
    }
    return true;
}

bool BackupBusinessDataIntegrityAudit::check(fs::FS& storage)
{
    BackupBusinessDataAuditMetrics ignoredMetrics;
    return check(storage, ignoredMetrics);
}

bool BackupBusinessDataIntegrityAudit::check(fs::FS& storage,
                                             BackupBusinessDataAuditMetrics& metrics)
{
    if (!checkWorkshopRegistry(storage, metrics)) return false;

    uint32_t pricingRecordCount = 0UL;
    if (!validatePricing(storage, pricingRecordCount)) return false;
    metrics.pricingRecordCount = pricingRecordCount;
    return true;
}
}
