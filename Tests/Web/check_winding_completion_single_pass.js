const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const completionPath = 'firmware/esp32/src/CM_WindingSessionCompletionAudit.cpp';
const transitionPath = 'firmware/esp32/src/CM_WindingJournalTransitionAudit.cpp';
const queryHeaderPath = 'firmware/esp32/src/CM_WindingJournalQuery.h';
const queryValidationPath = 'firmware/esp32/src/CM_WindingJournalQueryValidation.cpp';

const completion = fs.readFileSync(path.join(root, completionPath), 'utf8');
const transition = fs.readFileSync(path.join(root, transitionPath), 'utf8');
const queryHeader = fs.readFileSync(path.join(root, queryHeaderPath), 'utf8');
const queryValidation = fs.readFileSync(path.join(root, queryValidationPath), 'utf8');
const failures = [];

function requireText(relative, source, text, description) {
  if (!source.includes(text)) failures.push(relative + ': ' + description);
}
function forbidText(relative, source, text, description) {
  if (source.includes(text)) failures.push(relative + ': ' + description);
}

requireText(queryHeaderPath, queryHeader,
  'static bool isValidRecord(const String& line);',
  'authoritative per-record schema validator must be reusable by integrity scans');
requireText(queryValidationPath, queryValidation,
  'bool WindingJournalQuery::isValidRecord(const String& line)',
  'authoritative per-record schema validator implementation missing');
requireText(queryValidationPath, queryValidation,
  '!isValidRecord(line)',
  'validateAll must reuse the same authoritative per-record validator');

requireText(transitionPath, transition,
  '#include "CM_WindingJournalQuery.h"',
  'transition audit must consume the authoritative winding schema validator');
requireText(transitionPath, transition,
  '!WindingJournalQuery::isValidRecord(line)',
  'transition audit must validate the full record schema during its file pass');
requireText(transitionPath, transition,
  'targetRunId == 0UL || runId == targetRunId',
  'exact target-run completion evidence guard missing');
requireText(transitionPath, transition,
  'persistedCompletedRuns != static_cast<uint32_t>(completedRuns) + 1UL',
  'RUN_STARTED/RUN_COMPLETED sequence integrity guard missing');

requireText(completionPath, completion,
  'WindingJournalTransitionAudit::validate(storage, sessionId, runId, completed)',
  'completion audit must rely on the schema-aware transition pass');
forbidText(completionPath, completion,
  'WindingJournalQuery query(',
  'completion audit must not reopen the journal for a separate schema pass');
forbidText(completionPath, completion,
  'const WindingJournalQueryResult schemaAudit',
  'separate validateAll result handling must stay removed');
forbidText(completionPath, completion,
  '#include "CM_WindingJournalQuery.h"',
  'completion audit must not depend directly on the query scanner');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Winding completion single-pass contracts OK: full schema validation and transition/completion evidence share one journal file pass.');
