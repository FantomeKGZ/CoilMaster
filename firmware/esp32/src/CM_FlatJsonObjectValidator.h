#ifndef CM_FLAT_JSON_OBJECT_VALIDATOR_H
#define CM_FLAT_JSON_OBJECT_VALIDATOR_H

#include <Arduino.h>

namespace CM
{
class FlatJsonObjectValidator
{
public:
    static bool valid(const String& line)
    {
        size_t cursor = 0U;
        skipWhitespace(line, cursor);
        if (cursor >= line.length() || line[cursor] != '{') return false;
        ++cursor;
        skipWhitespace(line, cursor);

        if (cursor < line.length() && line[cursor] == '}')
        {
            ++cursor;
            skipWhitespace(line, cursor);
            return cursor == line.length();
        }

        while (cursor < line.length())
        {
            if (!skipString(line, cursor)) return false;
            skipWhitespace(line, cursor);
            if (cursor >= line.length() || line[cursor] != ':') return false;
            ++cursor;
            skipWhitespace(line, cursor);
            if (!skipPrimitiveValue(line, cursor)) return false;
            skipWhitespace(line, cursor);
            if (cursor >= line.length()) return false;

            if (line[cursor] == '}')
            {
                ++cursor;
                skipWhitespace(line, cursor);
                return cursor == line.length();
            }
            if (line[cursor] != ',') return false;
            ++cursor;
            skipWhitespace(line, cursor);
        }

        return false;
    }

private:
    static void skipWhitespace(const String& line, size_t& cursor)
    {
        while (cursor < line.length())
        {
            const char ch = line[cursor];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') return;
            ++cursor;
        }
    }

    static bool isHexDigit(char ch)
    {
        return (ch >= '0' && ch <= '9') ||
               (ch >= 'a' && ch <= 'f') ||
               (ch >= 'A' && ch <= 'F');
    }

    static bool skipString(const String& line, size_t& cursor)
    {
        if (cursor >= line.length() || line[cursor] != '"') return false;
        ++cursor;
        while (cursor < line.length())
        {
            const char ch = line[cursor++];
            if (ch == '"') return true;
            if (static_cast<uint8_t>(ch) < 0x20U) return false;
            if (ch != '\\') continue;
            if (cursor >= line.length()) return false;

            const char escaped = line[cursor++];
            if (escaped == '"' || escaped == '\\' || escaped == '/' ||
                escaped == 'b' || escaped == 'f' || escaped == 'n' ||
                escaped == 'r' || escaped == 't')
            {
                continue;
            }
            if (escaped != 'u' || cursor + 4U > line.length()) return false;
            for (uint8_t i = 0U; i < 4U; ++i)
            {
                if (!isHexDigit(line[cursor + i])) return false;
            }
            cursor += 4U;
        }
        return false;
    }

    static bool skipNumber(const String& line, size_t& cursor)
    {
        const size_t length = line.length();
        if (cursor >= length) return false;
        if (line[cursor] == '-')
        {
            ++cursor;
            if (cursor >= length) return false;
        }

        if (line[cursor] == '0')
        {
            ++cursor;
            if (cursor < length && line[cursor] >= '0' && line[cursor] <= '9')
                return false;
        }
        else
        {
            if (line[cursor] < '1' || line[cursor] > '9') return false;
            do
            {
                ++cursor;
            }
            while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9');
        }

        if (cursor < length && line[cursor] == '.')
        {
            ++cursor;
            if (cursor >= length || line[cursor] < '0' || line[cursor] > '9')
                return false;
            do
            {
                ++cursor;
            }
            while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9');
        }

        if (cursor < length && (line[cursor] == 'e' || line[cursor] == 'E'))
        {
            ++cursor;
            if (cursor < length && (line[cursor] == '+' || line[cursor] == '-'))
                ++cursor;
            if (cursor >= length || line[cursor] < '0' || line[cursor] > '9')
                return false;
            do
            {
                ++cursor;
            }
            while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9');
        }

        return true;
    }

    static bool skipLiteral(const String& line,
                            size_t& cursor,
                            const char* literal)
    {
        size_t offset = 0U;
        while (literal[offset] != '\0')
        {
            if (cursor + offset >= line.length() ||
                line[cursor + offset] != literal[offset])
            {
                return false;
            }
            ++offset;
        }
        cursor += offset;
        return true;
    }

    static bool skipPrimitiveValue(const String& line, size_t& cursor)
    {
        if (cursor >= line.length()) return false;
        if (line[cursor] == '"') return skipString(line, cursor);
        if (line[cursor] == '-' ||
            (line[cursor] >= '0' && line[cursor] <= '9'))
        {
            return skipNumber(line, cursor);
        }
        if (line[cursor] == 't') return skipLiteral(line, cursor, "true");
        if (line[cursor] == 'f') return skipLiteral(line, cursor, "false");
        if (line[cursor] == 'n') return skipLiteral(line, cursor, "null");
        return false;
    }
};
}

#endif // CM_FLAT_JSON_OBJECT_VALIDATOR_H
