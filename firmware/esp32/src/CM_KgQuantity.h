#ifndef CM_KG_QUANTITY_H
#define CM_KG_QUANTITY_H

#include <Arduino.h>

namespace CM
{
class KgQuantity
{
public:
    // Parse operator-entered kilograms without floating point. The canonical
    // warehouse resolution is one gram, therefore at most three fractional
    // decimal digits are accepted (for example: 1, 1.2, 1.250, 0.001).
    static bool parseGrams(const String& kilograms, uint32_t& grams)
    {
        grams = 0UL;
        if (kilograms.length() == 0U) return false;

        const int separator = kilograms.indexOf('.');
        if (separator != kilograms.lastIndexOf('.')) return false;

        const int wholeEnd = separator >= 0 ? separator : kilograms.length();
        if (wholeEnd <= 0) return false;
        if (wholeEnd > 7) return false; // keeps kg*1000 inside uint32_t below

        uint32_t wholeKg = 0UL;
        for (int i = 0; i < wholeEnd; ++i)
        {
            const char ch = kilograms[i];
            if (ch < '0' || ch > '9') return false;
            if (i == 0 && wholeEnd > 1 && ch == '0') return false;
            const uint8_t digit = static_cast<uint8_t>(ch - '0');
            if (wholeKg > (0xFFFFFFFFUL - digit) / 10UL) return false;
            wholeKg = wholeKg * 10UL + digit;
        }

        uint32_t fractionalGrams = 0UL;
        if (separator >= 0)
        {
            const int fractionalDigits = kilograms.length() - separator - 1;
            if (fractionalDigits <= 0 || fractionalDigits > 3) return false;
            uint32_t fraction = 0UL;
            for (int i = separator + 1; i < kilograms.length(); ++i)
            {
                const char ch = kilograms[i];
                if (ch < '0' || ch > '9') return false;
                fraction = fraction * 10UL + static_cast<uint8_t>(ch - '0');
            }
            if (fractionalDigits == 1) fractionalGrams = fraction * 100UL;
            else if (fractionalDigits == 2) fractionalGrams = fraction * 10UL;
            else fractionalGrams = fraction;
        }

        if (wholeKg > (0xFFFFFFFFUL - fractionalGrams) / 1000UL) return false;
        const uint32_t parsed = wholeKg * 1000UL + fractionalGrams;
        if (parsed == 0UL) return false;
        grams = parsed;
        return true;
    }

    static String canonicalKg(uint32_t grams)
    {
        if (grams == 0UL) return String();
        const uint32_t whole = grams / 1000UL;
        const uint16_t fraction = static_cast<uint16_t>(grams % 1000UL);
        String result(whole);
        if (fraction == 0U) return result;

        result += '.';
        if (fraction < 100U) result += '0';
        if (fraction < 10U) result += '0';
        result += fraction;
        while (result.endsWith("0")) result.remove(result.length() - 1U);
        return result;
    }
};
}

#endif
