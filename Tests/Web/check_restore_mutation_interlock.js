'use strict';

const fs = require('fs');

function read(path) { return fs.readFileSync(path, 'utf8'); }
function requireText(source, text, message) {
  if (!source.includes(text)) throw new Error(message || `missing ${text}`);
}
function forbidText(source, text, message) {
  if (source.includes(text)) throw new Error(message || `forbidden ${text}`);
}

const interlock = read('firmware/esp32/src/CM_ProductionMutationInterlock.h');
const backupHeader = read('firmware/esp32/src/CM_RemoteBackupWeb.h');
const backup = read('firmware/esp32/src/CM_RemoteBackupWeb.cpp');
const backupActivity = read('firmware/esp32/src/CM_BackupActivityGuard.cpp');

// One handler must sit in front of all subsequently registered API routes. It is
// inert during normal operation and only intercepts mutating API methods while
// restore apply/rollback owns the global production mutation lock.
requireText(interlock, 'class ProductionMutationInterlockHandler : public RequestHandler',
  'global restore mutation handler missing');
requireText(interlock, 'ProductionMutationInterlock::active()',
  'handler must be controlled by the authoritative restore lock');
requireText(interlock, 'requestMethod != HTTP_GET',
  'GET/read-only requests must remain available during restore mutation');
requireText(interlock, 'requestUri.startsWith("/api/")',
  'interlock must cover all API mutation modules, not only JOB endpoints');
requireText(interlock, 'restore_mutation_active',
  'blocked mutations need one stable 409 error contract');
requireText(interlock, 'server.addHandler(new ProductionMutationInterlockHandler())',
  'interlock handler must register with WebServer');
forbidText(interlock, 'SSR', 'restore interlock must not gain SSR authority');
forbidText(interlock, 'RUN_COMPLETED', 'restore interlock must not create run/writeoff semantics');

// Registration is a RemoteBackupWeb member initialized from m_server during the
// global RemoteBackupWeb construction, before configureWebServer() later registers
// ordinary application routes.
requireText(backupHeader, '#include "CM_ProductionMutationInterlock.h"',
  'RemoteBackupWeb must own the global interlock registration');
requireText(backupHeader,
  'ProductionMutationInterlockRegistration m_productionInterlockRegistration{m_server};',
  'interlock registration must happen as part of RemoteBackupWeb construction');

// Every APPLY/forward/rollback state owns the lock. Successful APPLY and FAILED
// stay fail-closed until reboot/cleanup because RAM stores reflect pre-restore
// files. Only a complete rollback or explicit Idle cleanup releases it.
requireText(backupHeader, 'class ApplyStageState',
  'apply state must centrally own lock transitions');
requireText(backupHeader,
  'if (value == ApplyStage::Idle || value == ApplyStage::RolledBack)',
  'only idle or proven rollback completion may release the lock');
requireText(backupHeader, 'ProductionMutationInterlock::release();',
  'apply state must release the interlock on safe terminal states');
requireText(backupHeader, 'ProductionMutationInterlock::acquire();',
  'apply state must acquire/retain the interlock for apply and failure states');
requireText(backupHeader, 'ApplyStageState m_applyStage;',
  'raw ApplyStage storage must not bypass lock ownership');

// Current restore owner already rechecks the full fail-closed activity guard on
// each forward apply entry/copy/verification step. Losing Safe causes update() to
// stop forward progress and enter beginApplyRollback().
for (const text of [
  'BackupActivityGuard::check(m_storage) != BackupActivityCheck::Safe',
  'beginApplyRollback("restore_apply_entry_failed")',
  'beginApplyRollback("restore_apply_copy_failed")',
  'beginApplyRollback("restore_apply_verification_failed")'
]) {
  requireText(backup, text, 'restore runtime recheck/rollback contract missing: ' + text);
}

// Checkpoint 121: an unfinished exact-spool RUN_WIRE ISSUE is a global stock
// transaction boundary. Backup/restore must stay fail-closed until its durable
// pending (or temp intent) has been recovered and cleared.
requireText(backupActivity, '#include "CM_RunWireIssuePendingStore.h"',
  'backup activity guard must know the RUN_WIRE pending store');
requireText(backupActivity, 'storage.exists(RunWireIssuePendingStore::Path)',
  'backup must block on authoritative RUN_WIRE pending');
requireText(backupActivity, 'storage.exists(RunWireIssuePendingStore::TempPath)',
  'backup must block on RUN_WIRE pending temp recovery residue');
requireText(backupActivity, 'return BackupActivityCheck::Busy;',
  'RUN_WIRE recovery residue must fail closed as backup busy');

console.log('Restore mutation interlock contracts: OK');
