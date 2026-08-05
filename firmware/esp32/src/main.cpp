#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

#include "CM_UartEventReceiver.h"
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
WebServer webServer(80);

uint32_t nextJobId = 1UL;
uint32_t nextSessionId = 1000UL;
uint32_t activeJobId = 0UL;
CM::JobDeliveryResult lastJobResult = CM::JobDeliveryResult::None;
bool jobAwaitingAck = false;

const char IndexPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CoilMaster</title>
<style>
body{font-family:Arial,sans-serif;max-width:720px;margin:24px auto;padding:0 16px;background:#f4f5f7;color:#20242a}
.card{background:white;border-radius:12px;padding:18px;box-shadow:0 2px 10px #0001;margin-bottom:16px}
input,select,button{width:100%;box-sizing:border-box;padding:12px;margin-top:8px;font-size:16px}
button{background:#1769aa;color:white;border:0;border-radius:8px;font-weight:bold}
small{color:#59636e}.status{font-weight:bold}
</style>
</head>
<body>
<h1>CoilMaster</h1>
<div class="card">
<h2>Отправить программу</h2>
<form method="post" action="/api/jobs">
<label>Тип обмотки</label>
<select name="type"><option value="working">Рабочая</option><option value="starting">Пусковая</option></select>
<label>Витки катушек через / или запятую</label>
<input name="turns" placeholder="140/100/80/40" required>
<button type="submit">Отправить на Arduino</button>
</form>
<p><small>Станок не запускается автоматически. После принятия задания нажмите A или внешнюю кнопку START.</small></p>
</div>
<div class="card"><h2>Состояние</h2><p id="status" class="status">Загрузка...</p></div>
<script>
async function refresh(){try{const r=await fetch('/api/status');const s=await r.json();document.getElementById('status').textContent=`Задание: ${s.job_id||'-'}; состояние: ${s.job_status}`;}catch(e){document.getElementById('status').textContent='Нет связи с ESP32';}}
refresh();setInterval(refresh,1500);
</script>
</body></html>
)HTML";

const char* jobStatusText()
{
    if (jobAwaitingAck)
    {
        return "WAITING_ARDUINO_ACK";
    }

    switch (lastJobResult)
    {
        case CM::JobDeliveryResult::Accepted:
            return "ACCEPTED_READY";
        case CM::JobDeliveryResult::Rejected:
            return "REJECTED";
        case CM::JobDeliveryResult::None:
        default:
            return "IDLE";
    }
}

void printEvent(const CM::RemoteWindingEvent& event)
{
    Serial.print(F("CMP RX type="));
    Serial.print(event.type == CM::RemoteEventType::RunStarted
                     ? F("RUN_STARTED")
                     : F("RUN_COMPLETED"));
    Serial.print(F(" session="));
    Serial.print(event.sessionId);
    Serial.print(F(" run="));
    Serial.print(event.runId);
    Serial.print(F(" completed="));
    Serial.println(event.completedRuns);
}

void handleEvent(const CM::RemoteWindingEvent& event)
{
    printEvent(event);

    const CM::JournalSaveResult result = journal.save(event);
    switch (result)
    {
        case CM::JournalSaveResult::Saved:
            receiver.sendAck(event.runId,
                             event.type == CM::RemoteEventType::RunCompleted
                                 ? "SAVED"
                                 : "RECORDED");
            Serial.println(F("Journal: event saved"));
            break;

        case CM::JournalSaveResult::Duplicate:
            receiver.sendAck(event.runId, "DUPLICATE");
            Serial.println(F("Journal: duplicate ignored"));
            break;

        case CM::JournalSaveResult::StorageUnavailable:
            receiver.sendNack(event.runId, "STORAGE_UNAVAILABLE");
            Serial.println(F("Journal: storage unavailable"));
            break;

        case CM::JournalSaveResult::WriteFailed:
        default:
            receiver.sendNack(event.runId, "WRITE_FAILED");
            Serial.println(F("Journal: write failed"));
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
        while (start < normalized.length() && normalized[start] == ',')
        {
            ++start;
        }
        if (start >= normalized.length())
        {
            break;
        }

        int end = normalized.indexOf(',', start);
        if (end < 0)
        {
            end = normalized.length();
        }

        const String token = normalized.substring(start, end);
        const long value = token.toInt();
        if (count >= MaxWebCoils || value < 1L || value > MaxTurnsPerCoil)
        {
            return false;
        }

        job.turns[count++] = static_cast<uint16_t>(value);
        start = end + 1;
    }

    job.coilCount = count;
    return count > 0U;
}

void sendJsonStatus()
{
    String response = F("{\"job_id\":");
    response += activeJobId;
    response += F(",\"job_status\":\"");
    response += jobStatusText();
    response += F("\",\"arduino_ack_pending\":");
    response += jobAwaitingAck ? F("true") : F("false");
    response += F("}");
    webServer.send(200, "application/json; charset=utf-8", response);
}

void handleCreateJob()
{
    if (!webServer.hasArg("turns"))
    {
        webServer.send(400, "application/json", "{\"error\":\"turns_required\"}");
        return;
    }

    CM::OutgoingWindingJob job;
    job.jobId = nextJobId++;
    job.sessionId = nextSessionId++;
    job.type = webServer.arg("type") == "starting"
                   ? CM::RemoteJobType::Starting
                   : CM::RemoteJobType::Working;

    if (!parseTurns(webServer.arg("turns"), job))
    {
        webServer.send(400, "application/json", "{\"error\":\"invalid_turns\"}");
        return;
    }

    if (!receiver.queueJob(job))
    {
        webServer.send(409, "application/json", "{\"error\":\"sender_busy\"}");
        return;
    }

    activeJobId = job.jobId;
    lastJobResult = CM::JobDeliveryResult::None;
    jobAwaitingAck = true;

    String response = F("{\"accepted\":true,\"job_id\":");
    response += job.jobId;
    response += F(",\"status\":\"WAITING_ARDUINO_ACK\"}");
    webServer.send(202, "application/json; charset=utf-8", response);
}

void configureWebServer()
{
    webServer.on("/", HTTP_GET, []()
    {
        webServer.send_P(200, "text/html; charset=utf-8", IndexPage);
    });
    webServer.on("/api/status", HTTP_GET, sendJsonStatus);
    webServer.on("/api/jobs", HTTP_POST, handleCreateJob);
    webServer.onNotFound([]()
    {
        webServer.send(404, "application/json", "{\"error\":\"not_found\"}");
    });
    webServer.begin();
}

void processJobDelivery()
{
    CM::JobDeliveryEvent delivery;
    while (receiver.takeJobDelivery(delivery))
    {
        activeJobId = delivery.jobId;
        lastJobResult = delivery.result;
        jobAwaitingAck = false;

        Serial.print(F("JOB_ACK id="));
        Serial.print(delivery.jobId);
        Serial.print(F(" result="));
        Serial.println(delivery.result == CM::JobDeliveryResult::Accepted
                           ? F("ACCEPTED_READY")
                           : F("REJECTED"));
    }
}
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);

    SPI.begin(SdSckPin, SdMisoPin, SdMosiPin, SdCsPin);
    const bool sdReady = SD.begin(SdCsPin, SPI);
    const bool journalReady = sdReady && journal.begin();

    WiFi.mode(WIFI_AP);
    const bool accessPointReady = WiFi.softAP(AccessPointName, AccessPointPassword);
    configureWebServer();

    Serial.println(F("CoilMaster ESP32 UART link ready"));
    Serial.println(journalReady
                       ? F("microSD winding journal ready")
                       : F("WARNING: microSD winding journal unavailable"));
    Serial.println(accessPointReady
                       ? F("Wi-Fi AP CoilMaster ready")
                       : F("WARNING: Wi-Fi AP failed"));
    Serial.print(F("Open http://"));
    Serial.println(WiFi.softAPIP());
}

void loop()
{
    const uint32_t nowMs = millis();
    webServer.handleClient();
    receiver.update(nowMs);

    CM::RemoteWindingEvent event{};
    while (receiver.poll(event))
    {
        handleEvent(event);
    }

    processJobDelivery();
}
