#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
class RepairLifecycle
{
public:
    static bool isOpen(fs::FS& storage, uint32_t repairId, bool& open)
    {
        open = false;
        if (repairId == 0UL) return false;
        if (!storage.exists(StatusPath))
        {
            open = true;
            return true;
        }

        File file = storage.open(StatusPath, FILE_READ);
        if (!file || file.isDirectory())
        {
            if (file) file.close();
            return false;
        }

        bool found = false;
        while (file.available())
        {
            const String line = file.readStringUntil('\n');
            if (line.length() == 0U) continue;
            if (line[0] != '{' || line[line.length() - 1U] != '}')
            {
                file.close();
                return false;
            }

            uint32_t candidateRepairId = 0UL;
            String status;
            String closedAt;
            if (!findUnsigned(line, "repair_id", candidateRepairId) ||
                candidateRepairId == 0UL ||
                !findString(line, "status", status) || status != "CLOSED" ||
                !findString(line, "closed_at", closedAt) ||
                closedAt.length() < 10U)
            {
                file.close();
                return false;
            }

            if (candidateRepairId != repairId) continue;
            if (found)
            {
                file.close();
                return false;
            }
            found = true;
        }
        file.close();

        open = !found;
        return true;
    }

private:
    static constexpr const char* StatusPath = "/data/workshop/repair-status.ndjson";

    static bool findUnsigned(const String& line, const char* key, uint32_t& value)
    {
        value = 0UL;
        const String marker = String('"') + key + F("\":");
        const int position = line.indexOf(marker);
        if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
            return false;

        int cursor = position + marker.length();
        while (cursor < line.length() && line[cursor] == ' ') ++cursor;
        if (cursor >= line.length() || !isDigit(line[cursor])) return false;
        if (line[cursor] == '0' && cursor + 1 < line.length() &&
            isDigit(line[cursor + 1])) return false;

        uint32_t parsed = 0UL;
        while (cursor < line.length() && isDigit(line[cursor]))
        {
            const uint8_t digit = static_cast<uint8_t>(line[cursor] - '0');
            if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
            parsed = parsed * 10UL + digit;
            ++cursor;
        }
        while (cursor < line.length() && line[cursor] == ' ') ++cursor;
        if (cursor >= line.length() ||
            (line[cursor] != ',' && line[cursor] != '}') || parsed == 0UL)
            return false;

        value = parsed;
        return true;
    }

    static bool findString(const String& line, const char* key, String& value)
    {
        value = String();
        const String marker = String('"') + key + F("\":\"");
        const int position = line.indexOf(marker);
        if (position < 0 || line.indexOf(marker, position + marker.length()) >= 0)
            return false;

        int cursor = position + marker.length();
        while (cursor < line.length())
        {
            const char ch = line[cursor++];
            if (ch == '"')
            {
                while (cursor < line.length() && line[cursor] == ' ') ++cursor;
                return cursor < line.length() &&
                       (line[cursor] == ',' || line[cursor] == '}');
            }
            if (ch == '\\')
            {
                if (cursor >= line.length()) return false;
                const char escaped = line[cursor++];
                if (escaped == '"' || escaped == '\\') value += escaped;
                else if (escaped == 'n') value += '\n';
                else if (escaped == 'r') value += '\r';
                else return false;
                continue;
            }
            if (static_cast<uint8_t>(ch) < 0x20U) return false;
            value += ch;
        }
        return false;
    }
};
}
