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
const staleGuard = read('firmware/esp32/web/shared/backup-restore-stale-guard.js');
const staticSiteServer = read('firmware/esp32/src/CM_StaticSiteServer.cpp');

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

// Backup HTML executes before the runtime UI switch script is appended by the
// live StaticSiteServer. The stale guard therefore must wait for delayed remote
// controls without polling apply-status while the static/offline page remains
// read-only. Once controls appear, the normal fail-closed STALE polling starts.
requireText(staticSiteServer, "if(rest==='/backup.html')",
  'live backup page must retain remote-backup runtime helper injection');
requireText(staticSiteServer, "helper.src='/shared/backup-remote-upload.js';",
  'live backup page must inject the remote-backup operator UI');
requireText(staleGuard, 'function hasRestoreUi()',
  'shared stale guard must detect whether restore UI is present');
requireText(staleGuard, 'function startGuard()',
  'shared stale guard must have one idempotent startup path');
requireText(staleGuard, 'const presenceObserver=new MutationObserver',
  'shared stale guard must wait for delayed runtime restore controls');
requireText(staleGuard, 'presenceObserver.disconnect();',
  'restore control presence observer must stop after the UI appears');
requireText(staleGuard, "fetch('/api/backup/remote/apply-status'",
  'restore pages must retain stale apply evidence polling');
forbidText(staleGuard, 'if(!hasRestoreUi())return;',
  'guard must not permanently exit before runtime remote UI injection');
const startAt = staleGuard.indexOf('function startGuard()');
const pollStartAt = staleGuard.indexOf('checkApplyEvidence();', startAt);
const presenceAt = staleGuard.indexOf('const presenceObserver=new MutationObserver');
const presenceStartAt = staleGuard.indexOf('startGuard();', presenceAt);
if (startAt < 0 || pollStartAt < 0 || presenceAt < 0 || presenceStartAt < 0 ||
    pollStartAt < startAt || presenceStartAt < presenceAt) {
  throw new Error('delayed restore UI must arm stale polling only through startGuard');
}

console.log('Restore mutation interlock contracts: OK');
