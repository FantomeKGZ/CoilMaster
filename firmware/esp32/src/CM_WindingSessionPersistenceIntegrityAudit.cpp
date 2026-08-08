#include "CM_WindingSessionPersistenceIntegrityAudit.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"
#include "CM_JobSpoolSelectionStore.h"
#include <Arduino.h>

namespace CM
{
namespace
{
constexpr const char* SnapshotDirectory = "/data/winding-jobs/snapshots";
constexpr const char* StateDirectory = "/data/winding-jobs/state";
constexpr const char* SelectionDirectory = "/data/winding-jobs/spool-selection";

String baseNameOf(const String& path)
{
    const int separator = path.lastIndexOf('/');
    return separator >= 0 ? path.substring(separator + 1) : path;
}

bool parseCanonicalUint32(const String& text, uint32_t& value)
{
    value = 0UL;
    if (text.length() == 0U || (text.length() > 1U && text[0] == '0')) return false;
    uint32_t parsed = 0UL;
    for (size_t i = 0U; i < text.length(); ++i)
    {
        const char ch = text[i];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }
    value = parsed;
    return true;
}

bool parseSessionFileName(const String& path, uint32_t& sessionId)
{
    const String name = baseNameOf(path);
    if (!name.startsWith(F("session-")) || !name.endsWith(F(".json"))) return false;
    if (name.length() <= 13U) return false;
    const String idText = name.substring(8U, name.length() - 5U);
    return parseCanonicalUint32(idText, sessionId) && sessionId != 0UL;
}

String sessionPath(const char* directory, uint32_t sessionId)
{
    String path(directory);
    path += F("/session-");
    path += sessionId;
    path += F(".json");
    return path;
}

bool directoryReady(fs::FS& storage, const char* path)
{
    if (!storage.exists(path)) return false;
    File directory = storage.open(path, FILE_READ);
    if (!directory) return false;
    const bool result = directory.isDirectory();
    directory.close();
    return result;
}
}

bool WindingSessionPersistenceIntegrityAudit::check(fs::FS& storage)
{
    const bool hasSnapshots = storage.exists(SnapshotDirectory);
    const bool hasStates = storage.exists(StateDirectory);
    const bool hasSelections = storage.exists(SelectionDirectory);

    if (!hasSnapshots && !hasStates && !hasSelections) return true;
    if ((hasSnapshots && !directoryReady(storage, SnapshotDirectory)) ||
        (hasStates && !directoryReady(storage, StateDirectory)) ||
        (hasSelections && !directoryReady(storage, SelectionDirectory)))
    {
        return false;
    }

    JobSnapshotStore snapshots(storage);
    JobStateStore states(storage);
    JobSpoolSelectionStore selections(storage);
    if ((hasSnapshots && !snapshots.begin()) ||
        (hasStates && !states.begin()) ||
        (hasSelections && !selections.begin()))
    {
        return false;
    }

    if (hasSnapshots)
    {
        File directory = storage.open(SnapshotDirectory, FILE_READ);
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            if (entry.isDirectory())
            {
                entry.close(); directory.close(); return false;
            }
            uint32_t sessionId = 0UL;
            if (!parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close(); return false;
            }
            entry.close();

            JobSnapshot snapshot;
            if (!snapshots.load(sessionId, snapshot) ||
                snapshot.sessionId != sessionId || snapshot.jobId == 0UL ||
                !snapshot.linkage.isValid())
            {
                directory.close(); return false;
            }
            if (!hasStates || !storage.exists(sessionPath(StateDirectory, sessionId)))
            {
                directory.close(); return false;
            }
            entry = directory.openNextFile();
        }
        directory.close();
    }

    if (hasStates)
    {
        File directory = storage.open(StateDirectory, FILE_READ);
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            if (entry.isDirectory())
            {
                entry.close(); directory.close(); return false;
            }
            uint32_t sessionId = 0UL;
            if (!parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close(); return false;
            }
            entry.close();

            JobRuntimeState state;
            JobSnapshot snapshot;
            if (!states.load(sessionId, state) || state.sessionId != sessionId || state.jobId == 0UL ||
                !hasSnapshots || !snapshots.load(sessionId, snapshot) ||
                snapshot.jobId != state.jobId || snapshot.sessionId != state.sessionId)
            {
                directory.close(); return false;
            }
            entry = directory.openNextFile();
        }
        directory.close();
    }

    if (hasSelections)
    {
        File directory = storage.open(SelectionDirectory, FILE_READ);
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            if (entry.isDirectory())
            {
                entry.close(); directory.close(); return false;
            }
            uint32_t sessionId = 0UL;
            if (!parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close(); return false;
            }
            entry.close();

            JobSpoolSelection selection;
            JobSnapshot snapshot;
            JobRuntimeState state;
            bool found = false;
            if (!selections.load(sessionId, selection, found) || !found || !selection.isValid() ||
                !hasSnapshots || !snapshots.load(sessionId, snapshot) ||
                !hasStates || !states.load(sessionId, state) ||
                selection.sessionId != sessionId || selection.jobId != snapshot.jobId ||
                selection.jobId != state.jobId || !snapshot.linkage.linked ||
                selection.repairId != snapshot.linkage.repairId ||
                selection.motorId != snapshot.linkage.motorId)
            {
                directory.close(); return false;
            }
            entry = directory.openNextFile();
        }
        directory.close();
    }

    return true;
}
}
