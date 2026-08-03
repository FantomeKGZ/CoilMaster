#pragma once

#include <Arduino.h>

class CM_System {
public:
    void begin();
    void update();
    bool test();
    const char* status() const;
    void reset();

private:
    bool m_initialized = false;
    bool m_ready = false;
};