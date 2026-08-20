#include <stdint.h>
#include <stdio.h>

#include "CM_StateMachine.h"

namespace
{
int g_failures = 0;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        ++g_failures;
        printf("FAIL: %s\n", message);
    }
}

void completeSegment(CM::StateMachine& machine, uint16_t turns)
{
    for (uint16_t turn = 0U; turn < turns; ++turn)
    {
        check(machine.registerTurn(), "register segment turn");
    }
}

void testRepeatTargetKeepsOneRunPerProgramCycle()
{
    CM::StateMachine machine;
    machine.setNextIdentifiers(1000UL, 2000UL);

    CM::WindingJob remote;
    remote.clear();
    remote.jobId = 77UL;
    remote.sessionId = 555UL;
    remote.type = CM::WindingType::Working;
    remote.coilCount = 2U;
    remote.repeatTarget = 6U;
    remote.targetTurns[0] = 38U;
    remote.targetTurns[1] = 38U;

    check(machine.loadRemoteJob(remote), "load 38/38 x6 remote job");
    check(machine.state() == CM::MachineState::Ready,
          "remote job waits for first physical START");
    check(machine.job().completedRuns == 0U,
          "new remote job has zero completed runs");

    uint32_t previousRunId = 0UL;

    for (uint16_t repeat = 1U; repeat <= 6U; ++repeat)
    {
        check(machine.startOrResume(), "physical START begins repeat");
        check(machine.state() == CM::MachineState::Winding,
              "repeat enters winding state");

        const uint32_t runId = machine.job().currentRunId;
        check(runId != 0UL, "repeat has non-zero run id");
        check(runId != previousRunId, "new repeat receives new run id");
        if (previousRunId != 0UL)
        {
            check(runId == previousRunId + 1UL,
                  "run id increments once per repeat");
        }

        CM::WindingEvent event;
        check(machine.takeEvent(event), "RUN_STARTED event published");
        check(event.type == CM::WindingEventType::RunStarted,
              "first event is RUN_STARTED");
        check(event.runId == runId,
              "RUN_STARTED uses repeat run id");
        check(event.completedRuns == 0U,
              "RUN_STARTED does not claim completion");

        completeSegment(machine, 38U);
        check(machine.state() == CM::MachineState::CoilComplete,
              "first 38-turn segment completes inside same run");
        check(machine.job().currentRunId == runId,
              "first segment keeps current run id");
        check(!machine.takeEvent(event),
              "segment boundary does not publish run event");

        check(machine.startOrResume(),
              "physical continuation starts second program segment");
        check(machine.state() == CM::MachineState::Winding,
              "second segment resumes winding");
        check(machine.job().currentRunId == runId,
              "second segment keeps same run id");
        check(!machine.takeEvent(event),
              "second segment does not publish another RUN_STARTED");

        completeSegment(machine, 38U);
        check(machine.state() == CM::MachineState::JobComplete,
              "full 38/38 program cycle completes");
        check(machine.job().completedRuns == repeat,
              "completedRuns counts full program cycles only");
        check(machine.job().currentRunId == runId,
              "RUN_COMPLETED uses same run id as RUN_STARTED");

        check(machine.takeEvent(event), "RUN_COMPLETED event published");
        check(event.type == CM::WindingEventType::RunCompleted,
              "final event is RUN_COMPLETED");
        check(event.runId == runId,
              "RUN_COMPLETED preserves repeat run id");
        check(event.completedRuns == repeat,
              "RUN_COMPLETED reports cumulative repeat count");

        if (repeat < 6U)
        {
            check(!machine.acknowledgeDeliveredRun(runId),
                  "intermediate completion ACK cannot clear job");
            check(machine.state() == CM::MachineState::JobComplete,
                  "intermediate repeat waits for next physical START");
        }
        else
        {
            check(!machine.startOrResume(),
                  "repeat target blocks a seventh physical run");
            check(!machine.acknowledgeDeliveredRun(runId - 1UL),
                  "wrong final run ACK cannot clear job");
            check(machine.acknowledgeDeliveredRun(runId),
                  "exact final RUN_COMPLETED ACK clears remote job");
            check(machine.state() == CM::MachineState::EnterCoilCount,
                  "final ACK returns Arduino to home");
            check(!machine.job().isValid(),
                  "final ACK removes active winding job");
        }

        previousRunId = runId;
    }
}
}

int main()
{
    testRepeatTargetKeepsOneRunPerProgramCycle();

    if (g_failures == 0)
    {
        printf("Repeat target state-machine tests passed.\n");
        return 0;
    }

    printf("Repeat target state-machine tests failed: %d\n", g_failures);
    return 1;
}
