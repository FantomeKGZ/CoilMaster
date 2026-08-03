#include "CM_System.h"

void CM_System::begin() {
    m_initialized = true;
    m_ready = true;
}

void CM_System::update() {
    // В Build 002 здесь будет центральная логика системы.
}

bool CM_System::test() {
    return m_initialized && m_ready;
}

const char* CM_System::status() const {
    if (!m_initialized) return "NOT_INITIALIZED";
    if (!m_ready) return "WARNING";
    return "READY";
}

void CM_System::reset() {
    m_initialized = false;
    m_ready = false;
}