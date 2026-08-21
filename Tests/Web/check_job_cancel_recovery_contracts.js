const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];

function read(relative) {
  return fs.readFileSync(path.join(root, relative), 'utf8');
}

function requireText(relative, source, text, description) {
  if (!source.includes(text)) failures.push(relative + ': ' + description);
}

const espTransportPath = 'firmware/esp32/src/CM_UartEventReceiver.cpp';
const arduinoMainPath = 'firmware/arduino/src/main.cpp';
const arduinoTransportPath = 'Arduino/CM_UartEventTransport.cpp';

const espTransport = read(espTransportPath);
const arduinoMain = read(arduinoMainPath);
const arduinoTransport = read(arduinoTransportPath);

// If a JOB frame may already have reached Arduino, cancelling on ESP32 must
// switch from local cancellation to the idempotent remote JOB_CANCEL handshake.
for (const text of [
  'const bool mayHaveReachedArduino = m_waitingJobAck || sendAttempts != 0U;',
  'if (!mayHaveReachedArduino)',
  'm_cancelJobId = jobId;',
  'm_hasPendingCancel = true;',
  'CMP1|JOB_CANCEL|%lu'
]) {
  requireText(espTransportPath, espTransport, text,
    'lost-ACK ghost-job cancellation contract missing: ' + text);
}

// A physical Arduino fallback publishes a CRC-protected job_id=0 ALL_CLEAR.
// ESP32 must correlate that proof to the current/recovered job and route it
// through the normal cancellation event, never as winding completion evidence.
for (const text of [
  'if (jobId == 0UL)',
  'strcmp(detail, "ALL_CLEAR") != 0',
  'm_hasPendingCancel\n            ? m_cancelJobId',
  'publishJobCancel(JobCancelResult::Cancelled,',
  '"ALL_CLEAR"'
]) {
  requireText(espTransportPath, espTransport, text,
    'ESP32 ALL_CLEAR recovery contract missing: ' + text);
}

// Arduino remote cancel remains safe, exact and idempotent for a job that is
// already absent. ALREADY_CLEAR is success so retries/reboots cannot strand ESP32.
for (const text of [
  'void processRemoteCancels()',
  'espTransport.takeRemoteCancel(jobId)',
  'detail = "ALREADY_CLEAR";',
  'espTransport.sendJobCancelResult(jobId, cancelled, detail);'
]) {
  requireText(arduinoMainPath, arduinoMain, text,
    'Arduino idempotent remote cancel contract missing: ' + text);
}

// The local emergency sequence can clear only a no-run READY remote job. It
// must reject an active physical run and emit ALL_CLEAR only after Arduino has
// proved that it holds no remote job.
for (const text of [
  "static const char Sequence[] = {'D', '*', '#', 'D'};",
  'machine.state() == CM::MachineState::Ready &&',
  'active.currentRunId == 0UL && active.completedRuns == 0U;',
  'if (!safeRemoteReady || !machine.cancel())',
  'CM_JOB EMERGENCY_CLEAR result=REJECTED_ACTIVE_RUN',
  'espTransport.sendJobClear();'
]) {
  requireText(arduinoMainPath, arduinoMain, text,
    'Arduino physical ALL_CLEAR safety contract missing: ' + text);
}

requireText(arduinoTransportPath, arduinoTransport,
  'CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C',
  'Arduino CRC-capable ALL_CLEAR frame missing');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('JOB cancel/recovery contracts OK: lost-ACK remote cancel, idempotent ALREADY_CLEAR, safe physical ALL_CLEAR, active-run rejection, and normal ESP32 cancellation correlation.');
