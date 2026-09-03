'use strict';

const fs=require('fs');

function read(path){return fs.readFileSync(path,'utf8')}
function requireText(source,text,message){if(!source.includes(text))throw new Error(message||`missing ${text}`)}
function forbidText(source,text,message){if(source.includes(text))throw new Error(message||`forbidden ${text}`)}

const desktop=read('firmware/esp32/web/desktop/client-new.html');
const mobile=read('firmware/esp32/web/mobile/client-new.html');
const backend=read('firmware/esp32/src/CM_RepairRegistryWeb.cpp');

for(const [mode,source] of [['desktop',desktop],['mobile',mobile]]){
  requireText(source,'name="name"',`${mode}: name field missing`);
  requireText(source,'name="phone"',`${mode}: phone field missing`);
  requireText(source,'name="comment"',`${mode}: comment field missing`);
  requireText(source,'name="name" maxlength="96"',`${mode}: name length contract missing`);
  requireText(source,'name="phone" maxlength="48"',`${mode}: phone length contract missing`);
  requireText(source,'name="comment" maxlength="320"',`${mode}: comment length contract missing`);
  requireText(source,'required',`${mode}: required fields must stay required`);
  requireText(source,'Обязательные данные',`${mode}: required section missing`);
  requireText(source,'Дополнительно',`${mode}: optional section missing`);
  requireText(source,"phone.replace(/\\D/g,'').length<7",`${mode}: client-side phone validation missing`);
  requireText(source,"fetch('/api/clients',{method:'POST'",`${mode}: canonical create endpoint missing`);
  forbidText(source,'name="comment" maxlength="320" required',`${mode}: comment must remain optional`);
}

for(const source of [desktop,mobile]){
  const nameTag=(source.match(/<input[^>]*name="name"[^>]*>/)||[])[0]||'';
  const phoneTag=(source.match(/<input[^>]*name="phone"[^>]*>/)||[])[0]||'';
  const commentTag=(source.match(/<textarea[^>]*name="comment"[^>]*>/)||[])[0]||'';
  if(!/\brequired\b/.test(nameTag))throw new Error('name must be required');
  if(!/\brequired\b/.test(phoneTag))throw new Error('phone must be required');
  if(/\brequired\b/.test(commentTag))throw new Error('comment must stay optional');
}

requireText(backend,'!m_server.hasArg("name") || !m_server.hasArg("phone")','Backend must require name and phone');
requireText(backend,'RepairRegistry::normalizePhone(m_server.arg("phone")).length() < 7U','Backend phone minimum must remain authoritative');
requireText(backend,'client.comment = m_server.arg("comment")','Backend must preserve optional comment');

console.log('Client creation UI contracts: OK');
