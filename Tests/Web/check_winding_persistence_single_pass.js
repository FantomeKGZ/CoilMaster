const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const persistencePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingPersistenceIntegrityAudit.cpp');
const transitionHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournalTransitionAudit.h');
const transitionSourcePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournalTransitionAudit.cpp');
const runtimeHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournal.h');
const runtimeSourcePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournal.cpp');

const persistence = fs.readFileSync(persistencePath, 'utf8');
const transitionHeader = fs.readFileSync(transitionHeaderPath, 'utf8');
const transitionSource = fs.readFileSync(transitionSourcePath, 'utf8');
const runtimeHeader = fs.readFileSync(runtimeHeaderPath, 'utf8');
const runtimeSource = fs.readFileSync(runtimeSourcePath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
}

requireText(
  persistence,
  'WindingJournalTransitionAudit::validate(storage, recordCount)',
  'single-pass winding persistence audit');
if (persistence.includes('WindingJournalQuery') || persistence.includes('validateAll(')) {
  throw new Error('Winding persistence audit must not perform a separate journal schema pre-pass');
}

requireText(
  transitionHeader,
  'static WindingJournalTransitionAuditResult validate(fs::FS& storage,\n                                                        uint32_t& recordCount);',
  'record-count transition overload');
requireText(
  transitionSource,
  'if (recordCount != nullptr) *recordCount = 0UL;',
  'record-count reset');
requireText(
  transitionSource,
  'if (*recordCount == 0xFFFFFFFFUL)',
  'record-count overflow guard');
requireText(
  transitionSource,
  '++(*recordCount);',
  'record counting inside transition scan');
requireText(
  transitionSource,
  'WindingJournalQuery::isValidRecord(line)',
  'full schema validation inside transition scan');

// Runtime save/session-state reads must not reopen the growing event journal for
// duplicate, active, highest-run, completed-run and matching-start evidence.
for (const text of [
  'bool analyzeSession(uint32_t sessionId,',
  'WindingSessionState& state,',
  'bool& exactEventFound,',
  'bool& targetRunStartFound) const;'
]) {
  requireText(runtimeHeader, text, 'runtime single-pass analyzer API');
}
for (const removed of [
  'sessionContextMatches(',
  'containsRunEvent(',
  'hasRunStart(',
  'loadSessionCompletedRuns(',
  'loadActiveRun(',
  'loadSessionHighestRunId('
]) {
  if (runtimeHeader.includes(removed) || runtimeSource.includes(removed)) {
    throw new Error(`Winding runtime multi-pass helper returned: ${removed}`);
  }
}
for (const text of [
  'if (!analyzeSession(event.sessionId,',
  'if (duplicateFound)',
  'return JournalSaveResult::Duplicate;',
  'return analyzeSession(sessionId,',
  'bool stateValid = true;',
  'bool duplicateSeen = false;',
  'if (duplicateSeen)',
  'if (expectedContext != nullptr && schemaVersion == 2UL)',
  'targetRunStartFound = true;',
  'state.journalConsistent = true;'
]) {
  requireText(runtimeSource, text, 'runtime journal single-pass contract');
}
const journalOpenCount = (runtimeSource.match(/m_fileSystem\.open\(JournalPath, FILE_READ\)/g) || []).length;
if (journalOpenCount !== 3) {
  throw new Error(`Expected exactly 3 JournalPath FILE_READ sites (boot structure, boot context, runtime analyzer), got ${journalOpenCount}`);
}

console.log('Winding persistence single-pass contracts OK: persistence audit shares one scan, and runtime save/session-state evidence now shares one bounded streamed journal analysis pass.');
