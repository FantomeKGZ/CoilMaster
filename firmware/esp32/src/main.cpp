#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

#include "CM_JobRecovery.h"
#include "CM_JobSnapshotStore.h"
#include "CM_JobStateStore.h"
#include "CM_PersistentIdAllocator.h"
#include "CM_StaticSiteServer.h"
#include "CM_UartEventReceiver.h"
#include "CM_WarehouseStore.h"
#include "CM_WarehouseWeb.h"
#include "CM_WindingJournal.h"

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
CM::JobStateStore jobStates(SD);
CM::WarehouseStore warehouse(SD);
WebServer webServer(80);
CM::StaticSiteServer staticSites(webServer, SD);
CM::WarehouseWeb warehouseWeb(webServer, warehouse);

uint32_t activeJobId = 0UL;
uint32_t activeSessionId = 0UL;
uint32_t lastRunId = 0UL;
uint32_t lastArduinoEventMs = 0UL;
uint16_t completedRuns = 0U;
uint8_t activeCoilCount = 0U;
uint16_t activeTurns[MaxWebCoils] = {};
CM::RemoteJobType activeJobType = CM::RemoteJobType::Working;
CM::JobDeliveryResult lastJobResult = CM::JobDeliveryResult::None;
CM::JobRecoveryInfo recoveryInfo;
bool recoveryEvaluated = false;
bool stateRecovered = false;
bool jobAwaitingAck = false;
bool runActive = false;
bool journalReady = false;
bool idAllocatorReady = false;
bool jobSnapshotStoreReady = false;
bool jobStateStoreReady = false;
bool warehouseReady = false;

const char FallbackPage[] PROGMEM = R"HTML(
<!doctype html><html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CoilMaster</title><style>
body{font-family:Arial,sans-serif;max-width:680px;margin:40px auto;padding:0 18px;background:#eef2f6;color:#17212b}
main{background:#fff;border-radius:16px;padding:24px;box-shadow:0 3px 16px #17212b18}
a{display:block;margin-top:12px;padding:13px;border-radius:10px;background:#1769aa;color:#fff;text-decoration:none;text-align:center}
.warn{padding:12px;border-radius:10px;background:#fff1d8;color:#8a5300}
</style></head><body><main><h1>CoilMaster</h1>
<p class="warn">Веб-файлы на microSD не найдены. Скопируйте содержимое папки firmware/esp32/web в папку /web на карте памяти.</p>
<a href="/api/status">Проверить API состояния</a></main></body></html>
)HTML";

bool manualReviewRequired()
{
    return recoveryEvaluated &&
           recoveryInfo.disposition == CM::JobRecoveryDisposition::ManualReviewRequired;
}

const char* jobStatusText()
{
    if (manualReviewRequired()) return "MANUAL_REVIEW_REQUIRED";
    if (jobAwaitingAck) return "WAITING_ARDUINO_ACK";
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
    if (runActive) return "Намотка";
    if (jobAwaitingAck) return "Передача";
    if (lastJobResult == CM::JobDeliveryResult::Accepted) return "Готов";
    if (completedRuns > 0U) return "Завершено";
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

    if (!jobStateStoreReady || !CM::JobRecovery::evaluate(jobStates, recoveryInfo))
    {
        jobStateStoreReady = false;
        recoveryInfo.mayCreateNewJob = false;
        Serial.println(F("ERROR: persisted job recovery failed; job creation blocked"));
        return;
    }

    if (recoveryInfo.disposition == CM::JobRecoveryDisposition::None)
    {
        Serial.println(F("No persisted winding job state found"));
        return;
    }

    stateRecovered = true;
    activeJobId = recoveryInfo.state.jobId;
    activeSessionId = recoveryInfo.state.sessionId;
    lastRunId = recoveryInfo.state.lastRunId;
    completedRuns = recoveryInfo.state.completedRuns;
    lastJobResult = deliveryResultFor(recoveryInfo.state.deliveryState);

    // Persisted RUNNING is historical after reboot, not proof of current motion.
    runActive = false;
    jobAwaitingAck = false;

    Serial.print(F("Recovered job state job=")); Serial.print(activeJobId);
    Serial.print(F(" session=")); Serial.print(activeSessionId);
    Serial.print(F(" run=")); Serial.print(lastRunId);
    Serial.print(F(" completed=")); Serial.println(completedRuns);
    if (manualReviewRequired())
        Serial.println(F("WARNING: manual review required; new job creation blocked"));
}

void printEvent(const CM::RemoteWindingEvent& event)
{
    Serial.print(F("CMP RX type="));
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

void handleEvent(const CM::RemoteWindingEvent& event)
{
    printEvent(event);
    lastArduinoEventMs = millis();
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
        recoveryInfo = CM::JobRecoveryInfo();
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
    String normalized = source;
    normalized.replace('/', ',');
    normalized.replace(';', ',');
    normalized.replace(' ', ',');
    uint8_t count = 0U;
    int start = 0;
    while (start < normalized.length())
    {
        while (start < normalized.length() && normalized[start] == ',') ++start;
        if (start >= normalized.length()) break;
        int end = normalized.indexOf(',', start);
        if (end < 0) end = normalized.length();
        const long value = normalized.substring(start, end).toInt();
        if (count >= MaxWebCoils || value < 1L || value > MaxTurnsPerCoil) return false;
        job.turns[count++] = static_cast<uint16_t>(value);
        start = end + 1;
    }
    job.coilCount = count;
    return count > 0U;
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
    response += F("\",\"completed_runs\":"); response += completedRuns;
    response += F(",\"last_run_id\":"); response += lastRunId;
    response += F(",\"run_active\":"); response += runActive ? F("true") : F("false");
    response += F(",\"arduino_ack_pending\":"); response += jobAwaitingAck ? F("true") : F("false");
    response += F(",\"arduino_online\":"); response += arduinoOnline ? F("true") : F("false");
    response += F(",\"state_recovered\":"); response += stateRecovered ? F("true") : F("false");
    response += F(",\"manual_review_required\":"); response += manualReviewRequired() ? F("true") : F("false");
    response += F(",\"new_job_allowed\":"); response += recoveryInfo.mayCreateNewJob ? F("true") : F("false");
    response += F(",\"automatic_queue_allowed\":false,\"automatic_resume_allowed\":false");
    response += F(",\"storage_ready\":"); response += journalReady ? F("true") : F("false");
    response += F(",\"id_allocator_ready\":"); response += idAllocatorReady && idAllocator.isReady() ? F("true") : F("false");
    response += F(",\"job_snapshot_store_ready\":"); response += jobSnapshotStoreReady && jobSnapshots.isReady() ? F("true") : F("false");
    response += F(",\"job_state_store_ready\":"); response += jobStateStoreReady && jobStates.isReady() ? F("true") : F("false");
    response += F(",\"last_allocated_job_id\":"); response += idAllocator.lastJobId();
    response += F(",\"last_allocated_session_id\":"); response += idAllocator.lastSessionId();
    response += F(",\"warehouse_ready\":"); response += warehouseReady ? F("true") : F("false");
    response += F(",\"web_storage_ready\":"); response += staticSites.storageReady() ? F("true") : F("false");
    response += F("}");
    webServer.send(200, "application/json; charset=utf-8", response);
}

void handleCreateJob()
{
    if (!recoveryEvaluated || !recoveryInfo.mayCreateNewJob)
    {
        webServer.send(409, "application/json", "{\"error\":\"manual_review_required\"}");
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
    if (!jobSnapshots.create(job, createdMs))
    {
        webServer.send(503, "application/json", "{\"error\":\"job_snapshot_persistence_failed\"}");
        return;
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
    activeCoilCount = job.coilCount;
    for (uint8_t i = 0U; i < activeCoilCount; ++i) activeTurns[i] = job.turns[i];
    completedRuns = 0U;
    lastRunId = 0UL;
    runActive = false;
    lastJobResult = CM::JobDeliveryResult::None;
    jobAwaitingAck = true;
    recoveryInfo = CM::JobRecoveryInfo();
    recoveryInfo.mayCreateNewJob = false;
    recoveryEvaluated = true;
    stateRecovered = false;

    String response = F("{\"accepted\":true,\"job_id\":"); response += job.jobId;
    response += F(",\"session_id\":"); response += job.sessionId;
    response += F(",\"snapshot_saved\":true,\"state_saved\":true");
    response += F(",\"status\":\"WAITING_ARDUINO_ACK\"}");
    webServer.send(202, "application/json; charset=utf-8", response);
}

void configureWebServer()
{
    webServer.on("/api/status", HTTP_GET, sendJsonStatus);
    webServer.on("/api/jobs", HTTP_POST, handleCreateJob);
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
            Serial.println(F("ERROR: job delivery state persistence failed"));
            continue;
        }

        activeJobId = delivery.jobId;
        lastJobResult = delivery.result;
        jobAwaitingAck = false;
        lastArduinoEventMs = millis();
        recoveryInfo.mayCreateNewJob = delivery.result != CM::JobDeliveryResult::Accepted;
        Serial.print(F("JOB_ACK id=")); Serial.print(delivery.jobId);
        Serial.print(F(" result="));
        Serial.println(delivery.result == CM::JobDeliveryResult::Accepted ? F("ACCEPTED_READY") : F("NOT_ACCEPTED"));
    }
}
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);
    SPI.begin(SdSckPin, SdMisoPin, SdMosiPin, SdCsPin);
    const bool sdReady = SD.begin(SdCsPin, SPI);
    journalReady = sdReady && journal.begin();
    idAllocatorReady = sdReady && idAllocator.begin();
    jobSnapshotStoreReady = sdReady && jobSnapshots.begin();
    jobStateStoreReady = sdReady && jobStates.begin();
    warehouseReady = sdReady && warehouse.begin();
    restoreLatestJobState();

    WiFi.mode(WIFI_AP);
    const bool accessPointReady = WiFi.softAP(AccessPointName, AccessPointPassword);
    configureWebServer();
    Serial.println(F("CoilMaster ESP32 web portal ready"));
    Serial.println(journalReady ? F("microSD winding journal ready") : F("WARNING: microSD winding journal unavailable"));
    Serial.println(idAllocatorReady ? F("persistent job/session ID allocator ready") : F("WARNING: persistent ID allocator unavailable; job creation blocked"));
    Serial.println(jobSnapshotStoreReady ? F("immutable job snapshot store ready") : F("WARNING: job snapshot store unavailable; job creation blocked"));
    Serial.println(jobStateStoreReady ? F("persistent job runtime state store ready") : F("WARNING: job state store unavailable; job creation blocked"));
    Serial.println(warehouseReady ? F("microSD warehouse store ready") : F("WARNING: microSD warehouse store unavailable"));
    Serial.println(staticSites.storageReady() ? F("microSD web root /web ready") : F("WARNING: microSD web root /web unavailable"));
    Serial.println(accessPointReady ? F("Wi-Fi AP CoilMaster ready") : F("WARNING: Wi-Fi AP failed"));
    Serial.print(F("Open http://")); Serial.println(WiFi.softAPIP());
}

void loop()
{
    const uint32_t nowMs = millis();
    webServer.handleClient();
    receiver.update(nowMs);
    CM::RemoteWindingEvent event{};
    while (receiver.poll(event)) handleEvent(event);
    processJobDelivery();
}
