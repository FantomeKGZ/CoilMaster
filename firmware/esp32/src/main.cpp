#include <Arduino.h>

#include "CM_UartEventReceiver.h"

namespace
{
constexpr int8_t ArduinoRxPin = 16;
constexpr int8_t ArduinoTxPin = 17;
constexpr uint32_t ArduinoBaud = 9600UL;

HardwareSerial arduinoSerial(2);
CM::UartEventReceiver receiver(arduinoSerial);

void handleEvent(const CM::RemoteWindingEvent& event)
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

    // Temporary acceptance point. Persistent microSD storage is added next.
    receiver.sendAck(event);
}
}

void setup()
{
    Serial.begin(115200);
    receiver.begin(ArduinoBaud, ArduinoRxPin, ArduinoTxPin);
    Serial.println(F("CoilMaster ESP32 UART receiver ready"));
}

void loop()
{
    CM::RemoteWindingEvent event{};
    if (receiver.poll(event))
    {
        handleEvent(event);
    }
}
