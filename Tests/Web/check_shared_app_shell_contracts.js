'use strict';

require('./check_client_crm_ui.js');
require('./check_crud_page_separation.js');

const fs=require('fs');
const path=require('path');

function read(file){return fs.readFileSync(file,'utf8')}
function requireText(source,text,message){if(!source.includes(text))throw new Error(message||`missing ${text}`)}
function forbidText(source,text,message){if(source.includes(text))throw new Error(message||`forbidden ${text}`)}
function htmlFiles(dir){return fs.readdirSync(dir).filter(name=>name.endsWith('.html')).sort()}

const shell=read('firmware/esp32/web/shared/app-shell.js');
const server=read('firmware/esp32/src/CM_StaticSiteServer.cpp');
const buildIdentity=read('scripts/platformio_build_id.py');
const registrySearch=read('firmware/esp32/src/CM_RepairRegistrySearch.cpp');
const lookupWeb=read('firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp');
const desktopFtp=read('firmware/esp32/web/desktop/settings-ftp.html');
const mobileFtp=read('firmware/esp32/web/mobile/settings-ftp.html');
const desktopDir='firmware/esp32/web/desktop';
const mobileDir='firmware/esp32/web/mobile';
const desktopPages=htmlFiles(desktopDir);
const mobilePages=htmlFiles(mobileDir);
const desktopSet=new Set(desktopPages);
const mobileSet=new Set(mobilePages);

new Function(shell);

requireText(server,"appShell.src='/shared/app-shell.js'",'StaticSiteServer must inject the shared shell into variant HTML');
requireText(server,'isUiVariantHtml(m_server.uri(), path)','Shared shell injection must stay scoped to desktop/mobile HTML');
requireText(server,'/api/system/build','Firmware build identity endpoint must be registered');
requireText(server,'CM_FIRMWARE_GIT_SHA','Build identity must use PlatformIO git SHA macro');
requireText(server,'CM_FIRMWARE_GIT_BRANCH','Build identity must expose the compiled branch');
requireText(server,'CM_FIRMWARE_BUILD_UTC','Build identity must expose firmware build time');

for(const text of [
  'CM_BuildIdentityGenerated.h',
  '#define CM_FIRMWARE_GIT_SHA',
  '#define CM_FIRMWARE_GIT_BRANCH',
  '#define CM_FIRMWARE_BUILD_UTC',
  'env.Append(CPPPATH=[build_dir])',
  'env.Append(CCFLAGS=["-include", header_name])'
]) requireText(buildIdentity,text,'Build identity generated-header contract missing: '+text);
forbidText(buildIdentity,'env.Append(CPPDEFINES=','Build identity strings must not use fragile CPPDEFINES quoting');

requireText(shell,'canonicalizeDesktopNav','Shared shell must normalize every desktop sidebar');
requireText(shell,'canonicalizeMobileNav','Shared shell must normalize every mobile bottom navigation');
requireText(shell,"aside=document.createElement('aside')",'Standalone desktop pages must receive a generated sidebar');
requireText(shell,"aside.id='cm-generated-desktop-nav'",'Generated desktop sidebar must have a stable owner id');
requireText(shell,"nav=document.createElement('nav')",'Standalone mobile pages must receive generated bottom navigation');
requireText(shell,"nav.id='cm-generated-mobile-nav'",'Generated mobile navigation must have a stable owner id');
requireText(shell,"['💵','Касса','cash.html']",'Canonical sections must include Cash');
requireText(shell,"['📚','Справочник',`/sites/reference/${uiMode}/`]",'Shared shell must link the current UI mode to the winding reference');
requireText(shell,"['☰','Ещё','more.html']",'Mobile primary navigation must retain the More entry');
requireText(shell,"const fallbackNav=nativeNavigation?'':",'Shared top section navigation must be fallback-only when native/generated navigation exists');
requireText(shell,'aria-current="page"','Shared navigation must expose the active section');
requireText(shell,"suffix==='/more.html'&&targetMode==='desktop'",'Mobile More must switch to the real desktop root rather than a missing desktop more page');
requireText(shell,'data-cm-mode-switch','Mode switch ownership must be centralized in the shared shell');
forbidText(shell,'Статистика','Obsolete Statistics menu item must not return to canonical navigation');

requireText(shell,'cm-shell-breadcrumbs','Shared shell must provide breadcrumbs');
requireText(shell,'cm-recent-items-v1','Shared shell must retain bounded recent items');
requireText(shell,'cm-toast-region','Shared shell must expose toast feedback');
requireText(shell,"fetchJson('/api/system/time')",'Global clock must use device time');
requireText(shell,'45000','Clock must resync infrequently rather than polling every second');
requireText(shell,"fetchJson('/api/system/build')",'Shell must display firmware/web build identity');
requireText(shell,"fetchJson('/web-bundle-manifest.json')",'Shell must load deployed SD web provenance');
requireText(shell,'bundle.schema_version===1','Shell must validate the SD bundle manifest schema');
requireText(shell,'SD web unknown','Shell must fail soft when an old SD bundle has no manifest');
requireText(shell,"'/api/motors?'",'Global search must query motors');
requireText(shell,"'/api/search/clients?'",'Global search must query clients by name/phone/id');
requireText(shell,"'/api/search/repairs?'",'Global search must query repair records rather than use a decorative fallback');
requireText(shell,'motor-details.html?motor_id=','Global search must open motor cards by ID');
requireText(shell,'client-details.html?client_id=','Global search must open the canonical client card by ID');
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

forbidText(shell,"method:'POST'",'Shared shell must remain read-only');
forbidText(shell,'SSR','Shared shell must not own SSR controls');
forbidText(shell,'/api/jobs','Shared shell must not create/cancel winding jobs');

if(!desktopFtp.includes('<title>')||!mobileFtp.includes('<title>'))throw new Error('FTP settings pages must remain valid HTML pages for centralized shell injection');

for(const file of desktopPages){
  if(!mobileSet.has(file))throw new Error(`Missing mobile route parity for desktop/${file}`);
}
for(const file of mobilePages){
  if(file==='more.html')continue;
  if(!desktopSet.has(file))throw new Error(`Missing desktop route parity for mobile/${file}`);
}
if(!mobileSet.has('cash.html'))throw new Error('Mobile Cash feature page is required');

function validateVariantHref(mode,file,href){
  if(!href.startsWith(`/${mode}/`))return;
  const clean=href.split(/[?#]/,1)[0];
  if(clean===`/${mode}/`)return;
  const target=clean.slice(mode.length+2);
  if(!target.endsWith('.html'))return;
  const set=mode==='desktop'?desktopSet:mobileSet;
  if(!set.has(target))throw new Error(`${mode}/${file} links to missing ${clean}`);
}

for(const [mode,dir,pages] of [['desktop',desktopDir,desktopPages],['mobile',mobileDir,mobilePages]]){
  for(const file of pages){
    const source=read(path.join(dir,file));
    if(!/<title>[^<]+<\/title>/i.test(source))throw new Error(`${mode}/${file} has no usable title`);
    if(!/<main[\s>]/i.test(source))throw new Error(`${mode}/${file} has no main content host for shared shell`);
    const hrefs=[...source.matchAll(/href=["']([^"']+)["']/gi)].map(match=>match[1]);
    for(const href of hrefs){
      validateVariantHref('desktop',file,href);
      validateVariantHref('mobile',file,href);
    }
  }
}

const more=read(path.join(mobileDir,'more.html'));
for(const required of [
  'href="/mobile/repairs.html"','>🧰<','href="/mobile/clients.html"','>👤<',
  'href="/mobile/motors.html"','>📊<','href="/mobile/calculator.html"','>🧮<',
  'href="/mobile/warehouse.html"','>📦<','href="/mobile/costing.html"','>💰<',
  'href="/mobile/cash.html"','>💵<','href="/mobile/reports.html"','>📈<',
  'href="/mobile/backup.html"','>💾<','href="/mobile/settings.html"','>⚙️<',
  'href="/sites/reference/mobile/"','>📚<'
]) requireText(more,required,'Mobile More navigation missing canonical item/icon: '+required);
forbidText(more,'Статистика','Mobile More must not expose obsolete Statistics');
forbidText(more,'href="/desktop/more.html"','Mobile More must never link to a nonexistent desktop More page');

console.log(`Shared app shell contracts: OK (${desktopPages.length} desktop + ${mobilePages.length} mobile HTML pages audited)`);
