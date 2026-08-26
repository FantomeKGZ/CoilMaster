#include "CM_CashPaymentStore.h"

#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool prepareCashNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t size = file.size();
    if (size > 0xFFFFFFFFUL) return false;
    if (size == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(size - 1U)) || file.read() != '\n') return false;
    return file.seek(0U);
}

bool validCashKindDirection(const String& kind, const String& direction)
{
    if (kind == "PAYMENT") return direction == "ADD";
    if (kind == "CORRECTION") return direction == "ADD" || direction == "SUBTRACT";
    return false;
}

bool addCashChecked(uint64_t& target, uint64_t value)
{
    if (target > 0xFFFFFFFFFFFFFFFFULL - value) return false;
    target += value;
    return true;
}
}

bool CashPaymentStore::eventBelongsToRepair(uint32_t eventId,
                                            uint32_t repairId,
                                            uint32_t clientId,
                                            bool& found) const
{
    found = false;
    if (!ready() || eventId == 0UL || repairId == 0UL || clientId == 0UL)
        return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareCashNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previous = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t current = 0UL, currentRepair = 0UL, currentClient = 0UL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", current) || current == 0UL ||
            current <= previous ||
            !findUnsigned(line, "repair_id", currentRepair) || currentRepair == 0UL ||
            !findUnsigned(line, "client_id", currentClient) || currentClient == 0UL)
        {
            file.close();
            return false;
        }
        previous = current;
        if (current == eventId)
        {
            found = currentRepair == repairId && currentClient == clientId;
            file.close();
            return true;
        }
        if (current > eventId) break;
    }
    file.close();
    return true;
}

bool CashPaymentStore::totalsForClient(uint32_t clientId,
                                       CashClientTotals& totals) const
{
    totals = CashClientTotals();
    if (!ready() || clientId == 0UL) return false;
    if (!m_storage.exists(Path)) return true;

    File file = m_storage.open(Path, FILE_READ);
    if (!prepareCashNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    uint64_t added = 0ULL, subtracted = 0ULL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, lineClient = 0UL;
        uint64_t amount = 0ULL;
        String kind, direction, currency, occurredAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "cash_event_id", id) || id == 0UL ||
            id <= previousId ||
            !findUnsigned(line, "client_id", lineClient) || lineClient == 0UL ||
            !findUnsigned64(line, "amount_minor", amount) || amount == 0ULL ||
            !findString(line, "kind", kind) ||
            !findString(line, "direction", direction) ||
            !findString(line, "currency", currency) || currency.length() != 3U ||
            !findString(line, "occurred_at", occurredAt) || occurredAt.length() < 10U ||
            !validCashKindDirection(kind, direction))
        {
            file.close();
            return false;
        }
        previousId = id;
        if (lineClient != clientId) continue;

        if (!totals.currencySet)
        {
            totals.currency = currency;
            totals.currencySet = true;
        }
        else if (totals.currency != currency)
        {
            file.close();
            return false;
        }
        if (totals.eventCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        ++totals.eventCount;
        if (direction == "ADD")
        {
            if (!addCashChecked(added, amount))
            {
                file.close();
                return false;
            }
        }
        else if (!addCashChecked(subtracted, amount))
        {
            file.close();
            return false;
        }
    }
    file.close();

    if (subtracted > added) return false;
    totals.paidMinor = added - subtracted;
    return true;
}
}
