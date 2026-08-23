'use strict';

const fs=require('fs');

function read(path){return fs.readFileSync(path,'utf8')}
function requireText(source,text,message){if(!source.includes(text))throw new Error(message||`missing ${text}`)}
function forbidText(source,text,message){if(source.includes(text))throw new Error(message||`forbidden ${text}`)}

const shell=read('firmware/esp32/web/shared/app-shell.js');
const server=read('firmware/esp32/src/CM_StaticSiteServer.cpp');
const buildIdentity=read('scripts/platformio_build_id.py');
const registrySearch=read('firmware/esp32/src/CM_RepairRegistrySearch.cpp');
const lookupWeb=read('firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp');
const desktopFtp=read('firmware/esp32/web/desktop/settings-ftp.html');
const mobileFtp=read('firmware/esp32/web/mobile/settings-ftp.html');

// Basic JavaScript syntax guard.
new Function(shell);

requireText(server,"appShell.src='/shared/app-shell.js'",'StaticSiteServer must inject the shared shell into variant HTML');
requireText(server,'isUiVariantHtml(m_server.uri(), path)','Shared shell injection must stay scoped to desktop/mobile HTML');
requireText(server,'/api/system/build','Firmware build identity endpoint must be registered');
requireText(server,'CM_FIRMWARE_GIT_SHA','Build identity must use PlatformIO git SHA macro');
requireText(server,'CM_FIRMWARE_GIT_BRANCH','Build identity must expose the compiled branch');
requireText(server,'CM_FIRMWARE_BUILD_UTC','Build identity must expose firmware build time');

// Build metadata contains hyphens, timestamps and hexadecimal text. Passing
// quoted values through CPPDEFINES proved unsafe because SCons/compiler quoting
// can remove the C-string delimiter. Require a generated build-only header.
for(const text of [
  'CM_BuildIdentityGenerated.h',
  '#define CM_FIRMWARE_GIT_SHA',
  '#define CM_FIRMWARE_GIT_BRANCH',
  '#define CM_FIRMWARE_BUILD_UTC',
  'env.Append(CPPPATH=[build_dir])',
  'env.Append(CCFLAGS=["-include", header_name])'
]) requireText(buildIdentity,text,'Build identity generated-header contract missing: '+text);
forbidText(buildIdentity,'env.Append(CPPDEFINES=','Build identity strings must not use fragile CPPDEFINES quoting');

requireText(shell,'cm-shell-nav','Shared shell must provide one navigation layer');
requireText(shell,"['📚','Справочник',`/sites/reference/${uiMode}/`]",'Shared shell must link the current UI mode to the winding reference');
requireText(shell,'aria-current="page"','Shared navigation must expose the active section');
requireText(shell,'cm-shell-breadcrumbs','Shared shell must provide breadcrumbs');
requireText(shell,'cm-recent-items-v1','Shared shell must retain bounded recent items');
requireText(shell,'cm-toast-region','Shared shell must expose toast feedback');
requireText(shell,"fetchJson('/api/system/time')",'Global clock must use device time');
requireText(shell,'45000','Clock must resync infrequently rather than polling every second');
requireText(shell,"fetchJson('/api/system/build')",'Shell must display firmware/web build identity');
requireText(shell,"'/api/motors?'",'Global search must query motors');
requireText(shell,"'/api/search/clients?'",'Global search must query clients by name/phone/id');
requireText(shell,"'/api/search/repairs?'",'Global search must query repair records rather than use a decorative fallback');
requireText(shell,'winding-history.html?repair_id=','Global search must provide repair navigation by ID');
requireText(shell,"placeholder=\"Поиск: двигатель, клиент, ремонт\"",'Global search control must be visible');
requireText(shell,'event.ctrlKey||event.metaKey','Global search keyboard shortcut must support Ctrl/Cmd');
forbidText(shell,'cm_search=${encodeURIComponent(query)}','Global search must not fall back to catalog-only pseudo search');

requireText(lookupWeb,'/api/search/clients','Client global-search endpoint must be registered');
requireText(lookupWeb,'/api/search/repairs','Repair global-search endpoint must be registered');
requireText(lookupWeb,'HTTP_GET','Registry search endpoints must remain read-only');
requireText(registrySearch,'appendClientsSearchPageJson','Client search must use a bounded registry reader');
requireText(registrySearch,'appendRepairsSearchPageJson','Repair search must use a bounded registry reader');
requireText(registrySearch,'count < limit','Client search must stop at the requested page size');
requireText(registrySearch,'batchCount < MaxListPageSize','Repair search must keep bounded temporary batches');
requireText(registrySearch,'resolveRepairPageStatuses','Repair search must preserve authoritative current status decoration');
forbidText(registrySearch,'FILE_APPEND','Global search must never write workshop data');
forbidText(registrySearch,'FILE_WRITE','Global search must never write workshop data');

// This shared layer is read-only/navigation UX. It must never become a physical-control path.
forbidText(shell,"method:'POST'",'Shared shell must remain read-only');
forbidText(shell,'SSR','Shared shell must not own SSR controls');
forbidText(shell,'/api/jobs','Shared shell must not create/cancel winding jobs');

if(!desktopFtp.includes('<title>')||!mobileFtp.includes('<title>'))throw new Error('FTP settings pages must remain valid HTML pages for centralized shell injection');

console.log('Shared app shell contracts: OK');
