'use strict';

const fs = require('fs');

const path = 'Arduino/CM_UartEventTransport.cpp';
const source = fs.readFileSync(path, 'utf8');

function requireText(text, message) {
  if (!source.includes(text)) throw new Error(message || `missing ${text}`);
}

function forbidText(text, message) {
  if (source.includes(text)) throw new Error(message || `forbidden ${text}`);
}

requireText('bool parseCanonicalUnsigned(const char* text,',
  'Arduino JOB parser must use strict canonical unsigned parsing');
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

console.log('Arduino JOB parser contracts: OK');
