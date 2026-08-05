#include "CM_UiModel.h"

namespace CM
{

UiModel::UiModel()
    : screen(UiScreen::EnterCoilCount),
      inputValue(0U),
      inputDigits(0U),
      coilNumber(0U),
      coilCount(0U),
      currentTurns(0U),
      targetTurns(0U),
      completedRuns(0U),
      pendingSyncCount(0U),
      syncState(UiSyncState::Synchronized),
      windingType(WindingType::Working),
      jobSource(JobSource::LocalKeypad)
{
}

UiModel UiModelBuilder::build(const StateMachine& stateMachine,
                              const InputController& inputController)
{
    UiModel model;
    const WindingJob& job = stateMachine.job();

    model.inputValue = inputController.numberInput().value();
    model.inputDigits = inputController.numberInput().digitCount();
    model.coilCount = job.coilCount;
    model.currentTurns = job.currentTurns;
    model.targetTurns = job.activeTarget();
    model.completedRuns = job.completedRuns;
    model.windingType = job.type;
    model.jobSource = job.source;

    if (job.currentCoil < job.coilCount)
    {
        model.coilNumber = static_cast<uint8_t>(job.currentCoil + 1U);
    }
    else if (job.coilCount > 0U)
    {
        model.coilNumber = job.coilCount;
    }

    switch (stateMachine.state())
    {
        case MachineState::EnterCoilCount:
            model.screen = UiScreen::EnterCoilCount;
            break;

        case MachineState::EnterTurns:
            model.screen = UiScreen::EnterTurns;
            model.coilNumber = static_cast<uint8_t>(
                inputController.editingCoilIndex() + 1U);
            break;

        case MachineState::Ready:
            model.screen = UiScreen::Ready;
            break;

        case MachineState::Winding:
            model.screen = UiScreen::Winding;
            break;

        case MachineState::Paused:
            model.screen = UiScreen::Paused;
            break;

        case MachineState::ManualRun:
            model.screen = UiScreen::ManualRun;
            break;

        case MachineState::CoilComplete:
            model.screen = UiScreen::CoilComplete;
            break;

        case MachineState::JobComplete:
            model.screen = UiScreen::JobComplete;
            break;

        case MachineState::Fault:
        default:
            model.screen = UiScreen::Fault;
            break;
    }

    return model;
}

} // namespace CM
