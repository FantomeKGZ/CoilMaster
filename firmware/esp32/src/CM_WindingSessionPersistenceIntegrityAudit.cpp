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

bool isCanonicalTempName(const String& path)
{
    const String name = baseNameOf(path);
    if (!name.startsWith(F("session-")) || !name.endsWith(F(".tmp"))) return false;
    if (name.length() <= 12U) return false;
    const String idText = name.substring(8U, name.length() - 4U);
    uint32_t sessionId = 0UL;
    return parseCanonicalUint32(idText, sessionId) && sessionId != 0UL;
}

bool stateConsistentWithSnapshot(const JobRuntimeState& state,
                                 const JobSnapshot& snapshot)
{
    if (snapshot.repeatTarget == 0U || state.completedRuns > snapshot.repeatTarget)
        return false;

    if (state.executionState == JobExecutionState::ProgramCompleted)
        return state.completedRuns == snapshot.repeatTarget;

    if (state.executionState == JobExecutionState::WaitingPhysicalStart ||
        state.executionState == JobExecutionState::Running)
    {
        return state.completedRuns < snapshot.repeatTarget;
    }

    return true;
}

WindingSessionPersistenceAuditFailure directoryContentsCanonical(fs::FS& storage,
                                                                  const char* path)
{
    if (!storage.exists(path)) return WindingSessionPersistenceAuditFailure::None;
    File directory = storage.open(path, FILE_READ);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        return WindingSessionPersistenceAuditFailure::DirectoryUnavailable;
    }

    File entry = directory.openNextFile();
    while (entry)
    {
        const String name = entry.name();
        if (entry.isDirectory())
        {
            entry.close();
            directory.close();
            return WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry;
        }
        if (isCanonicalTempName(name))
        {
            entry.close();
            directory.close();
            return WindingSessionPersistenceAuditFailure::TemporaryFilePresent;
        }
        uint32_t sessionId = 0UL;
        if (!parseSessionFileName(name, sessionId))
        {
            entry.close();
            directory.close();
            return WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry;
        }
        entry.close();
        entry = directory.openNextFile();
    }
    directory.close();
    return WindingSessionPersistenceAuditFailure::None;
}

void addFileSizeMetric(File& entry,
                       uint32_t& totalBytes,
                       bool& byteTotalsAvailable)
{
    if (!byteTotalsAvailable) return;
    const size_t fileSize = entry.size();
    if (fileSize > 0xFFFFFFFFUL)
    {
        totalBytes = 0UL;
        byteTotalsAvailable = false;
        return;
    }
    const uint32_t sizeBytes = static_cast<uint32_t>(fileSize);
    if (totalBytes > 0xFFFFFFFFUL - sizeBytes)
    {
        totalBytes = 0UL;
        byteTotalsAvailable = false;
        return;
    }
    totalBytes += sizeBytes;
}

bool failAudit(WindingSessionPersistenceAuditMetrics& metrics,
               WindingSessionPersistenceAuditMetrics& validatedMetrics,
               WindingSessionPersistenceAuditFailure failure)
{
    validatedMetrics.failure = failure;
    metrics = validatedMetrics;
    return false;
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

    // Only JobSpoolSelectionStore::begin() may promote a recoverable temp file.
    // Snapshot/state begin() only ensure already-known directories and their
    // content passes below reject temp/non-canonical entries themselves. Keep
    // the read-only preflight solely on the selection directory so corrupt or
    // stale selection temp evidence cannot be mutated before the audit sees it,
    // while avoiding duplicate snapshot/state directory scans.
    const uint32_t preflightStartedAtMs = millis();
    if (hasSelections)
    {
        const WindingSessionPersistenceAuditFailure failure =
            directoryContentsCanonical(storage, SelectionDirectory);
        if (failure != WindingSessionPersistenceAuditFailure::None)
        {
            validatedMetrics.directoryPreflightDurationMs = millis() - preflightStartedAtMs;
            validatedMetrics.directoryPreflightMeasured = true;
            return failAudit(metrics, validatedMetrics, failure);
        }
    }
    validatedMetrics.directoryPreflightDurationMs = millis() - preflightStartedAtMs;
    validatedMetrics.directoryPreflightMeasured = true;

    if (!hasSnapshots && !hasStates && !hasSelections)
    {
        metrics = validatedMetrics;
        return true;
    }

    JobSnapshotStore snapshots(storage);
    JobStateStore states(storage);
    JobSpoolSelectionStore selections(storage);
    if ((hasSnapshots && !snapshots.begin()) ||
        (hasStates && !states.begin()) ||
        (hasSelections && !selections.begin()))
    {
        return failAudit(metrics, validatedMetrics,
                         WindingSessionPersistenceAuditFailure::ContentInvalid);
    }

    if (hasSnapshots)
    {
        File directory = storage.open(SnapshotDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return failAudit(metrics, validatedMetrics,
                             WindingSessionPersistenceAuditFailure::DirectoryUnavailable);
        }
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry);
            }
            addFileSizeMetric(entry,
                              validatedMetrics.snapshotTotalBytes,
                              validatedMetrics.byteTotalsAvailable);
            entry.close();

            JobSnapshot snapshot;
            if (!snapshots.load(sessionId, snapshot) ||
                snapshot.sessionId != sessionId || snapshot.jobId == 0UL ||
                !snapshot.linkage.isValid())
            {
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
            }
            if (validatedMetrics.snapshotFileCount == 0xFFFFFFFFUL)
            {
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
            }
            ++validatedMetrics.snapshotFileCount;
            entry = directory.openNextFile();
        }
        directory.close();
    }

    if (hasStates)
    {
        File directory = storage.open(StateDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return failAudit(metrics, validatedMetrics,
                             WindingSessionPersistenceAuditFailure::DirectoryUnavailable);
        }
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry);
            }
            addFileSizeMetric(entry,
                              validatedMetrics.stateTotalBytes,
                              validatedMetrics.byteTotalsAvailable);
            entry.close();

            JobRuntimeState state;
            JobSnapshot snapshot;
            if (!states.load(sessionId, state) || state.sessionId != sessionId || state.jobId == 0UL ||
                !hasSnapshots || !snapshots.load(sessionId, snapshot) ||
                snapshot.jobId != state.jobId || snapshot.sessionId != state.sessionId ||
                !stateConsistentWithSnapshot(state, snapshot))
            {
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
            }

            if (snapshot.linkage.linked && !JobStateStore::isLocalPreparation(state))
            {
                JobSpoolSelection selection;
                bool selectionFound = false;
                if (!hasSelections ||
                    !selections.load(sessionId, selection, selectionFound) ||
                    !selectionFound || !selection.isValid() ||
                    selection.sessionId != sessionId ||
                    selection.jobId != state.jobId ||
                    selection.repairId != snapshot.linkage.repairId ||
                    selection.motorId != snapshot.linkage.motorId)
                {
                    directory.close();
                    return failAudit(metrics, validatedMetrics,
                                     WindingSessionPersistenceAuditFailure::ContentInvalid);
                }
            }

            if (validatedMetrics.stateFileCount == 0xFFFFFFFFUL)
            {
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
            }
            ++validatedMetrics.stateFileCount;
            entry = directory.openNextFile();
        }
        directory.close();
    }

    if (hasSelections)
    {
        File directory = storage.open(SelectionDirectory, FILE_READ);
        if (!directory || !directory.isDirectory())
        {
            if (directory) directory.close();
            return failAudit(metrics, validatedMetrics,
                             WindingSessionPersistenceAuditFailure::DirectoryUnavailable);
        }
        File entry = directory.openNextFile();
        while (entry)
        {
            const String name = entry.name();
            uint32_t sessionId = 0UL;
            if (entry.isDirectory() || !parseSessionFileName(name, sessionId))
            {
                entry.close(); directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry);
            }
            addFileSizeMetric(entry,
                              validatedMetrics.spoolSelectionTotalBytes,
                              validatedMetrics.byteTotalsAvailable);
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
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
            }
            if (validatedMetrics.spoolSelectionFileCount == 0xFFFFFFFFUL)
            {
                directory.close();
                return failAudit(metrics, validatedMetrics,
                                 WindingSessionPersistenceAuditFailure::ContentInvalid);
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
