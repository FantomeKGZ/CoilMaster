#ifndef CM_UI_MODEL_H
#define CM_UI_MODEL_H

#include <stdint.h>

#include "CM_InputController.h"
#include "CM_StateMachine.h"

namespace CM
{

enum class UiScreen : uint8_t
{
    EnterCoilCount = 0U,
    EnterTurns,
    Ready,
    Winding,
    Paused,
    ManualRun,
    CoilComplete,
    JobComplete,
    Fault
};

/**
 * @brief Hardware-independent data required to render the 16x2 LCD.
 *
 * Text and LCD character encoding are intentionally kept outside Core.
 * The Arduino LCD adapter converts this model into the required 16x2 lines.
 */
struct UiModel
{
    UiScreen screen;
    uint16_t inputValue;
    uint8_t inputDigits;
    uint8_t coilNumber;
    uint8_t coilCount;
    uint16_t currentTurns;
    uint16_t targetTurns;
    WindingType windingType;
    JobSource jobSource;

    UiModel();
};

class UiModelBuilder
{
public:
    static UiModel build(const StateMachine& stateMachine,
                         const InputController& inputController);
};

} // namespace CM

#endif // CM_UI_MODEL_H
