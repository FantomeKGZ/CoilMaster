#include "CM_WindingJournal.h"

namespace CM
{
bool WindingJournal::loadSessionState(uint32_t sessionId,
                                      WindingSessionState& state) const
{
    state = WindingSessionState();
    state.sessionId = sessionId;

    if (!m_ready || sessionId == 0UL)
    {
        return false;
    }

    if (!loadSessionCompletedRuns(sessionId, state.completedRuns) ||
        !loadSessionHighestRunId(sessionId, state.highestRunId) ||
        !loadActiveRun(sessionId, state.activeRunId, state.activeRunFound))
    {
        return false;
    }

    if (!state.activeRunFound)
    {
        state.activeRunId = 0UL;
    }

    state.journalConsistent =
        (!state.activeRunFound || state.activeRunId > 0UL) &&
        state.completedRuns <= state.highestRunId;

    return state.journalConsistent;
}
}
