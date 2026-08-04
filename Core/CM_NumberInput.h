#ifndef CM_NUMBER_INPUT_H
#define CM_NUMBER_INPUT_H

#include <stdint.h>

namespace CM
{

/**
 * @brief Fixed-size numeric input buffer for Arduino Uno.
 *
 * The class does not use String or dynamic allocation. Digits are accumulated
 * directly into an unsigned 16-bit value with overflow and range checks.
 */
class NumberInput
{
public:
    static constexpr uint8_t MaxDigits = 5U;

    NumberInput();

    /** Reset the current value and digit count. */
    void clear();

    /**
     * Append one decimal digit.
     * @return true when the digit was accepted.
     */
    bool appendDigit(uint8_t digit);

    /** Remove the last entered digit. */
    bool backspace();

    /** Return true when at least one digit has been entered. */
    bool hasValue() const;

    /** Current numeric value. */
    uint16_t value() const;

    /** Number of entered digits, including leading zeroes. */
    uint8_t digitCount() const;

    /** Check that the current value exists and is inside the inclusive range. */
    bool isInRange(uint16_t minimum, uint16_t maximum) const;

private:
    uint16_t m_value;
    uint8_t m_digitCount;
};

} // namespace CM

#endif // CM_NUMBER_INPUT_H
