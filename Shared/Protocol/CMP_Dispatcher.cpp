#include "CMP_Dispatcher.h"

namespace CMP
{
Dispatcher::Dispatcher()
    : m_entries{},
      m_count(0U)
{
    clear();
}

Result Dispatcher::registerHandler(Command command,
                                   Handler handler,
                                   void* context)
{
    if (handler == nullptr || command == Command::None)
    {
        return Result::InvalidArgument;
    }

    if (find(command) >= 0)
    {
        return Result::Busy;
    }

    for (uint8_t index = 0U; index < MaxHandlers; ++index)
    {
        if (!m_entries[index].active)
        {
            m_entries[index].command = command;
            m_entries[index].handler = handler;
            m_entries[index].context = context;
            m_entries[index].active = true;
            ++m_count;
            return Result::Ok;
        }
    }

    return Result::BufferFull;
}

Result Dispatcher::unregisterHandler(Command command)
{
    const int8_t index = find(command);
    if (index < 0)
    {
        return Result::UnknownCommand;
    }

    Entry& entry = m_entries[static_cast<uint8_t>(index)];
    entry.command = Command::None;
    entry.handler = nullptr;
    entry.context = nullptr;
    entry.active = false;
    --m_count;

    return Result::Ok;
}

Result Dispatcher::dispatch(const Packet& packet) const
{
    const int8_t index = find(packet.header.command);
    if (index < 0)
    {
        return Result::UnknownCommand;
    }

    const Entry& entry = m_entries[static_cast<uint8_t>(index)];
    if (!entry.active || entry.handler == nullptr)
    {
        return Result::UnknownCommand;
    }

    return entry.handler(packet, entry.context);
}

void Dispatcher::clear()
{
    for (uint8_t index = 0U; index < MaxHandlers; ++index)
    {
        m_entries[index].command = Command::None;
        m_entries[index].handler = nullptr;
        m_entries[index].context = nullptr;
        m_entries[index].active = false;
    }

    m_count = 0U;
}

uint8_t Dispatcher::handlerCount() const
{
    return m_count;
}

uint8_t Dispatcher::capacity() const
{
    return MaxHandlers;
}

int8_t Dispatcher::find(Command command) const
{
    for (uint8_t index = 0U; index < MaxHandlers; ++index)
    {
        if (m_entries[index].active &&
            m_entries[index].command == command)
        {
            return static_cast<int8_t>(index);
        }
    }

    return -1;
}
}
