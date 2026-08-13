#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

#include "CM_AutonomousWindingArchive.h"
#include "CM_AutonomousWindingWeb.h"
#include "CM_BackupActivityGuard.h"
#include "CM_JobDisplayRecovery.h"
#include "CM_JobLinkageRequest.h"
#include "CM_JobLinkageResolver.h"
#include "CM_JobRecovery.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_JobSpoolSelectionWeb.h"
#include "CM_JobStateStore.h"
#include "CM_MotorSimilarityWeb.h"
#include "CM_NetworkManager.h"
#include "CM_NetworkProfileStore.h"
#include "CM_NetworkWeb.h"
#include "CM_PersistentIdAllocator.h"
#include "CM_RepairRegistry.h"
#include "CM_RepairRegistryWeb.h"
#include "CM_RemoteBackupSettings.h"
#include "CM_RemoteBackupWeb.h"
#include "CM_StaticSiteServer.h"
#include "CM_UartEventReceiver.h"
#include "CM_WebRecoveryFtpServer.h"
#include "CM_WarehouseStore.h"
#include "CM_WarehouseWeb.h"
#include "CM_WindingJournal.h"
#include "CM_WindingProgramParser.h"

namespace
{
constexpr int8_t ArduinoRxPin = 16;
constexpr int8_t ArduinoTxPin = 17;
constexpr uint32_t ArduinoBaud = 9600UL;
constexpr int8_t SdCsPin = 5;
constexpr int8_t SdSckPin = 18;
constexpr int8_t SdMisoPin = 19;
constexpr int8_t SdMosiPin = 23;
constexpr char AccessPointName[] = "CoilMaster";
constexpr char AccessPointPassword[] = "CoilMaster123";
constexpr uint8_t MaxWebCoils = 10U;
constexpr uint16_t MaxTurnsPerCoil = 9999U;

HardwareSerial arduinoSerial(2);
CM::UartEventReceiver receiver(arduinoSerial);
CM::WindingJournal journal(SD);
CM::PersistentIdAllocator idAllocator(SD);
CM::JobSnapshotStore jobSnapshots(SD);
CM::JobSpoolSelectionStore jobSpoolSelections(SD);
CM::JobStateStore jobStates(SD);
CM::JobLinkageResolver jobLinkageResolver(SD);
CM::WarehouseStore warehouse(SD);
CM::RepairRegistry repairRegistry(SD);
CM::AutonomousWindingArchive autonomousWindingArchive(SD);
CM::RemoteBackupSettingsStore remoteBackupSettings(SD);
CM::NetworkProfileStore networkProfiles(SD);
CM::NetworkManager networkManager(networkProfiles);
WebServer webServer(80);
CM::WebRecoveryFtpServer webRecoveryFtp(webServer, SD);
CM::RemoteBackupWeb remoteBackupWeb(webServer, SD, remoteBackupSettings);
CM::NetworkWeb networkWeb(webServer, networkProfiles, networkManager);
CM::JobSpoolSelectionWeb jobSpoolSelectionWeb(webServer, jobSpoolSelections);
CM::StaticSiteServer staticSites(webServer, SD, networkManager, webRecoveryFtp);
CM::WarehouseWeb warehouseWeb(webServer, warehouse);
CM::RepairRegistryWeb repairRegistryWeb(webServer, repairRegistry);
CM::MotorSimilarityWeb motorSimilarityWeb(webServer, repairRegistry);
CM::AutonomousWindingWeb autonomousWindingWeb(webServer,
                                               autonomousWindingArchive,
                                               repairRegistry);

uint32_t activeJobId = 0UL;
uint32_t activeSessionId = 0UL;
uint32_t lastRunId = 0UL;
uint32_t lastArduinoEventMs = 0UL;
uint16_t completedRuns = 0U;
uint8_t activeCoilCount = 0U;
uint16_t activeTurns[MaxWebCoils] = {};
CM::RemoteJobType activeJobType = CM::RemoteJobType::Working;
CM::JobLinkage activeJobLinkage;
CM::JobSpoolSelection activeJobSpoolSelection;
CM::JobDeliveryResult lastJobResult = CM::JobDeliveryResult::None;
CM::JobRecoveryInfo recoveryInfo;
bool recoveryEvaluated = false;
bool stateRecovered = false;
bool jobAwaitingAck = false;
bool jobCancelAwaitingAck = false;
bool runActive = false;
bool autonomousRunActive = false;
bool journalReady = false;
bool idAllocatorReady = false;
bool jobSnapshotStoreReady = false;
bool jobSpoolSelectionStoreReady = false;
bool jobStateStoreReady = false;
bool jobLinkageResolverReady = false;
bool warehouseReady = false;
bool repairRegistryReady = false;
bool autonomousWindingArchiveReady = false;
bool remoteBackupSettingsReady = false;
bool networkProfilesReady = false;
bool networkManagerReady = false;
bool webRecoveryRequired = false;

const char FallbackPage[] PROGMEM = R"HTML(
<!doctype html><html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CoilMaster</title><style>
body{font-family:Arial,sans-serif;max-width:680px;margin:40px auto;padding:0 18px;background:#eef2f6;color:#17212b}
main{background:#fff;border-radius:16px;padding:24px;box-shadow:0 3px 16px #17212b18}
a{display:block;margin-top:12px;padding:13px;border-radius:10px;background:#1769aa;color:#fff;text-decoration:none;text-align:center}
.warn{padding:12px;border-radius:10px;background:#fff1d8;color:#8a5300}
</style></head><body><main><h1>CoilMaster</h1>
<p class="warn">Веб-файлы на microSD не найдены. Подключитесь к Wi-Fi <b>CoilMaster</b> и загрузите содержимое папки firmware/esp32/web через FTP в корень.</p>
<p><b>FTP:</b> 192.168.4.1:21<br><b>Логин:</b> CoilMaster<br><b>Пароль:</b> CoilMaster123</p>
<p>FTP видит только папку <code>/web</code>. После полной загрузки перезагрузите CoilMaster.</p>
<a href="/api/status">Проверить API состояния</a></main></body></html>
)HTML";

bool manualReviewRequired()
{
    return recoveryEvaluated &&
           recoveryInfo.disposition == CM::JobRecoveryDisposition::ManualReviewRequired;
}

CM::BackupActivityCheck backupRuntimeActivity()
{
    if (!recoveryEvaluated || !jobStateStoreReady || !jobStates.isReady())
        return CM::BackupActivityCheck::Unavailable;
    // Manual review means ESP32 cannot prove that Arduino is physically idle
    // after a reboot/fault/delivery interruption. Keep heavy backup scans blocked
    // until the operator explicitly closes the recovery state.
    if (manualReviewRequired())
        return CM::BackupActivityCheck::Busy;
    if (runActive || autonomousRunActive || jobAwaitingAck || jobCancelAwaitingAck ||
        (lastJobResult == CM::JobDeliveryResult::Accepted && completedRuns == 0U))
    {
        return CM::BackupActivityCheck::Busy;
    }
    return CM::BackupActivityCheck::Safe;
}

bool jobCreationReady()
{
    return recoveryEvaluated &&
           recoveryInfo.mayCreateNewJob &&
           !manualReviewRequired() &&
           !autonomousRunActive &&
           !jobCancelAwaitingAck &&
           journalReady && journal.isReady() &&
           idAllocatorReady && idAllocator.isReady() &&
           jobSnapshotStoreReady && jobSnapshots.isReady() &&
           jobStateStoreReady && jobStates.isReady();
}

bool linkedJobCreationReady()
{
    return jobCreationReady() &&
           jobSpoolSelectionStoreReady && jobSpoolSelections.isReady() &&
           jobLinkageResolverReady && jobLinkageResolver.isReady() &&
           repairRegistryReady && repairRegistry.ready() &&
           warehouseReady && warehouse.ready();
}

const char* jobStatusText()
{
    if (manualReviewRequired()) return "MANUAL_REVIEW_REQUIRED";
    if (autonomousRunActive) return "ARDUINO_LOCAL_RUNNING";
    if (runActive) return "RUNNING";
    if (jobCancelAwaitingAck) return "CANCELLING";
    if (jobAwaitingAck) return "WAITING_ARDUINO_ACK";
    if (completedRuns > 0U) return "PROGRAM_COMPLETED";
    switch (lastJobResult)
    {
        case CM::JobDeliveryResult::Accepted: return "ACCEPTED_READY";
        case CM::JobDeliveryResult::Rejected: return "REJECTED";
        case CM::JobDeliveryResult::TimedOut: return "TIMED_OUT";
        case CM::JobDeliveryResult::Cancelled: return "CANCELLED";
        case CM::JobDeliveryResult::None:
        default: return "IDLE";
    }
}

const char* machineStatusText()
{
    if (manualReviewRequired()) return "Требуется проверка";
    if (autonomousRunActive) return "Автономная намотка";
    if (runActive) return "Намотка";
    if (jobCancelAwaitingAck) return "Отмена";
    if (jobAwaitingAck) return "Передача";
    if (completedRuns > 0U) return "Завершено";
    if (lastJobResult == CM::JobDeliveryResult::Accepted) return "Готов";
    return "Ожидание";
}

String activeProgramText()
{
    String result;
    for (uint8_t i = 0U; i < activeCoilCount; ++i)
    {
        if (i > 0U) result += '/';
        result += activeTurns[i];
    }
    return result;
}

CM::JobDeliveryResult deliveryResultFor(CM::JobDeliveryState state)
{
    switch (state)
    {
        case CM::JobDeliveryState::Accepted: return CM::JobDeliveryResult::Accepted;
        case CM::JobDeliveryState::Rejected: return CM::JobDeliveryResult::Rejected;
        case CM::JobDeliveryState::TimedOut: return CM::JobDeliveryResult::TimedOut;
        case CM::JobDeliveryState::Cancelled: return CM::JobDeliveryResult::Cancelled;
        case CM::JobDeliveryState::Created:
        case CM::JobDeliveryState::Delivering:
        default: return CM::JobDeliveryResult::None;
    }
}

void restoreLatestJobState()
{
    recoveryEvaluated = true;
    recoveryInfo = CM::JobRecoveryInfo();
    stateRecovered = false;
    activeJobId = 0UL;
    activeSessionId = 0UL;
    lastRunId = 0UL;
    completedRuns = 0U;
    activeJobType = CM::RemoteJobType::Working;
    activeJobLinkage = CM::JobLinkage();
    activeJobSpoolSelection = CM::JobSpoolSelection();
    activeCoilCount = 0U;
    lastJobResult = CM::JobDeliveryResult::None;
    runActive = false;
    jobAwaitingAck = false;
    jobCancelAwaitingAck = false;
    for (uint8_t i = 0U; i < MaxWebCoils; ++i) activeTurns[i] = 0U;

    if (!jobStateStoreReady ||
        !jobSnapshotStoreReady ||
        !CM::JobRecovery::evaluate(jobStates, jobSnapshots, recoveryInfo))
    {
        jobStateStoreReady = false;
        recoveryInfo.mayCreateNewJob = false;
        Serial.println(F("ERROR: persisted job recovery or snapshot identity validation failed; job creation blocked"));
        return;
    }

    if (recoveryInfo.disposition == CM::JobRecoveryDisposition::None)
    {
        Serial.println(F("No active persisted winding job state found"));
        return;
    }

    CM::RecoveredJobDisplay display;
    if (!CM::JobDisplayRecovery::load(jobSnapshots,
                                      recoveryInfo.state.jobId,
                                      recoveryInfo.state.sessionId,
                                      display))
    {
        jobStateStoreReady = false;
        recoveryInfo.mayCreateNewJob = false;
        Serial.println(F("ERROR: immutable job display recovery failed; job creation blocked"));
        return;
    }

    stateRecovered = true;
    activeJobId = recoveryInfo.state.jobId;
    activeSessionId = recoveryInfo.state.sessionId;
    activeJobType = display.type;
    activeJobLinkage = display.linkage;
    activeCoilCount = display.coilCount;
    for (uint8_t i = 0U; i < activeCoilCount; ++i)
        activeTurns[i] = display.turns[i];
    lastRunId = recoveryInfo.state.lastRunId;
    completedRuns = recoveryInfo.state.completedRuns;
    lastJobResult = deliveryResultFor(recoveryInfo.state.deliveryState);

    if (activeJobLinkage.linked)
    {
        if (!jobSpoolSelectionStoreReady || !jobSpoolSelections.isReady())
        {
            recoveryInfo.mayCreateNewJob = false;
            Serial.println(F("ERROR: linked job spool selection store unavailable; job creation blocked"));
            return;
        }

        CM::JobSpoolSelection recoveredSelection;
        if (!jobSpoolSelections.load(activeSessionId, recoveredSelection) ||
            !recoveredSelection.isValid() ||
            recoveredSelection.jobId != activeJobId ||
            recoveredSelection.sessionId != activeSessionId ||
            recoveredSelection.repairId != activeJobLinkage.repairId ||
            recoveredSelection.motorId != activeJobLinkage.motorId)
        {
            jobSpoolSelectionStoreReady = false;
            recoveryInfo.mayCreateNewJob = false;
            Serial.println(F("ERROR: immutable linked spool selection recovery failed; job creation blocked"));
            return;
        }
        activeJobSpoolSelection = recoveredSelection;
    }

    runActive = false;
    jobAwaitingAck = false;
    jobCancelAwaitingAck = false;

    Serial.print(F("Recovered job state job=")); Serial.print(activeJobId);
    Serial.print(F(" session=")); Serial.print(activeSessionId);
    Serial.print(F(" program=")); Serial.print(activeProgramText());
    if (activeJobLinkage.linked)
    {
        Serial.print(F(" repair=")); Serial.print(activeJobLinkage.repairId);
        Serial.print(F(" motor=")); Serial.print(activeJobLinkage.motorId);
        Serial.print(F(" spool=")); Serial.print(activeJobSpoolSelection.spoolId);
    }
    Serial.print(F(" run=")); Serial.print(lastRunId);
    Serial.print(F(" completed=")); Serial.println(completedRuns);
    if (manualReviewRequired())
        Serial.println(F("WARNING: manual review required; new job creation blocked"));
}

void printEvent(const CM::RemoteWindingEvent& event)
{
    Serial.print(F("CMP RX source="));
    Serial.print(event.localStandalone ? F("ARDUINO_LOCAL") : F("ESP32_JOB"));
    Serial.print(F(" type="));
    Serial.print(event.type == CM::RemoteEventType::RunStarted ? F("RUN_STARTED") : F("RUN_COMPLETED"));
    Serial.print(F(" session=")); Serial.print(event.sessionId);
    Serial.print(F(" run=")); Serial.print(event.runId);
    Serial.print(F(" completed=")); Serial.println(event.completedRuns);
}

bool persistEventState(const CM::RemoteWindingEvent& event)
{
    if (!jobStateStoreReady || !jobStates.isReady()) return false;
    CM::JobRuntimeState state;
    if (!jobStates.load(event.sessionId, state)) return false;

    if (event.type == CM::RemoteEventType::RunStarted)
    {
        if (state.executionState == CM::JobExecutionState::Running &&
            state.lastRunId == event.runId &&
            state.completedRuns == event.completedRuns)
            return true;
        return jobStates.updateExecution(event.sessionId,
                                         CM::JobExecutionState::Running,
                                         event.runId,
                                         event.completedRuns,
                                         millis());
    }

    if (event.type == CM::RemoteEventType::RunCompleted)
    {
        if (state.executionState == CM::JobExecutionState::ProgramCompleted &&
            state.lastRunId == event.runId &&
            state.completedRuns == event.completedRuns)
            return true;
        return jobStates.updateExecution(event.sessionId,
                                         CM::JobExecutionState::ProgramCompleted,
                                         event.runId,
                                         event.completedRuns,
                                         millis());
    }
    return false;
}

void handleAutonomousEvent(const CM::RemoteWindingEvent& event)
{
    const CM::AutonomousWindingSaveResult result = autonomousWindingArchive.save(event);
    if (result == CM::AutonomousWindingSaveResult::Saved ||
        result == CM::AutonomousWindingSaveResult::Duplicate)
    {
        autonomousRunActive = event.type == CM::RemoteEventType::RunStarted;
        receiver.sendAck(event.runId,
                         result == CM::AutonomousWindingSaveResult::Duplicate
                             ? "DUPLICATE"
                             : (event.type == CM::RemoteEventType::RunCompleted
                                    ? "SAVED" : "RECORDED"));
        return;
    }

    switch (result)
    {
        case CM::AutonomousWindingSaveResult::StorageUnavailable:
            autonomousWindingArchiveReady = false;
            receiver.sendNack(event.runId, "LOCAL_ARCHIVE_UNAVAILABLE");
            break;
        case CM::AutonomousWindingSaveResult::Invalid:
            receiver.sendNack(event.runId, "INVALID_LOCAL_EVENT");
            break;
        case CM::AutonomousWindingSaveResult::WriteFailed:
        default:
            autonomousWindingArchiveReady = false;
            receiver.sendNack(event.runId, "LOCAL_ARCHIVE_WRITE_FAILED");
            break;
    }
}

void handleEvent(const CM::RemoteWindingEvent& event)
{
    printEvent(event);
    lastArduinoEventMs = millis();

    if (event.localStandalone)
    {
        handleAutonomousEvent(event);
        return;
    }

    const CM::JournalSaveResult result = journal.save(event);
    if (result == CM::JournalSaveResult::Saved ||
        result == CM::JournalSaveResult::Duplicate)
    {
        if (!persistEventState(event))
        {
            jobStateStoreReady = false;
            receiver.sendNack(event.runId, "STATE_WRITE_FAILED");
            return;
        }
        activeSessionId = event.sessionId;
        lastRunId = event.runId;
        completedRuns = event.completedRuns;
        runActive = event.type == CM::RemoteEventType::RunStarted;
        if (runActive) jobCancelAwaitingAck = false;
        recoveryInfo = CM::JobRecoveryInfo();
        recoveryInfo.mayCreateNewJob = event.type == CM::RemoteEventType::RunCompleted;
        recoveryEvaluated = true;
        stateRecovered = false;
        receiver.sendAck(event.runId,
                         result == CM::JournalSaveResult::Duplicate
                             ? "DUPLICATE"
                             : (event.type == CM::RemoteEventType::RunCompleted
                                    ? "SAVED" : "RECORDED"));
        return;
    }

    switch (result)
    {
        case CM::JournalSaveResult::StorageUnavailable:
            receiver.sendNack(event.runId, "STORAGE_UNAVAILABLE");
            break;
        case CM::JournalSaveResult::InvalidTransition:
            receiver.sendNack(event.runId, "INVALID_TRANSITION");
            break;
        case CM::JournalSaveResult::WriteFailed:
        default:
            receiver.sendNack(event.runId, "WRITE_FAILED");
            break;
    }
}

bool parseTurns(const String& source, CM::OutgoingWindingJob& job)
{
    uint8_t count = 0U;
    if (!CM::WindingProgramParser::parse(source,
                                         job.turns,
                                         MaxWebCoils,
                                         count,
                                         MaxTurnsPerCoil))
    {
        job.coilCount = 0U;
        return false;
    }
    job.coilCount = count;
    return true;
}

bool parseCanonicalUint32(const String& source, uint32_t& value)
{
    value = 0UL;
    if (source.length() == 0U) return false;
    if (source.length() > 1U && source[0] == '0') return false;

    uint32_t parsed = 0UL;
    for (size_t index = 0U; index < source.length(); ++index)
    {
        const char ch = source[index];
        if (!isDigit(ch)) return false;
        const uint8_t digit = static_cast<uint8_t>(ch - '0');
        if (parsed > (0xFFFFFFFFUL - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    value = parsed;
    return true;
}

bool sameProgram(const CM::OutgoingWindingJob& left,
                 const CM::OutgoingWindingJob& right)
{
    if (left.coilCount != right.coilCount) return false;
    for (uint8_t index = 0U; index < left.coilCount; ++index)
    {
        if (left.turns[index] != right.turns[index]) return false;
    }
    return true;
}

void sendJsonStatus()
{
    const bool arduinoOnline = lastArduinoEventMs > 0UL &&
        static_cast<uint32_t>(millis() - lastArduinoEventMs) < 15000UL;
    String response = F("{\"job_id\":"); response += activeJobId;
    response += F(",\"session_id\":"); response += activeSessionId;
    response += F(",\"job_status\":\""); response += jobStatusText();
    response += F("\",\"machine_status\":\""); response += machineStatusText();
    response += F("\",\"job_type\":\"");
    response += activeJobType == CM::RemoteJobType::Starting ? F("STARTING") : F("WORKING");
    response += F("\",\"program\":\""); response += activeProgramText();
    response += F("\",\"linked\":"); response += activeJobLinkage.linked ? F("true") : F("false");
    response += F(",\"repair_id\":");
    if (activeJobLinkage.linked) response += activeJobLinkage.repairId;
    else response += F("null");
    response += F(",\"motor_id\":");
    if (activeJobLinkage.linked) response += activeJobLinkage.motorId;
    else response += F("null");
    response += F(",\"spool_id\":");
    if (activeJobSpoolSelection.isValid()) response += activeJobSpoolSelection.spoolId;
    else response += F("null");
    response += F(",\"spool_wire_type\":");
    if (activeJobSpoolSelection.isValid())
    {
        response += '"'; response += activeJobSpoolSelection.wireType; response += '"';
    }
    else response += F("null");
    response += F(",\"spool_diameter_hundredths_mm\":");
    if (activeJobSpoolSelection.isValid()) response += activeJobSpoolSelection.diameterHundredthsMm;
    else response += F("null");
    response += F(",\"spool_weight_at_selection_g\":");
    if (activeJobSpoolSelection.isValid()) response += activeJobSpoolSelection.weightAtSelectionGrams;
    else response += F("null");
    response += F(",\"completed_runs\":"); response += completedRuns;
    response += F(",\"last_run_id\":"); response += lastRunId;
    response += F(",\"run_active\":"); response += runActive ? F("true") : F("false");
    response += F(",\"autonomous_run_active\":"); response += autonomousRunActive ? F("true") : F("false");
    response += F(",\"arduino_ack_pending\":"); response += jobAwaitingAck ? F("true") : F("false");
    response += F(",\"arduino_cancel_pending\":"); response += jobCancelAwaitingAck ? F("true") : F("false");
    response += F(",\"arduino_online\":"); response += arduinoOnline ? F("true") : F("false");
    response += F(",\"state_recovered\":"); response += stateRecovered ? F("true") : F("false");
    response += F(",\"manual_review_required\":"); response += manualReviewRequired() ? F("true") : F("false");
    response += F(",\"new_job_allowed\":"); response += recoveryInfo.mayCreateNewJob ? F("true") : F("false");
    response += F(",\"job_creation_ready\":"); response += jobCreationReady() ? F("true") : F("false");
    response += F(",\"linked_job_creation_ready\":"); response += linkedJobCreationReady() ? F("true") : F("false");
    response += F(",\"automatic_queue_allowed\":false,\"automatic_resume_allowed\":false,\"automatic_wire_writeoff_allowed\":false");
    response += F(",\"storage_ready\":"); response += journalReady && journal.isReady() ? F("true") : F("false");
    response += F(",\"autonomous_winding_archive_ready\":"); response += autonomousWindingArchiveReady && autonomousWindingArchive.ready() ? F("true") : F("false");
    response += F(",\"id_allocator_ready\":"); response += idAllocatorReady && idAllocator.isReady() ? F("true") : F("false");
    response += F(",\"job_snapshot_store_ready\":"); response += jobSnapshotStoreReady && jobSnapshots.isReady() ? F("true") : F("false");
    response += F(",\"job_spool_selection_store_ready\":"); response += jobSpoolSelectionStoreReady && jobSpoolSelections.isReady() ? F("true") : F("false");
    response += F(",\"job_state_store_ready\":"); response += jobStateStoreReady && jobStates.isReady() ? F("true") : F("false");
    response += F(",\"job_linkage_resolver_ready\":"); response += jobLinkageResolverReady && jobLinkageResolver.isReady() ? F("true") : F("false");
    response += F(",\"repair_registry_ready\":"); response += repairRegistryReady && repairRegistry.ready() ? F("true") : F("false");
    response += F(",\"last_allocated_job_id\":"); response += idAllocator.lastJobId();
    response += F(",\"last_allocated_session_id\":"); response += idAllocator.lastSessionId();
    response += F(",\"warehouse_ready\":"); response += warehouseReady && warehouse.ready() ? F("true") : F("false");
    response += F(",\"web_storage_ready\":"); response += staticSites.storageReady() ? F("true") : F("false");
    response += F(",\"winding_history_ready\":"); response += staticSites.windingHistoryReady() ? F("true") : F("false");
    response += F("}");
    webServer.send(200, "application/json; charset=utf-8", response);
}

void handleRecoveryAcknowledge()
{
    if (!manualReviewRequired())
    {
        webServer.send(409, "application/json", "{\"error\":\"manual_review_not_required\"}");
        return;
    }
    if (!jobStateStoreReady || !jobStates.isReady() ||
        !jobSnapshotStoreReady || !jobSnapshots.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"job_recovery_storage_unavailable\"}");
        return;
    }
    if (!webServer.hasArg("session_id") || !webServer.hasArg("confirmed"))
    {
        webServer.send(400, "application/json", "{\"error\":\"session_id_and_confirmation_required\"}");
        return;
    }

    uint32_t sessionId = 0UL;
    if (!parseCanonicalUint32(webServer.arg("session_id"), sessionId) ||
        sessionId == 0UL || sessionId != activeSessionId ||
        sessionId != recoveryInfo.state.sessionId)
    {
        webServer.send(409, "application/json", "{\"error\":\"session_mismatch\"}");
        return;
    }
    if (webServer.arg("confirmed") != "true")
    {
        webServer.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }

    CM::JobRuntimeState latest;
    bool found = false;
    if (!jobStates.loadLatest(latest, found))
    {
        webServer.send(500, "application/json", "{\"error\":\"job_state_integrity_failed\"}");
        return;
    }
    if (!found || latest.jobId != activeJobId || latest.sessionId != sessionId ||
        latest.jobId != recoveryInfo.state.jobId)
    {
        webServer.send(409, "application/json", "{\"error\":\"recovery_state_changed\"}");
        return;
    }

    CM::JobSnapshot snapshot;
    if (!jobSnapshots.load(sessionId, snapshot) || snapshot.jobId != latest.jobId)
    {
        webServer.send(500, "application/json", "{\"error\":\"job_snapshot_identity_failed\"}");
        return;
    }

    if (snapshot.linkage.linked)
    {
        if (!jobSpoolSelectionStoreReady || !jobSpoolSelections.isReady())
        {
            webServer.send(503, "application/json", "{\"error\":\"job_spool_selection_store_unavailable\"}");
            return;
        }
        CM::JobSpoolSelection selection;
        if (!jobSpoolSelections.load(sessionId, selection) || !selection.isValid() ||
            selection.jobId != latest.jobId || selection.sessionId != sessionId ||
            selection.repairId != snapshot.linkage.repairId ||
            selection.motorId != snapshot.linkage.motorId)
        {
            webServer.send(500, "application/json", "{\"error\":\"job_spool_selection_identity_failed\"}");
            return;
        }
    }

    if (!jobStates.closeAfterManualReview(sessionId, millis()))
    {
        jobStateStoreReady = false;
        webServer.send(503, "application/json", "{\"error\":\"review_closure_persistence_failed\"}");
        return;
    }

    String response = F("{\"acknowledged\":true,\"session_id\":");
    response += sessionId;
    response += F(",\"state\":\"CLOSED_AFTER_REVIEW\",\"restarting\":true,");
    response += F("\"automatic_queue_allowed\":false,\"automatic_resume_allowed\":false}");
    webServer.send(200, "application/json; charset=utf-8", response);
    Serial.print(F("Operator review closed session=")); Serial.println(sessionId);
    delay(350);
    ESP.restart();
}

void handleCancelJob()
{
    if (!recoveryEvaluated)
    {
        webServer.send(503, "application/json", "{\"error\":\"job_recovery_not_evaluated\"}");
        return;
    }
    if (manualReviewRequired())
    {
        webServer.send(409, "application/json", "{\"error\":\"manual_review_required\"}");
        return;
    }
    if (!jobStateStoreReady || !jobStates.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"job_state_store_unavailable\"}");
        return;
    }
    if (jobCancelAwaitingAck || receiver.jobCancelPending())
    {
        webServer.send(409, "application/json", "{\"error\":\"job_cancel_already_pending\"}");
        return;
    }
    if (!webServer.hasArg("job_id") || !webServer.hasArg("session_id") ||
        !webServer.hasArg("confirmed"))
    {
        webServer.send(400, "application/json", "{\"error\":\"job_session_and_confirmation_required\"}");
        return;
    }
    if (webServer.arg("confirmed") != "true")
    {
        webServer.send(400, "application/json", "{\"error\":\"explicit_confirmation_required\"}");
        return;
    }

    uint32_t jobId = 0UL;
    uint32_t sessionId = 0UL;
    if (!parseCanonicalUint32(webServer.arg("job_id"), jobId) ||
        !parseCanonicalUint32(webServer.arg("session_id"), sessionId) ||
        jobId == 0UL || sessionId == 0UL)
    {
        webServer.send(400, "application/json", "{\"error\":\"invalid_job_or_session_id\"}");
        return;
    }
    if (jobId != activeJobId || sessionId != activeSessionId)
    {
        webServer.send(409, "application/json", "{\"error\":\"job_or_session_mismatch\"}");
        return;
    }
    if (activeJobLinkage.linked)
    {
        webServer.send(409, "application/json", "{\"error\":\"linked_job_cannot_be_cancelled_here\"}");
        return;
    }
    if (runActive || completedRuns != 0U || lastRunId != 0UL || jobAwaitingAck ||
        lastJobResult != CM::JobDeliveryResult::Accepted)
    {
        webServer.send(409, "application/json", "{\"error\":\"job_not_waiting_physical_start\"}");
        return;
    }

    CM::JobRuntimeState persisted;
    if (!jobStates.load(sessionId, persisted))
    {
        webServer.send(500, "application/json", "{\"error\":\"job_state_read_failed\"}");
        return;
    }
    if (persisted.jobId != jobId ||
        persisted.deliveryState != CM::JobDeliveryState::Accepted ||
        persisted.executionState != CM::JobExecutionState::WaitingPhysicalStart ||
        persisted.lastRunId != 0UL || persisted.completedRuns != 0U)
    {
        webServer.send(409, "application/json", "{\"error\":\"persisted_job_not_cancellable\"}");
        return;
    }

    if (!receiver.requestJobCancel(jobId))
    {
        webServer.send(409, "application/json", "{\"error\":\"uart_cancel_sender_busy\"}");
        return;
    }

    jobCancelAwaitingAck = true;
    recoveryInfo.mayCreateNewJob = false;
    String response = F("{\"cancel_requested\":true,\"job_id\":");
    response += jobId;
    response += F(",\"session_id\":");
    response += sessionId;
    response += F(",\"status\":\"CANCELLING\",\"automatic_resume_allowed\":false}");
    webServer.send(202, "application/json; charset=utf-8", response);
}

void handleCreateJob()
{
    if (!recoveryEvaluated)
    {
        webServer.send(503, "application/json", "{\"error\":\"job_recovery_not_evaluated\"}");
        return;
    }
    if (autonomousRunActive)
    {
        webServer.send(409, "application/json", "{\"error\":\"arduino_local_winding_active\"}");
        return;
    }
    if (jobCancelAwaitingAck)
    {
        webServer.send(409, "application/json", "{\"error\":\"job_cancel_pending\"}");
        return;
    }
    if (manualReviewRequired())
    {
        webServer.send(409, "application/json", "{\"error\":\"manual_review_required\"}");
        return;
    }
    if (!recoveryInfo.mayCreateNewJob)
    {
        webServer.send(409, "application/json", "{\"error\":\"current_job_not_complete\"}");
        return;
    }
    if (!webServer.hasArg("turns"))
    {
        webServer.send(400, "application/json", "{\"error\":\"turns_required\"}");
        return;
    }

    CM::OutgoingWindingJob job;
    job.type = webServer.arg("type") == "starting" ? CM::RemoteJobType::Starting : CM::RemoteJobType::Working;
    if (!parseTurns(webServer.arg("turns"), job))
    {
        webServer.send(400, "application/json", "{\"error\":\"invalid_turns\"}");
        return;
    }

    CM::JobLinkage linkage;
    CM::ActiveWireSpoolIdentity selectedSpool;
    bool selectedSpoolReady = false;
    const CM::JobLinkageRequestResult linkageResult = CM::JobLinkageRequest::parse(
        webServer.hasArg("repair_id"), webServer.arg("repair_id"),
        webServer.hasArg("motor_id"), webServer.arg("motor_id"), linkage);
    if (linkageResult == CM::JobLinkageRequestResult::Partial)
    {
        webServer.send(400, "application/json", "{\"error\":\"repair_id_and_motor_id_required_together\"}");
        return;
    }
    if (linkageResult == CM::JobLinkageRequestResult::Invalid)
    {
        webServer.send(400, "application/json", "{\"error\":\"invalid_repair_motor_linkage\"}");
        return;
    }
    if (linkageResult == CM::JobLinkageRequestResult::Linked)
    {
        if (!jobLinkageResolverReady || !jobLinkageResolver.isReady() ||
            !repairRegistryReady || !repairRegistry.ready())
        {
            webServer.send(503, "application/json", "{\"error\":\"repair_store_unavailable\"}");
            return;
        }
        if (!warehouseReady || !warehouse.ready())
        {
            webServer.send(503, "application/json", "{\"error\":\"warehouse_unavailable\"}");
            return;
        }
        if (!jobSpoolSelectionStoreReady || !jobSpoolSelections.isReady())
        {
            webServer.send(503, "application/json", "{\"error\":\"job_spool_selection_store_unavailable\"}");
            return;
        }
        uint32_t spoolId = 0UL;
        if (!webServer.hasArg("spool_id") ||
            !parseCanonicalUint32(webServer.arg("spool_id"), spoolId) || spoolId == 0UL)
        {
            webServer.send(400, "application/json", "{\"error\":\"linked_spool_id_required\"}");
            return;
        }
        bool spoolFound = false;
        if (!warehouse.loadActiveSpoolIdentity(spoolId, selectedSpool, spoolFound))
        {
            if (!warehouse.ready())
                webServer.send(503, "application/json", "{\"error\":\"warehouse_unavailable\"}");
            else
                webServer.send(500, "application/json", "{\"error\":\"spool_catalog_read_failed\"}");
            return;
        }
        if (!spoolFound || !selectedSpool.isValid())
        {
            webServer.send(409, "application/json", "{\"error\":\"selected_spool_not_active_or_material_unknown\"}");
            return;
        }
        selectedSpoolReady = true;

        CM::JobLinkage resolved;
        String catalogProgram;
        if (!jobLinkageResolver.resolveWithProgram(linkage.repairId,
                                                   linkage.motorId,
                                                   resolved,
                                                   catalogProgram))
        {
            webServer.send(409, "application/json", "{\"error\":\"repair_motor_program_not_found_or_ambiguous\"}");
            return;
        }

        CM::OutgoingWindingJob catalogJob;
        if (!parseTurns(catalogProgram, catalogJob))
        {
            webServer.send(409, "application/json", "{\"error\":\"invalid_motor_coil_program\"}");
            return;
        }
        if (!sameProgram(job, catalogJob))
        {
            webServer.send(409, "application/json", "{\"error\":\"turns_do_not_match_motor_program\"}");
            return;
        }
        linkage = resolved;
    }
    else if (webServer.hasArg("spool_id") && webServer.arg("spool_id").length() > 0U)
    {
        webServer.send(400, "application/json", "{\"error\":\"spool_id_requires_linked_job\"}");
        return;
    }

    if (!journalReady || !journal.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"winding_journal_unavailable\"}");
        return;
    }
    if (!idAllocatorReady || !idAllocator.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"id_allocator_unavailable\"}");
        return;
    }
    if (!jobSnapshotStoreReady || !jobSnapshots.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"job_snapshot_store_unavailable\"}");
        return;
    }
    if (!jobStateStoreReady || !jobStates.isReady())
    {
        webServer.send(503, "application/json", "{\"error\":\"job_state_store_unavailable\"}");
        return;
    }
    if (!idAllocator.allocate(job.jobId, job.sessionId))
    {
        idAllocatorReady = false;
        webServer.send(503, "application/json", "{\"error\":\"id_persistence_failed\"}");
        return;
    }

    const uint32_t createdMs = millis();
    if (!jobSnapshots.create(job, linkage, createdMs))
    {
        webServer.send(503, "application/json", "{\"error\":\"job_snapshot_persistence_failed\"}");
        return;
    }

    CM::JobSpoolSelection persistedSpoolSelection;
    if (linkage.linked)
    {
        if (!selectedSpoolReady)
        {
            webServer.send(500, "application/json", "{\"error\":\"linked_spool_selection_internal_error\"}");
            return;
        }
        persistedSpoolSelection.jobId = job.jobId;
        persistedSpoolSelection.sessionId = job.sessionId;
        persistedSpoolSelection.repairId = linkage.repairId;
        persistedSpoolSelection.motorId = linkage.motorId;
        persistedSpoolSelection.spoolId = selectedSpool.spoolId;
        persistedSpoolSelection.diameterHundredthsMm = selectedSpool.diameterHundredthsMm;
        persistedSpoolSelection.weightAtSelectionGrams = selectedSpool.currentWeightGrams;
        persistedSpoolSelection.wireType = selectedSpool.wireType;
        if (!jobSpoolSelections.create(persistedSpoolSelection))
        {
            jobSpoolSelectionStoreReady = false;
            webServer.send(503, "application/json", "{\"error\":\"job_spool_selection_persistence_failed\"}");
            return;
        }
    }

    if (!jobStates.create(job.jobId, job.sessionId, createdMs) ||
        !jobStates.updateDelivery(job.sessionId,
                                  CM::JobDeliveryState::Delivering,
                                  millis()))
    {
        jobStateStoreReady = false;
        webServer.send(503, "application/json", "{\"error\":\"job_state_persistence_failed\"}");
        return;
    }
    if (!receiver.queueJob(job))
    {
        jobStates.updateDelivery(job.sessionId,
                                 CM::JobDeliveryState::Cancelled,
                                 millis());
        webServer.send(409, "application/json", "{\"error\":\"sender_busy\"}");
        return;
    }

    activeJobId = job.jobId;
    activeSessionId = job.sessionId;
    activeJobType = job.type;
    activeJobLinkage = linkage;
    activeJobSpoolSelection = linkage.linked
        ? persistedSpoolSelection
        : CM::JobSpoolSelection();
    activeCoilCount = job.coilCount;
    for (uint8_t i = 0U; i < activeCoilCount; ++i) activeTurns[i] = job.turns[i];
    completedRuns = 0U;
    lastRunId = 0UL;
    runActive = false;
    lastJobResult = CM::JobDeliveryResult::None;
    jobAwaitingAck = true;
    jobCancelAwaitingAck = false;
    recoveryInfo = CM::JobRecoveryInfo();
    recoveryInfo.mayCreateNewJob = false;
    recoveryEvaluated = true;
    stateRecovered = false;

    String response = F("{\"accepted\":true,\"job_id\":"); response += job.jobId;
    response += F(",\"session_id\":"); response += job.sessionId;
    response += F(",\"linked\":"); response += linkage.linked ? F("true") : F("false");
    if (linkage.linked)
    {
        response += F(",\"repair_id\":"); response += linkage.repairId;
        response += F(",\"motor_id\":"); response += linkage.motorId;
        response += F(",\"spool_id\":"); response += persistedSpoolSelection.spoolId;
        response += F(",\"spool_wire_type\":\""); response += persistedSpoolSelection.wireType;
        response += F("\",\"spool_diameter_hundredths_mm\":"); response += persistedSpoolSelection.diameterHundredthsMm;
        response += F(",\"spool_weight_at_selection_g\":"); response += persistedSpoolSelection.weightAtSelectionGrams;
        response += F(",\"spool_selection_saved\":true");
    }
    else
    {
        response += F(",\"repair_id\":null,\"motor_id\":null,\"spool_id\":null,\"spool_wire_type\":null,\"spool_diameter_hundredths_mm\":null,\"spool_weight_at_selection_g\":null,\"spool_selection_saved\":false");
    }
    response += F(",\"snapshot_saved\":true,\"state_saved\":true,\"automatic_wire_writeoff_allowed\":false");
    response += F(",\"status\":\"WAITING_ARDUINO_ACK\"}");
    webServer.send(202, "application/json; charset=utf-8", response);
}

void configureWebServer()
{
    webServer.on("/api/status", HTTP_GET, sendJsonStatus);
    webServer.on("/api/jobs", HTTP_POST, handleCreateJob);
    webServer.on("/api/jobs/cancel", HTTP_POST, handleCancelJob);
    webServer.on("/api/recovery/acknowledge", HTTP_POST, handleRecoveryAcknowledge);
    jobSpoolSelectionWeb.begin();
    repairRegistryWeb.begin();
    motorSimilarityWeb.begin();
    autonomousWindingWeb.begin();
    remoteBackupWeb.begin();
    networkWeb.begin();
    webRecoveryFtp.begin(webRecoveryRequired);
    warehouseWeb.begin();
    warehouseWeb.beginSpoolList();
    staticSites.begin("/web");
    webServer.onNotFound([]()
    {
        if (staticSites.serveCurrentRequest()) return;
        if (webServer.uri() == "/" && !staticSites.storageReady())
        {
            webServer.send_P(503, "text/html; charset=utf-8", FallbackPage);
            return;
        }
        if (webServer.uri().startsWith("/api/"))
        {
            webServer.send(404, "application/json", "{\"error\":\"not_found\"}");
            return;
        }
        webServer.send(404, "text/plain; charset=utf-8", "Страница не найдена");
    });
    webServer.begin();
}

CM::JobDeliveryState deliveryStateFor(CM::JobDeliveryResult result)
{
    switch (result)
    {
        case CM::JobDeliveryResult::Accepted: return CM::JobDeliveryState::Accepted;
        case CM::JobDeliveryResult::Rejected: return CM::JobDeliveryState::Rejected;
        case CM::JobDeliveryResult::TimedOut: return CM::JobDeliveryState::TimedOut;
        case CM::JobDeliveryResult::Cancelled: return CM::JobDeliveryState::Cancelled;
        case CM::JobDeliveryResult::None:
        default: return CM::JobDeliveryState::Delivering;
    }
}

void processJobDelivery()
{
    CM::JobDeliveryEvent delivery;
    while (receiver.takeJobDelivery(delivery))
    {
        const uint32_t sessionId = activeSessionId;
        if (sessionId == 0UL || delivery.jobId != activeJobId ||
            delivery.result == CM::JobDeliveryResult::None ||
            !jobStateStoreReady ||
            !jobStates.updateDelivery(sessionId,
                                      deliveryStateFor(delivery.result),
                                      millis()))
        {
            jobStateStoreReady = false;
            jobAwaitingAck = false;
            lastJobResult = CM::JobDeliveryResult::Rejected;
            recoveryInfo.mayCreateNewJob = false;
            Serial.println(F("ERROR: job delivery state persistence failed"));
            continue;
        }

        jobAwaitingAck = false;

        if (delivery.result == CM::JobDeliveryResult::TimedOut)
        {
            // No ACK is not proof of rejection: Arduino may have accepted the
            // JOB while every acknowledgement was lost. Re-evaluate the just-
            // persisted state through the same recovery path used after reboot
            // so operator review is required immediately, without auto resend.
            restoreLatestJobState();
            if (!jobStateStoreReady || !manualReviewRequired())
            {
                recoveryInfo.mayCreateNewJob = false;
                Serial.println(F("ERROR: timed-out job recovery verification failed"));
                continue;
            }

            Serial.print(F("JOB_ACK id=")); Serial.print(delivery.jobId);
            Serial.println(F(" result=TIMED_OUT_MANUAL_REVIEW"));
            continue;
        }

        if (delivery.result == CM::JobDeliveryResult::Accepted ||
            delivery.result == CM::JobDeliveryResult::Rejected)
        {
            // Only a parsed JOB_ACK proves that Arduino was heard. Locally
            // generated timeout/cancel results must not make arduino_online true.
            lastArduinoEventMs = millis();
        }
        activeJobId = delivery.jobId;
        lastJobResult = delivery.result;
        recoveryInfo.mayCreateNewJob =
            delivery.result == CM::JobDeliveryResult::Rejected ||
            delivery.result == CM::JobDeliveryResult::Cancelled;
        Serial.print(F("JOB_ACK id=")); Serial.print(delivery.jobId);
        Serial.print(F(" result="));
        Serial.println(delivery.result == CM::JobDeliveryResult::Accepted
                           ? F("ACCEPTED_READY") : F("NOT_ACCEPTED"));
    }
}

void processJobCancel()
{
    CM::JobCancelEvent cancel;
    while (receiver.takeJobCancel(cancel))
    {
        jobCancelAwaitingAck = false;
        if (cancel.result != CM::JobCancelResult::TimedOut)
            lastArduinoEventMs = millis();

        if (cancel.jobId == 0UL || cancel.jobId != activeJobId)
        {
            Serial.println(F("WARNING: ignored mismatched JOB_CANCEL_ACK"));
            continue;
        }

        if (cancel.result == CM::JobCancelResult::Cancelled)
        {
            const uint32_t sessionId = activeSessionId;
            if (sessionId == 0UL || !jobStateStoreReady ||
                !jobStates.closeAfterRemoteCancel(sessionId, millis()))
            {
                jobStateStoreReady = false;
                recoveryInfo.mayCreateNewJob = false;
                Serial.println(F("ERROR: acknowledged job cancellation persistence failed"));
                continue;
            }

            restoreLatestJobState();
            if (!jobStateStoreReady || manualReviewRequired())
            {
                recoveryInfo.mayCreateNewJob = false;
                Serial.println(F("ERROR: cancelled job recovery verification failed"));
                continue;
            }

            Serial.print(F("JOB_CANCEL_ACK id="));
            Serial.print(cancel.jobId);
            Serial.println(F(" result=CANCELLED"));
            continue;
        }

        recoveryInfo.mayCreateNewJob = false;
        Serial.print(F("JOB_CANCEL_ACK id="));
        Serial.print(cancel.jobId);
        Serial.print(F(" result="));
        Serial.println(cancel.result == CM::JobCancelResult::TimedOut
                           ? F("TIMED_OUT") : F("REJECTED"));
    }
}
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);
    SPI.begin(SdSckPin, SdMisoPin, SdMosiPin, SdCsPin);
    const bool sdReady = SD.begin(SdCsPin, SPI);
    webRecoveryRequired = sdReady && !SD.exists("/web");
    journalReady = sdReady && journal.begin();
    idAllocatorReady = sdReady && idAllocator.begin();
    jobSnapshotStoreReady = sdReady && jobSnapshots.begin();
    jobSpoolSelectionStoreReady = sdReady && jobSpoolSelections.begin();
    jobStateStoreReady = sdReady && jobStates.begin();
    jobLinkageResolverReady = sdReady && jobLinkageResolver.begin();
    warehouseReady = sdReady && warehouse.begin();
    repairRegistryReady = sdReady && repairRegistry.begin();
    autonomousWindingArchiveReady = sdReady && autonomousWindingArchive.begin();
    remoteBackupSettingsReady = sdReady && remoteBackupSettings.begin();
    networkProfilesReady = sdReady && networkProfiles.begin();
    restoreLatestJobState();
    CM::BackupActivityGuard::setRuntimeProbe(backupRuntimeActivity);
    remoteBackupWeb.setActivityProbe(backupRuntimeActivity);
    webRecoveryFtp.setActivityProbe(backupRuntimeActivity);

    networkManagerReady =
        networkManager.begin(AccessPointName, AccessPointPassword);
    configureWebServer();
    Serial.println(F("CoilMaster ESP32 web portal ready"));
    Serial.println(journalReady ? F("microSD winding journal ready") : F("WARNING: microSD winding journal unavailable"));
    Serial.println(autonomousWindingArchiveReady ? F("autonomous Arduino winding archive ready") : F("WARNING: autonomous Arduino winding archive unavailable"));
    Serial.println(remoteBackupSettingsReady ? F("remote FTP backup settings ready") : F("WARNING: remote FTP backup settings unavailable"));
    Serial.println(networkProfilesReady ? F("Wi-Fi profile storage ready") : F("WARNING: Wi-Fi profile storage unavailable"));
    Serial.println(idAllocatorReady ? F("persistent job/session ID allocator ready") : F("WARNING: persistent ID allocator unavailable; job creation blocked"));
    Serial.println(jobSnapshotStoreReady ? F("immutable job snapshot store ready") : F("WARNING: job snapshot store unavailable; job creation blocked"));
    Serial.println(jobSpoolSelectionStoreReady ? F("immutable job spool selection store ready") : F("WARNING: job spool selection store unavailable; linked job creation blocked"));
    Serial.println(jobStateStoreReady ? F("persistent job runtime state store ready") : F("WARNING: job state store unavailable; job creation blocked"));
    Serial.println(jobLinkageResolverReady ? F("repair motor linkage resolver ready") : F("WARNING: repair motor linkage resolver unavailable; linked job creation blocked"));
    Serial.println(repairRegistryReady ? F("client motor repair registry ready") : F("WARNING: client motor repair registry unavailable; linked job creation blocked"));
    Serial.println(warehouseReady ? F("microSD warehouse store ready") : F("WARNING: microSD warehouse store unavailable"));
    Serial.println(staticSites.storageReady() ? F("microSD web root /web ready") : F("WARNING: microSD web root /web unavailable"));
    Serial.println(staticSites.windingHistoryReady() ? F("read-only winding history API ready") : F("WARNING: winding history API unavailable"));
    Serial.println(networkManagerReady ? F("Wi-Fi AP+STA manager ready") : F("WARNING: Wi-Fi manager unavailable"));
    Serial.println(webRecoveryFtp.running() ? F("restricted /web recovery FTP ready at 192.168.4.1:21") : F("recovery FTP stopped; operator start available"));
    Serial.print(F("Open http://")); Serial.println(WiFi.softAPIP());
}

void loop()
{
    const uint32_t nowMs = millis();
    networkManager.update(nowMs);
    webServer.handleClient();
    receiver.update(nowMs);
    CM::RemoteWindingEvent event;
    while (receiver.poll(event)) handleEvent(event);
    processJobDelivery();
    processJobCancel();
    remoteBackupWeb.update(nowMs);
    webRecoveryFtp.update(nowMs);
}
