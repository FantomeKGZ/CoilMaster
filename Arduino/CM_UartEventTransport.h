#ifndef CM_UART_EVENT_TRANSPORT_H
#define CM_UART_EVENT_TRANSPORT_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "../Core/CM_WindingEvent.h"

namespace CM
{
class UartEventTransport
{
public:
    UartEventTransport(uint8_t rxPin, uint8_t txPin, uint32_t baudRate);

    void begin();
    bool send(const WindingEvent& event);

private:
    static const char* eventName(WindingEventType type);
    static uint16_t crc16Ccitt(const uint8_t* data, size_t length);

    SoftwareSerial m_serial;
    uint32_t m_baudRate;
};
}

#endif // CM_UART_EVENT_TRANSPORT_H
