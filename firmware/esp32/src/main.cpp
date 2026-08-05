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
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);

    SPI.begin(SdSckPin, SdMisoPin, SdMosiPin, SdCsPin);
    const bool sdReady = SD.begin(SdCsPin, SPI);
    const bool journalReady = sdReady && journal.begin();

    Serial.println(F("CoilMaster ESP32 UART receiver ready"));
    Serial.println(journalReady
                       ? F("microSD winding journal ready")
                       : F("WARNING: microSD winding journal unavailable"));
}

void loop()
{
    CM::RemoteWindingEvent event{};
    if (receiver.poll(event))
    {
        handleEvent(event);
    }
}
