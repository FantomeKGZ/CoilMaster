'use strict';

const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_JobStateStore.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_JobStateStore.cpp', 'utf8');

function requireText(text, message) {
  if (!source.includes(text)) throw new Error(message || `missing ${text}`);
}
function forbidText(text, message) {
  if (source.includes(text)) throw new Error(message || `forbidden ${text}`);
}

if (!header.includes('String backupPath(uint32_t sessionId) const;'))
  throw new Error('JobStateStore must own a per-session rollback backup path');

requireText('F(".bak")', 'job state backup suffix missing');
requireText('if (m_fileSystem.exists(temp) || m_fileSystem.exists(backup)) return false;',
  'interrupted transaction residue must fail closed instead of being erased');
requireText('if (hadTarget && !m_fileSystem.rename(target, backup))',
  'old authoritative job state must rotate to backup before replacement');
requireText('if (!m_fileSystem.rename(temp, target))',
  'verified candidate must commit by rename');
requireText('rollbackRestored = m_fileSystem.rename(backup, target);',
  'failed candidate commit must attempt to restore the old authoritative state');
requireText('if (!load(state.sessionId, committed)',
  'committed target must be re-read and verified before backup cleanup');
requireText('if (hadTarget && m_fileSystem.exists(backup) &&',
  'backup cleanup must happen only after committed target verification');
requireText('m_ready = false;',
  'backup cleanup failure must poison readiness instead of hiding transaction residue');

const rotate = source.indexOf('m_fileSystem.rename(target, backup)');
const commit = source.indexOf('m_fileSystem.rename(temp, target)');
const verify = source.indexOf('if (!load(state.sessionId, committed)');
const cleanup = source.indexOf('m_fileSystem.remove(backup)', verify);
if (rotate < 0 || commit < 0 || verify < 0 || cleanup < 0 ||
    !(rotate < commit && commit < verify && verify < cleanup)) {
  throw new Error('job state atomic replacement ordering must be rotate -> commit -> verify -> cleanup');
}

forbidText('m_fileSystem.exists(target) && !m_fileSystem.remove(target)',
  'job state must never delete the only authoritative target before committing replacement');

console.log('Job state atomic replacement contracts: OK');
