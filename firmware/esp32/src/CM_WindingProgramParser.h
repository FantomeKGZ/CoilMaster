#ifndef CM_WINDING_PROGRAM_PARSER_H
#define CM_WINDING_PROGRAM_PARSER_H

#include <Arduino.h>

namespace CM
{
class WindingProgramParser
{
public:
    static bool parse(const String& source,
                      uint16_t* turns,
                      uint8_t capacity,
                      uint8_t& count,
                      uint16_t maximumTurns = 9999U)
    {
        count = 0U;
        if (turns == nullptr || capacity == 0U || maximumTurns == 0U)
            return false;

        String normalized = source;
        normalized.trim();
        normalized.replace(" ", "");
        if (normalized.length() == 0U) return false;

        uint32_t value = 0UL;
        bool hasDigit = false;
        bool leadingZero = false;

        for (size_t index = 0U; index <= normalized.length(); ++index)
        {
            const bool atEnd = index == normalized.length();
            const char ch = atEnd ? '\0' : normalized[index];
            const bool separator = ch == '/' || ch == ',' || ch == ';';

            if (atEnd || separator)
            {
                if (!hasDigit || value == 0UL || value > maximumTurns ||
                    count >= capacity)
                {
                    count = 0U;
                    return false;
                }

                turns[count++] = static_cast<uint16_t>(value);
                value = 0UL;
                hasDigit = false;
                leadingZero = false;
                continue;
            }

            if (!isDigit(ch))
            {
                count = 0U;
                return false;
            }
            if (!hasDigit)
            {
                hasDigit = true;
                leadingZero = ch == '0';
            }
            else if (leadingZero)
            {
                count = 0U;
                return false;
            }

            const uint8_t digit = static_cast<uint8_t>(ch - '0');
            if (digit > maximumTurns ||
                value > (static_cast<uint32_t>(maximumTurns) - digit) / 10UL)
            {
                count = 0U;
                return false;
            }
            value = value * 10UL + digit;
        }

        return count > 0U;
    }

    static bool valid(const String& source,
                      uint8_t maximumCoils = 10U,
                      uint16_t maximumTurns = 9999U)
    {
        uint16_t turns[10] = {};
        if (maximumCoils == 0U || maximumCoils > 10U) return false;
        uint8_t count = 0U;
        return parse(source, turns, maximumCoils, count, maximumTurns);
    }

    static bool equivalent(const String& left,
                           const String& right,
                           uint8_t maximumCoils = 10U,
                           uint16_t maximumTurns = 9999U)
    {
        if (maximumCoils == 0U || maximumCoils > 10U) return false;

        uint16_t leftTurns[10] = {};
        uint16_t rightTurns[10] = {};
        uint8_t leftCount = 0U;
        uint8_t rightCount = 0U;
        if (!parse(left, leftTurns, maximumCoils, leftCount, maximumTurns) ||
            !parse(right, rightTurns, maximumCoils, rightCount, maximumTurns) ||
            leftCount != rightCount)
        {
            return false;
        }

        for (uint8_t index = 0U; index < leftCount; ++index)
            if (leftTurns[index] != rightTurns[index]) return false;
        return true;
    }

    static bool canonicalize(const String& source,
                             String& result,
                             uint8_t maximumCoils = 10U,
                             uint16_t maximumTurns = 9999U)
    {
        result = String();
        if (maximumCoils == 0U || maximumCoils > 10U) return false;

        uint16_t turns[10] = {};
        uint8_t count = 0U;
        if (!parse(source, turns, maximumCoils, count, maximumTurns))
            return false;

        result.reserve(static_cast<unsigned int>(count) * 5U);
        for (uint8_t index = 0U; index < count; ++index)
        {
            if (index > 0U) result += '/';
            result += turns[index];
        }
        return true;
    }
};
}

#endif
