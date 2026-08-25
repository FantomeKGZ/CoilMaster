'use strict';

const fs = require('fs');

const path = 'Arduino/CM_UartEventTransport.cpp';
const source = fs.readFileSync(path, 'utf8');
const header = fs.readFileSync('Arduino/CM_UartEventTransport.h', 'utf8');

function requireText(text, message) {
  if (!source.includes(text)) throw new Error(message || `missing ${text}`);
}

function forbidText(text, message) {
  if (source.includes(text)) throw new Error(message || `forbidden ${text}`);
}

requireText('bool parseCanonicalUnsigned(const char* text,',
  'Arduino transport must use strict canonical unsigned parsing');
requireText("if (text[0] == '0' && text[1] != '\\0') return false;",
  'Canonical numeric tokens must reject leading zeroes');
requireText("if (*cursor < '0' || *cursor > '9') return false;",
  'Canonical numeric tokens must reject signs, whitespace and trailing text');
requireText('value > quotient || (value == quotient && digit > remainder)',
  'Canonical numeric parser must reject overflow and per-field range overflow');

requireText('parseCanonicalUnsigned(jobId, 0xFFFFFFFFUL, parsedJobId)',
  'JOB id must be parsed as a complete bounded token');
requireText('parseCanonicalUnsigned(sessionId, 0xFFFFFFFFUL, parsedSessionId)',
  'session id must be parsed as a complete bounded token');
requireText('parseCanonicalUnsigned(count, MaxCoilsPerJob, parsedCoilCount)',
  'coil count must be range checked before narrowing');
requireText('parseCanonicalUnsigned(repeatOrCapability + 1,',
  'repeat target must use strict canonical parsing');
requireText('parseCanonicalUnsigned(token, MaxTurnsPerCoil, value)',
  'every turns token must use strict canonical parsing');
requireText('parsedJobId == 0UL || parsedSessionId == 0UL ||',
  'remote JOB identity must reject zero ids at the wire boundary');

requireText('if (strcmp(type, "STARTING") == 0)',
  'STARTING winding type must remain accepted');
requireText('else if (strcmp(type, "WORKING") == 0)',
  'WORKING winding type must remain accepted');
requireText('else\n        return false;',
  'Unknown winding type must fail closed');

requireText('parseCanonicalUnsigned(runText, 0xFFFFFFFFUL, runId)',
  'ACK/NACK run id must be parsed as a complete canonical token before queue correlation');
requireText('parseCanonicalUnsigned(jobText, 0xFFFFFFFFUL, parsed)',
  'JOB_CANCEL job id must be parsed as a complete canonical token');

forbidText('job.jobId = strtoul(jobId, nullptr, 10);',
  'JOB id must not return to permissive strtoul parsing');
forbidText('job.sessionId = strtoul(sessionId, nullptr, 10);',
  'session id must not return to permissive strtoul parsing');
forbidText('static_cast<uint8_t>(strtoul(count, nullptr, 10))',
  'coil count must not be narrowed before validation');
forbidText('const unsigned long value = strtoul(token, nullptr, 10);',
  'turn tokens must not accept numeric prefixes');
forbidText('strcmp(type, "STARTING") == 0 ? WindingType::Starting',
  'unknown winding type must not silently become WORKING');
forbidText('const uint32_t runId = strtoul(runText, nullptr, 10);',
  'ACK/NACK correlation must not accept a numeric prefix');
forbidText('const unsigned long parsed = strtoul(jobText, &end, 10);',
  'JOB_CANCEL correlation must not accept signs/whitespace/non-canonical tokens');

requireText('class CrcFrameWriter',
  'Arduino transport must stream CRC frames with bounded stack');
requireText('frame.write(F("CMP1|LOCAL_EVT|"))',
  'LOCAL_EVT provenance frame must use the streaming writer');
requireText('frame.writeUnsigned(program.coilCount)',
  'streamed LOCAL_EVT must retain exact coil count');
requireText('frame.writeUnsigned(turns)',
  'streamed LOCAL_EVT must retain every exact turn target');
requireText('return frame.finish();',
  'streamed frames must append their CRC suffix');
forbidText('char frame[176]',
  'Uno transport must not restore the 176-byte LOCAL_EVT stack frame');
forbidText('char frame[96]',
  'Uno transport must not restore the 96-byte EVT stack frame');

const maxJob = 'CMP1|JOB|4294967295|4294967295|STARTING|10|' +
  Array(10).fill('9999').join(',') + '|R65535|C|FFFF';
if (maxJob.length !== 106) {
  throw new Error(`test fixture drift: worst-case JOB payload+CRC must be 106 bytes, got ${maxJob.length}`);
}
const replyMatch = header.match(/MaxReplyLength = (\d+)U/);
if (!replyMatch) throw new Error('Arduino transport: MaxReplyLength declaration missing');
const maxReplyLength = Number(replyMatch[1]);
const exactReplyLength = maxJob.length + 1;
if (maxReplyLength !== exactReplyLength) {
  throw new Error(`Arduino transport: MaxReplyLength must equal exact worst-case JOB plus NUL (${exactReplyLength}), got ${maxReplyLength}`);
}

console.log('Arduino JOB/ACK/cancel parser contracts: OK');
