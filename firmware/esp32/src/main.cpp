#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

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

HardwareSerial arduinoSerial(2);
CM::UartEventReceiver receiver(arduinoSerial);
CM::WindingJournal journal(SD);
uint32_t nextDemoJobId = 1UL;
uint32_t nextDemoSessionId = 1000UL;

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

void queueDemoJob()
{
    CM::OutgoingWindingJob job;
    job.jobId = nextDemoJobId++;
    job.sessionId = nextDemoSessionId++;
    job.type = CM::RemoteJobType::Working;
    job.coilCount = 4U;
    job.turns[0] = 140U;
    job.turns[1] = 100U;
    job.turns[2] = 80U;
    job.turns[3] = 40U;

    if (receiver.queueJob(job))
    {
        Serial.print(F("JOB queued id="));
        Serial.print(job.jobId);
        Serial.println(F(" turns=140,100,80,40"));
    }
    else
    {
        Serial.println(F("JOB queue rejected: sender busy or invalid job"));
    }
}

void processUsbCommands()
{
    if (Serial.available() <= 0)
    {
        return;
    }

    const String command = Serial.readStringUntil('\n');
    String normalized = command;
    normalized.trim();
    normalized.toLowerCase();

    if (normalized == "demo")
    {
        queueDemoJob();
    }
    else if (normalized.length() > 0U)
    {
        Serial.println(F("Commands: demo"));
    }
}

void processJobDelivery()
{
    CM::JobDeliveryEvent delivery;
    while (receiver.takeJobDelivery(delivery))
    {
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

    Serial.println(F("CoilMaster ESP32 UART link ready"));
    Serial.println(journalReady
                       ? F("microSD winding journal ready")
                       : F("WARNING: microSD winding journal unavailable"));
    Serial.println(F("Type 'demo' in Serial Monitor to send 140/100/80/40"));
}

void loop()
{
    const uint32_t nowMs = millis();
    processUsbCommands();
    receiver.update(nowMs);

    CM::RemoteWindingEvent event{};
    while (receiver.poll(event))
    {
        handleEvent(event);
    }

    processJobDelivery();
}
