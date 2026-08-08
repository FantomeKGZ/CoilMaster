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

bool directoryContentsCanonical(fs::FS& storage, const char* path)
{
    if (!storage.exists(path)) return true;
    File directory = storage.open(path, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return false;
    }

    File entry = directory.openNextFile();
    while (entry)
    {
        const String name = entry.name();
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return false;
        }
        uint32_t sessionId = 0UL;
        if (!parseSessionFileName(name, sessionId))
        {
            entry.close();
            directory.close();
            return false;
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return true;
}
}

bool WindingSessionPersistenceIntegrityAudit::check(fs::FS& storage)
{
    WindingSessionPersistenceAuditMetrics ignoredMetrics;
    return check(storage, ignoredMetrics);
}

bool WindingSessionPersistenceIntegrityAudit::check(
    fs::FS& storage,
    WindingSessionPersistenceAuditMetrics& metrics)
{
    metrics = WindingSessionPersistenceAuditMetrics();
    WindingSessionPersistenceAuditMetrics validatedMetrics;

    const bool hasSnapshots = storage.exists(SnapshotDirectory);
    const bool hasStates = storage.exists(StateDirectory);
    const bool hasSelections = storage.exists(SelectionDirectory);

    if (!hasSnapshots && !hasStates && !hasSelections)
    {
        metrics = validatedMetrics;
        return true;
    }

    // Complete the read-only directory audit before any store begin(). In
    // particular JobSpoolSelectionStore::begin() can recover a .tmp file, so a
    // non-canonical entry must fail here before that code is reached.
    if (!directoryContentsCanonical(storage, SnapshotDirectory) ||
        !directoryContentsCanonical(storage, StateDirectory) ||
        !directoryContentsCanonical(storage, SelectionDirectory))
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
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
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
            if (validatedMetrics.snapshotFileCount == 0xFFFFFFFFUL)
            {
                directory.close(); return false;
            }
            ++validatedMetrics.snapshotFileCount;
            // Snapshot-only sessions are valid legacy archive entries. Newer
            // state/spool files, when present, are cross-checked below.
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
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
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
            if (validatedMetrics.stateFileCount == 0xFFFFFFFFUL)
            {
                directory.close(); return false;
            }
            ++validatedMetrics.stateFileCount;
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
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
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
            if (validatedMetrics.spoolSelectionFileCount == 0xFFFFFFFFUL)
            {
                directory.close(); return false;
            }
            ++validatedMetrics.spoolSelectionFileCount;
            entry = directory.openNextFile();
        }
        directory.close();
    }

    metrics = validatedMetrics;
    return true;
}
}
