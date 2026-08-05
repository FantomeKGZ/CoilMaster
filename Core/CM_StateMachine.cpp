#include "CM_StateMachine.h"

namespace CM
{
StateMachine::StateMachine()
    : m_state(MachineState::EnterCoilCount),
      m_job(),
      m_pendingEvent(),
      m_hasPendingEvent(false),
      m_nextSessionId(1UL),
      m_nextRunId(1UL)
{
    m_job.clear();
}

void StateMachine::resetToHome()
{
    m_job.clear();
    m_job.status = JobStatus::Editing;
    m_state = MachineState::EnterCoilCount;
    m_hasPendingEvent = false;
}

MachineState StateMachine::state() const
{
    return m_state;
}

const WindingJob& StateMachine::job() const
{
    return m_job;
}

WindingJob& StateMachine::job()
{
    return m_job;
}

bool StateMachine::setCoilCount(uint8_t count)
{
    if (m_state != MachineState::EnterCoilCount ||
        count == 0U || count > MaxCoilsPerJob)
    {
        return false;
    }

    m_job.clear();
    m_job.status = JobStatus::Editing;
    m_job.coilCount = count;
    m_state = MachineState::EnterTurns;
    return true;
}

bool StateMachine::setCoilTurns(uint8_t index, uint16_t turns)
{
    if (m_state != MachineState::EnterTurns ||
        index >= m_job.coilCount ||
        turns == 0U || turns > MaxTurnsPerCoil)
    {
        return false;
    }

    m_job.targetTurns[index] = turns;

    for (uint8_t current = 0U; current < m_job.coilCount; ++current)
    {
        if (m_job.targetTurns[current] == 0U)
        {
            return true;
        }
    }

    m_job.sessionId = allocateSessionId();
    m_job.currentRunId = 0UL;
    m_job.currentCoil = 0U;
    m_job.currentTurns = 0U;
    m_job.completedRuns = 0U;
    m_job.status = JobStatus::Ready;
    m_state = MachineState::Ready;
    return true;
}

bool StateMachine::loadRemoteJob(const WindingJob& remoteJob)
{
    if (m_state == MachineState::Winding ||
        m_state == MachineState::Paused ||
        m_state == MachineState::ManualRun ||
        !remoteJob.isValid())
    {
        return false;
    }

    m_job = remoteJob;
    if (m_job.sessionId == 0UL)
    {
        m_job.sessionId = allocateSessionId();
    }

    m_job.source = JobSource::Esp32Web;
    m_job.status = JobStatus::Ready;
    m_job.currentRunId = 0UL;
    m_job.currentCoil = 0U;
    m_job.currentTurns = 0U;
    m_job.completedRuns = 0U;
    m_state = MachineState::Ready;
    return true;
}

bool StateMachine::startOrResume()
{
    if (m_state == MachineState::JobComplete)
    {
        if (!m_job.isValid())
        {
            return false;
        }

        m_job.currentCoil = 0U;
        m_job.currentTurns = 0U;
        return beginRun();
    }

    if (m_state == MachineState::CoilComplete)
    {
        if (!acknowledgeCoilComplete())
        {
            return false;
        }
    }

    if (m_state == MachineState::Ready)
    {
        return beginRun();
    }

    if (m_state == MachineState::Paused)
    {
        if (!m_job.hasMoreCoils())
        {
            return false;
        }

        m_job.status = JobStatus::Running;
        m_state = MachineState::Winding;
        return true;
    }

    return false;
}

bool StateMachine::pause()
{
    if (m_state != MachineState::Winding)
    {
        return false;
    }

    m_job.status = JobStatus::Paused;
    m_state = MachineState::Paused;
    return true;
}

bool StateMachine::toggleManual()
{
    if (m_state == MachineState::ManualRun)
    {
        m_state = m_job.isValid() ? MachineState::Ready
                                 : MachineState::EnterCoilCount;
        return true;
    }

    if (m_state == MachineState::Ready ||
        m_state == MachineState::EnterCoilCount ||
        m_state == MachineState::EnterTurns ||
        m_state == MachineState::JobComplete)
    {
        m_state = MachineState::ManualRun;
        return true;
    }

    return false;
}

bool StateMachine::registerTurn()
{
    if (m_state != MachineState::Winding || !m_job.hasMoreCoils())
    {
        return false;
    }

    if (m_job.currentTurns < m_job.activeTarget())
    {
        ++m_job.currentTurns;
    }

    if (m_job.currentTurns >= m_job.activeTarget())
    {
        finishActiveCoil();
    }

    return true;
}

bool StateMachine::acknowledgeCoilComplete()
{
    if (m_state != MachineState::CoilComplete)
    {
        return false;
    }

    ++m_job.currentCoil;
    m_job.currentTurns = 0U;

    if (m_job.currentCoil >= m_job.coilCount)
    {
        ++m_job.completedRuns;
        m_job.status = JobStatus::Completed;
        m_state = MachineState::JobComplete;
        publishEvent(WindingEventType::RunCompleted);
    }
    else
    {
        m_job.status = JobStatus::Ready;
        m_state = MachineState::Ready;
    }

    return true;
}

bool StateMachine::cancel()
{
    if (m_state == MachineState::Fault)
    {
        return false;
    }

    m_job.status = JobStatus::Cancelled;
    resetToHome();
    return true;
}

void StateMachine::setFault()
{
    m_job.status = JobStatus::Failed;
    m_state = MachineState::Fault;
}

bool StateMachine::takeEvent(WindingEvent& event)
{
    if (!m_hasPendingEvent)
    {
        return false;
    }

    event = m_pendingEvent;
    m_pendingEvent = WindingEvent();
    m_hasPendingEvent = false;
    return true;
}

bool StateMachine::beginRun()
{
    if (!m_job.hasMoreCoils())
    {
        return false;
    }

    m_job.currentRunId = allocateRunId();
    m_job.status = JobStatus::Running;
    m_state = MachineState::Winding;
    publishEvent(WindingEventType::RunStarted);
    return true;
}

void StateMachine::finishActiveCoil()
{
    const bool isLastCoil =
        static_cast<uint8_t>(m_job.currentCoil + 1U) >= m_job.coilCount;

    if (isLastCoil)
    {
        m_job.currentCoil = m_job.coilCount;
        m_job.currentTurns = 0U;
        ++m_job.completedRuns;
        m_job.status = JobStatus::Completed;
        m_state = MachineState::JobComplete;
        publishEvent(WindingEventType::RunCompleted);
        return;
    }

    m_job.status = JobStatus::Paused;
    m_state = MachineState::CoilComplete;
}

void StateMachine::publishEvent(WindingEventType type)
{
    m_pendingEvent.type = type;
    m_pendingEvent.sessionId = m_job.sessionId;
    m_pendingEvent.runId = m_job.currentRunId;
    m_pendingEvent.completedRuns = m_job.completedRuns;
    m_hasPendingEvent = true;
}

uint32_t StateMachine::allocateSessionId()
{
    const uint32_t result = m_nextSessionId;
    ++m_nextSessionId;
    if (m_nextSessionId == 0UL)
    {
        m_nextSessionId = 1UL;
    }
    return result;
}

uint32_t StateMachine::allocateRunId()
{
    const uint32_t result = m_nextRunId;
    ++m_nextRunId;
    if (m_nextRunId == 0UL)
    {
        m_nextRunId = 1UL;
    }
    return result;
}
}
