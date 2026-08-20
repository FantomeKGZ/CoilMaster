from pathlib import Path

PATH = Path("firmware/esp32/src/CM_UartEventReceiver.cpp")
EXPECTED_BLOB = "0238720ff8a91c3df1f5bc1091b6c03133a2e1f6"

text = PATH.read_text()


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected 1 anchor, found {count}")
    text = text.replace(old, new, 1)


replace_once(
    ": m_serial(serial), m_line(), m_length(0U), m_pendingJob(),",
    ": m_serial(serial), m_hardwareControl(serial), m_line(), m_length(0U), m_pendingJob(),",
    "constructor",
)

replace_once(
    '''                else if (strncmp(m_line, "CMP1|JOB_CANCEL_ACK|", 20U) == 0)\n                    processCancelAck(m_line);\n                else\n                    eventReady = parseEventLine(m_line, event);''',
    '''                else if (strncmp(m_line, "CMP1|JOB_CANCEL_ACK|", 20U) == 0)\n                    processCancelAck(m_line);\n                else if (m_hardwareControl.processLine(m_line, millis()))\n                {\n                    // Service frame consumed; never reinterpret it as winding evidence.\n                }\n                else\n                    eventReady = parseEventLine(m_line, event);''',
    "poll dispatch",
)

replace_once(
    '''    if (!m_hasPendingCancel) return;\n    if (m_cancelSendAttempts == 0U ||\n        static_cast<uint32_t>(nowMs - m_lastCancelSendMs) >= JobRetryIntervalMs)\n    {\n        if (m_cancelSendAttempts >= MaxCancelSendAttempts)\n        {\n            publishJobCancel(JobCancelResult::TimedOut,\n                             m_cancelJobId,\n                             m_cancelSendAttempts,\n                             "NO_CANCEL_ACK");\n            m_hasPendingCancel = false;\n            m_cancelJobId = 0UL;\n            m_cancelSendAttempts = 0U;\n            return;\n        }\n        sendPendingCancel(nowMs);\n    }\n}''',
    '''    if (m_hasPendingCancel &&\n        (m_cancelSendAttempts == 0U ||\n         static_cast<uint32_t>(nowMs - m_lastCancelSendMs) >= JobRetryIntervalMs))\n    {\n        if (m_cancelSendAttempts >= MaxCancelSendAttempts)\n        {\n            publishJobCancel(JobCancelResult::TimedOut,\n                             m_cancelJobId,\n                             m_cancelSendAttempts,\n                             "NO_CANCEL_ACK");\n            m_hasPendingCancel = false;\n            m_cancelJobId = 0UL;\n            m_cancelSendAttempts = 0U;\n        }\n        else\n            sendPendingCancel(nowMs);\n    }\n\n    m_hardwareControl.update(nowMs);\n}''',
    "update cancel block",
)

replace_once(
    '''bool UartEventReceiver::queueJob(const OutgoingWindingJob& job)\n{\n    if (m_hasPendingJob || m_hasJobDelivery || m_hasPendingCancel ||\n        m_hasJobCancel || !job.isValid()) return false;''',
    '''bool UartEventReceiver::queueJob(const OutgoingWindingJob& job)\n{\n    if (m_hasPendingJob || m_hasJobDelivery || m_hasPendingCancel ||\n        m_hasJobCancel || m_hardwareControl.requestPending() || !job.isValid())\n        return false;''',
    "queue job guard",
)

replace_once(
    '''bool UartEventReceiver::requestJobCancel(uint32_t jobId)\n{\n    if (jobId == 0UL || m_hasPendingJob || m_hasJobDelivery ||\n        m_hasPendingCancel || m_hasJobCancel)\n        return false;''',
    '''bool UartEventReceiver::requestJobCancel(uint32_t jobId)\n{\n    if (jobId == 0UL || m_hasPendingJob || m_hasJobDelivery ||\n        m_hasPendingCancel || m_hasJobCancel || m_hardwareControl.requestPending())\n        return false;''',
    "cancel guard",
)

replace_once(
    '''bool UartEventReceiver::jobCancelPending() const\n{\n    return m_hasPendingCancel;\n}\n\nvoid UartEventReceiver::rememberJobId(uint32_t jobId)''',
    '''bool UartEventReceiver::jobCancelPending() const\n{\n    return m_hasPendingCancel;\n}\n\nbool UartEventReceiver::controlLaneBusy() const\n{\n    return m_hasPendingJob || m_waitingJobAck || m_hasPendingCancel;\n}\n\nbool UartEventReceiver::requestHallSettings()\n{\n    return !controlLaneBusy() && m_hardwareControl.requestSettings();\n}\n\nbool UartEventReceiver::setHallSettings(\n    uint16_t threshold,\n    uint16_t hysteresis,\n    uint16_t releaseDebounceMs,\n    HallSignalDirectionRemote direction)\n{\n    return !controlLaneBusy() &&\n           m_hardwareControl.setSettings(\n               threshold, hysteresis, releaseDebounceMs, direction);\n}\n\nbool UartEventReceiver::resetHallSettings()\n{\n    return !controlLaneBusy() && m_hardwareControl.resetSettings();\n}\n\nbool UartEventReceiver::setHallTelemetryEnabled(bool enabled)\n{\n    return !controlLaneBusy() &&\n           m_hardwareControl.setTelemetryEnabled(enabled);\n}\n\nbool UartEventReceiver::hallControlPending() const\n{\n    return m_hardwareControl.requestPending();\n}\n\nbool UartEventReceiver::takeHallSettings(HallSettingsState& state)\n{\n    return m_hardwareControl.takeSettings(state);\n}\n\nbool UartEventReceiver::takeHallTelemetry(HallTelemetryState& state)\n{\n    return m_hardwareControl.takeTelemetry(state);\n}\n\nbool UartEventReceiver::takeHardwareControlReply(HardwareControlReply& reply)\n{\n    return m_hardwareControl.takeReply(reply);\n}\n\nvoid UartEventReceiver::rememberJobId(uint32_t jobId)''',
    "hardware wrapper block",
)

PATH.write_text(text)
