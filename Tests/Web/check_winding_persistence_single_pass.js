const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const persistencePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingPersistenceIntegrityAudit.cpp');
const transitionHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournalTransitionAudit.h');
const transitionSourcePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WindingJournalTransitionAudit.cpp');

const persistence = fs.readFileSync(persistencePath, 'utf8');
const transitionHeader = fs.readFileSync(transitionHeaderPath, 'utf8');
const transitionSource = fs.readFileSync(transitionSourcePath, 'utf8');

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

console.log('Winding persistence single-pass contracts OK: schema, transition integrity, and record count share one journal scan.');
