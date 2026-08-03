#ifndef CMP_DISPATCHER_H
#define CMP_DISPATCHER_H

#include <stdint.h>

#include "CMP_Command.h"
#include "CMP_Packet.h"
#include "CMP_Result.h"

namespace CMP
{
class Dispatcher
{
public:
    using Handler = Result (*)(const Packet& packet, void* context);

    Dispatcher();

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Result registerHandler(Command command,
                           Handler handler,
                           void* context = nullptr);

    Result unregisterHandler(Command command);

    Result dispatch(const Packet& packet) const;

    void clear();

    uint8_t handlerCount() const;
    uint8_t capacity() const;

private:
    static constexpr uint8_t MaxHandlers = 16U;

    struct Entry
    {
        Command command;
        Handler handler;
        void* context;
        bool active;
    };

    int8_t find(Command command) const;

    Entry m_entries[MaxHandlers];
    uint8_t m_count;
};
}

#endif // CMP_DISPATCHER_H
