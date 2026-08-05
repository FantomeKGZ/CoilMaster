#ifndef CM_UART_EVENT_RECEIVER_H
#define CM_UART_EVENT_RECEIVER_H

#include <Arduino.h>

namespace CM
{
enum class RemoteEventType : uint8_t
{
    None = 0U,
    RunStarted,
    RunCompleted
};

struct RemoteWindingEvent
{
    RemoteEventType type;
    uint32_t sessionId;
    uint32_t runId;
    uint16_t completedRuns;
};

class UartEventReceiver
{
public:
    explicit UartEventReceiver(HardwareSerial& serial);

    void begin(uint32_t baud, int8_t rxPin, int8_t txPin);
    bool poll(RemoteWindingEvent& event);
    void sendAck(const RemoteWindingEvent& event);
    void sendNack(uint32_t runId, const char* reason);

private:
    static constexpr size_t MaxLineLength = 96U;

    bool parseLine(char* line, RemoteWindingEvent& event) const;
    static uint16_t crc16(const char* data, size_t length);
    static bool parseHex16(const char* text, uint16_t& value);

    HardwareSerial& m_serial;
    char m_line[MaxLineLength];
    size_t m_length;
};
}

#endif // CM_UART_EVENT_RECEIVER_H
