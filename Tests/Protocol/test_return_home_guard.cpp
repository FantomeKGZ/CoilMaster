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

CM::WindingJob makeRemote(uint32_t jobId,
                          uint32_t sessionId,
                          uint16_t turns,
                          uint16_t repeatTarget = 1U)
{
    CM::WindingJob job;
    job.clear();
    job.jobId = jobId;
    job.sessionId = sessionId;
    job.source = CM::JobSource::Esp32Web;
    job.type = CM::WindingType::Working;
    job.coilCount = 1U;
    job.repeatTarget = repeatTarget;
    job.targetTurns[0] = turns;
    return job;
}

void testRemoteReadyCannotBeSilentlyCleared()
{
    CM::StateMachine machine;
    const CM::WindingJob remote = makeRemote(10UL, 20UL, 5U);
    check(machine.loadRemoteJob(remote), "remote READY job accepted");
    check(!machine.returnHome(), "remote READY job rejects ordinary return-home");
    check(machine.state() == CM::MachineState::Ready,
          "remote READY state remains intact");
    check(machine.job().jobId == 10UL && machine.job().sessionId == 20UL,
          "remote READY identity remains intact");
}

void testActiveRunCannotBeSilentlyCleared()
{
    CM::StateMachine machine;
    check(machine.setCoilCount(2U), "local coil count accepted");
    check(machine.setCoilTurns(0U, 2U), "first local target accepted");
    check(machine.setCoilTurns(1U, 2U), "second local target accepted");
    check(machine.startOrResume(), "local run starts");

    CM::WindingEvent event;
    check(machine.takeEvent(event), "RUN_STARTED consumed");
    check(!machine.returnHome(), "WINDING rejects ordinary return-home");
    check(machine.state() == CM::MachineState::Winding,
          "WINDING state survives rejected return-home");

    check(machine.pause(), "run pauses");
    check(!machine.returnHome(), "PAUSED rejects ordinary return-home");
    check(machine.state() == CM::MachineState::Paused,
          "PAUSED state survives rejected return-home");

    check(machine.startOrResume(), "paused run resumes");
    check(machine.registerTurn(), "first turn accepted");
    check(machine.registerTurn(), "second turn reaches coil boundary");
    check(machine.state() == CM::MachineState::CoilComplete,
          "run reaches coil-complete boundary");
    check(!machine.returnHome(), "COIL_COMPLETE rejects ordinary return-home");
    check(machine.state() == CM::MachineState::CoilComplete,
          "COIL_COMPLETE state survives rejected return-home");
}

void testRemoteCompletionWaitsForExactAck()
{
    CM::StateMachine machine;
    const CM::WindingJob remote = makeRemote(30UL, 40UL, 1U);
    check(machine.loadRemoteJob(remote), "remote completion job accepted");
    check(machine.startOrResume(), "remote completion job starts");

    CM::WindingEvent event;
    check(machine.takeEvent(event), "remote RUN_STARTED consumed");
    const uint32_t runId = machine.job().currentRunId;
    check(machine.registerTurn(), "remote run completes");
    check(machine.state() == CM::MachineState::JobComplete,
          "remote run reaches JOB_COMPLETE");
    check(!machine.returnHome(),
          "remote JOB_COMPLETE rejects ordinary return-home before exact ACK");
    check(machine.job().currentRunId == runId,
          "remote completed run identity remains intact");
    check(machine.acknowledgeDeliveredRun(runId),
          "exact completion ACK still clears remote job");
    check(machine.state() == CM::MachineState::EnterCoilCount,
          "exact completion ACK returns home");
}

void testSafeLocalMenuTransitionsRemainAvailable()
{
    {
        CM::StateMachine machine;
        check(machine.setCoilCount(2U), "local editing started");
        check(machine.returnHome(), "local turn-entry can return home");
        check(machine.state() == CM::MachineState::EnterCoilCount,
              "local turn-entry returns home");
    }

    {
        CM::StateMachine machine;
        check(machine.setCoilCount(1U), "local single coil accepted");
        check(machine.setCoilTurns(0U, 1U), "local target accepted");
        check(machine.returnHome(), "never-started local READY job can return home");
        check(machine.state() == CM::MachineState::EnterCoilCount,
              "never-started local READY job returns home");
    }

    {
        CM::StateMachine machine;
        check(machine.setCoilCount(1U), "local completion coil accepted");
        check(machine.setCoilTurns(0U, 1U), "local completion target accepted");
        check(machine.startOrResume(), "local completion run starts");
        CM::WindingEvent event;
        check(machine.takeEvent(event), "local RUN_STARTED consumed");
        check(machine.registerTurn(), "local completion run completes");
        check(machine.takeEvent(event), "local RUN_COMPLETED consumed");
        check(machine.returnHome(), "completed local job can return to menu");
        check(machine.state() == CM::MachineState::EnterCoilCount,
              "completed local job returns home");
    }
}
}

int main()
{
    testRemoteReadyCannotBeSilentlyCleared();
    testActiveRunCannotBeSilentlyCleared();
    testRemoteCompletionWaitsForExactAck();
    testSafeLocalMenuTransitionsRemainAvailable();

    if (g_failures == 0)
    {
        printf("Return-home state-machine guard tests passed.\n");
        return 0;
    }

    printf("Return-home state-machine guard tests failed: %d\n", g_failures);
    return 1;
}
