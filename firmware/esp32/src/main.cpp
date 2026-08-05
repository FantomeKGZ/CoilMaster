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
uint32_t activeSessionId = 0UL;
uint32_t lastRunId = 0UL;
uint32_t lastArduinoEventMs = 0UL;
uint16_t completedRuns = 0U;
uint8_t activeCoilCount = 0U;
uint16_t activeTurns[MaxWebCoils] = {};
CM::RemoteJobType activeJobType = CM::RemoteJobType::Working;
CM::JobDeliveryResult lastJobResult = CM::JobDeliveryResult::None;
bool jobAwaitingAck = false;
bool runActive = false;
bool journalReady = false;

const char IndexPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CoilMaster</title>
<style>
:root{--bg:#eef2f6;--card:#fff;--text:#16202a;--muted:#66727f;--blue:#1769aa;--green:#18864b;--orange:#b86d00;--red:#b42318}
*{box-sizing:border-box}body{margin:0;font-family:Arial,sans-serif;background:var(--bg);color:var(--text)}
header{background:#142536;color:#fff;padding:16px 20px;position:sticky;top:0;z-index:2}header b{font-size:22px}header small{display:block;color:#bfd0df;margin-top:3px}
nav{display:flex;gap:8px;overflow:auto;padding:10px 14px;background:#fff;border-bottom:1px solid #dce3ea}nav a{white-space:nowrap;text-decoration:none;color:#334155;padding:9px 12px;border-radius:8px}nav a.active{background:#e8f2fb;color:#0b5d96;font-weight:bold}
main{max-width:1080px;margin:auto;padding:16px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.card{background:var(--card);border-radius:14px;padding:17px;box-shadow:0 2px 12px #1b273315;margin-bottom:14px}.metric strong{display:block;font-size:28px;margin-top:8px}.muted{color:var(--muted)}
.two{display:grid;grid-template-columns:1.15fr .85fr;gap:14px}.badge{display:inline-block;padding:6px 10px;border-radius:999px;font-size:13px;font-weight:bold;background:#e7edf3}.ok{background:#e4f5eb;color:var(--green)}.wait{background:#fff1d8;color:var(--orange)}.bad{background:#fde8e7;color:var(--red)}
input,select,button{width:100%;padding:12px;margin-top:8px;border:1px solid #cdd6df;border-radius:9px;font-size:16px}button{border:0;background:var(--blue);color:#fff;font-weight:bold;cursor:pointer}button:disabled{opacity:.55}.program{font-size:20px;font-weight:bold;word-break:break-word}.progress{height:12px;background:#e4eaf0;border-radius:20px;overflow:hidden;margin-top:10px}.progress i{display:block;height:100%;width:0;background:var(--blue)}
.notice{padding:12px;border-radius:9px;background:#f3f7fa;margin-top:12px}.row{display:flex;justify-content:space-between;gap:12px;padding:8px 0;border-bottom:1px solid #edf1f4}.row:last-child{border:0}
@media(max-width:800px){.grid{grid-template-columns:repeat(2,1fr)}.two{grid-template-columns:1fr}}@media(max-width:430px){.grid{grid-template-columns:1fr 1fr}.metric strong{font-size:23px}main{padding:10px}.card{padding:14px}}
</style>
</head>
<body>
<header><b>CoilMaster</b><small>Портал управления намоточным станком</small></header>
<nav><a class="active" href="#">Намотка</a><a href="#">Главная</a><a href="#">Двигатели</a><a href="#">Клиенты</a><a href="#">История</a><a href="#">Склад</a><a href="#">Настройки</a></nav>
<main>
<div class="grid">
 <div class="card metric"><span class="muted">Состояние станка</span><strong id="machine">—</strong></div>
 <div class="card metric"><span class="muted">Выполнено повторов</span><strong id="runs">0</strong></div>
 <div class="card metric"><span class="muted">Последний проход</span><strong id="runid">—</strong></div>
 <div class="card metric"><span class="muted">microSD</span><strong id="sd">—</strong></div>
</div>
<div class="two">
 <section class="card">
  <h2>Мониторинг намотки</h2>
  <div class="row"><span class="muted">Задание</span><b id="job">—</b></div>
  <div class="row"><span class="muted">Тип</span><b id="type">—</b></div>
  <div class="row"><span class="muted">Программа</span><span id="program" class="program">Нет активной программы</span></div>
  <div class="row"><span class="muted">Связь Arduino</span><span id="link" class="badge">Нет данных</span></div>
  <div class="progress"><i id="bar"></i></div>
  <p id="hint" class="notice">Отправьте программу на Arduino. Физический запуск выполняется только клавишей A или внешней кнопкой START.</p>
 </section>
 <section class="card">
  <h2>Отправить программу</h2>
  <form id="jobForm">
   <label>Тип обмотки</label>
   <select name="type"><option value="working">Рабочая</option><option value="starting">Пусковая</option></select>
   <label>Витки катушек</label>
   <input name="turns" placeholder="140/100/80/40" required>
   <button id="sendButton" type="submit">Отправить на Arduino</button>
  </form>
  <p class="muted">До 10 катушек, от 1 до 9999 витков каждая. Автозапуск запрещён.</p>
  <div id="formResult" class="notice">Готово к вводу.</div>
 </section>
</div>
</main>
<script>
const $=id=>document.getElementById(id);
function badge(el,text,cls){el.textContent=text;el.className='badge '+cls}
async function refresh(){try{const r=await fetch('/api/status',{cache:'no-store'});const s=await r.json();
 $('machine').textContent=s.machine_status; $('runs').textContent=s.completed_runs; $('runid').textContent=s.last_run_id||'—'; $('sd').textContent=s.storage_ready?'OK':'Ошибка';
 $('job').textContent=s.job_id||'—'; $('type').textContent=s.job_type==='STARTING'?'Пусковая':s.job_id?'Рабочая':'—'; $('program').textContent=s.program||'Нет активной программы';
 if(s.arduino_online)badge($('link'),'Подключено','ok');else badge($('link'),'Нет свежих данных','wait');
 $('bar').style.width=s.run_active?'55%':(s.job_status==='ACCEPTED_READY'?'12%':s.completed_runs>0?'100%':'0%');
 $('hint').textContent=s.run_active?'Намотка выполняется. Точный счётчик витков будет добавлен следующим этапом телеметрии.':s.job_status==='ACCEPTED_READY'?'Arduino приняла программу. Нажмите A или внешнюю кнопку START на станке.':s.job_status==='WAITING_ARDUINO_ACK'?'Ожидание подтверждения Arduino…':'Станок ожидает программу.';
}catch(e){$('machine').textContent='Нет связи';badge($('link'),'ESP32 недоступна','bad')}}
$('jobForm').addEventListener('submit',async e=>{e.preventDefault();const b=$('sendButton');b.disabled=true;$('formResult').textContent='Отправка…';try{const r=await fetch('/api/jobs',{method:'POST',body:new FormData(e.target)});const j=await r.json();$('formResult').textContent=r.ok?`Задание №${j.job_id} передано в очередь.`:`Ошибка: ${j.error||'unknown'}`;}catch(err){$('formResult').textContent='Ошибка связи с ESP32';}b.disabled=false;refresh();});
refresh();setInterval(refresh,1200);
</script>
</body></html>
)HTML";

const char* jobStatusText()
{
    if (jobAwaitingAck) return "WAITING_ARDUINO_ACK";
    switch (lastJobResult)
    {
        case CM::JobDeliveryResult::Accepted: return "ACCEPTED_READY";
        case CM::JobDeliveryResult::Rejected: return "REJECTED";
        case CM::JobDeliveryResult::None:
        default: return "IDLE";
    }
}

const char* machineStatusText()
{
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

void printEvent(const CM::RemoteWindingEvent& event)
{
    Serial.print(F("CMP RX type="));
    Serial.print(event.type == CM::RemoteEventType::RunStarted ? F("RUN_STARTED") : F("RUN_COMPLETED"));
    Serial.print(F(" session=")); Serial.print(event.sessionId);
    Serial.print(F(" run=")); Serial.print(event.runId);
    Serial.print(F(" completed=")); Serial.println(event.completedRuns);
}

void handleEvent(const CM::RemoteWindingEvent& event)
{
    printEvent(event);
    lastArduinoEventMs = millis();
    activeSessionId = event.sessionId;
    lastRunId = event.runId;
    completedRuns = event.completedRuns;
    runActive = event.type == CM::RemoteEventType::RunStarted;

    const CM::JournalSaveResult result = journal.save(event);
    switch (result)
    {
        case CM::JournalSaveResult::Saved:
            receiver.sendAck(event.runId, event.type == CM::RemoteEventType::RunCompleted ? "SAVED" : "RECORDED");
            break;
        case CM::JournalSaveResult::Duplicate:
            receiver.sendAck(event.runId, "DUPLICATE");
            break;
        case CM::JournalSaveResult::StorageUnavailable:
            receiver.sendNack(event.runId, "STORAGE_UNAVAILABLE");
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
    normalized.replace('/', ','); normalized.replace(';', ','); normalized.replace(' ', ',');
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
    const bool arduinoOnline = lastArduinoEventMs > 0UL && static_cast<uint32_t>(millis() - lastArduinoEventMs) < 15000UL;
    String response = F("{\"job_id\":"); response += activeJobId;
    response += F(",\"session_id\":"); response += activeSessionId;
    response += F(",\"job_status\":\""); response += jobStatusText();
    response += F("\",\"machine_status\":\""); response += machineStatusText();
    response += F("\",\"job_type\":\""); response += activeJobType == CM::RemoteJobType::Starting ? F("STARTING") : F("WORKING");
    response += F("\",\"program\":\""); response += activeProgramText();
    response += F("\",\"completed_runs\":"); response += completedRuns;
    response += F(",\"last_run_id\":"); response += lastRunId;
    response += F(",\"run_active\":"); response += runActive ? F("true") : F("false");
    response += F(",\"arduino_ack_pending\":"); response += jobAwaitingAck ? F("true") : F("false");
    response += F(",\"arduino_online\":"); response += arduinoOnline ? F("true") : F("false");
    response += F(",\"storage_ready\":"); response += journalReady ? F("true") : F("false");
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
    job.type = webServer.arg("type") == "starting" ? CM::RemoteJobType::Starting : CM::RemoteJobType::Working;
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
    activeSessionId = job.sessionId;
    activeJobType = job.type;
    activeCoilCount = job.coilCount;
    for (uint8_t i = 0U; i < activeCoilCount; ++i) activeTurns[i] = job.turns[i];
    completedRuns = 0U;
    lastRunId = 0UL;
    runActive = false;
    lastJobResult = CM::JobDeliveryResult::None;
    jobAwaitingAck = true;

    String response = F("{\"accepted\":true,\"job_id\":"); response += job.jobId;
    response += F(",\"status\":\"WAITING_ARDUINO_ACK\"}");
    webServer.send(202, "application/json; charset=utf-8", response);
}

void configureWebServer()
{
    webServer.on("/", HTTP_GET, [](){ webServer.send_P(200, "text/html; charset=utf-8", IndexPage); });
    webServer.on("/api/status", HTTP_GET, sendJsonStatus);
    webServer.on("/api/jobs", HTTP_POST, handleCreateJob);
    webServer.onNotFound([](){ webServer.send(404, "application/json", "{\"error\":\"not_found\"}"); });
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
        lastArduinoEventMs = millis();
        Serial.print(F("JOB_ACK id=")); Serial.print(delivery.jobId);
        Serial.print(F(" result="));
        Serial.println(delivery.result == CM::JobDeliveryResult::Accepted ? F("ACCEPTED_READY") : F("REJECTED"));
    }
}
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);
    SPI.begin(SdSckPin, SdMisoPin, SdMosiPin, SdCsPin);
    journalReady = SD.begin(SdCsPin, SPI) && journal.begin();
    WiFi.mode(WIFI_AP);
    const bool accessPointReady = WiFi.softAP(AccessPointName, AccessPointPassword);
    configureWebServer();

    Serial.println(F("CoilMaster ESP32 dashboard ready"));
    Serial.println(journalReady ? F("microSD winding journal ready") : F("WARNING: microSD winding journal unavailable"));
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
