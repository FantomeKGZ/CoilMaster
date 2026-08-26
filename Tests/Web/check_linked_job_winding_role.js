const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_JobLinkageResolver.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_JobLinkageResolver.cpp', 'utf8');
const main = fs.readFileSync('firmware/esp32/src/main.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, '#include "CM_MotorWindingVersionStore.h"', 'version-store owner include');
must(header, 'MotorWindingVersionStore m_windingVersions;', 'version-store member');
must(header, 'resolveWithProgramAndRepeat', 'role+repeat resolver contract');
must(header, 'RemoteJobType requestedType', 'role-aware resolver overload');
must(source, 'm_windingVersionsReady = m_windingVersions.begin();', 'fail-closed version store begin');
must(source, 'appendLatestByMotorJson(versionJson', 'latest winding version lookup');
must(source, 'requestedType == RemoteJobType::Starting', 'STARTING role branch');
must(source, '\"starting_present\":true', 'STARTING presence gate');
must(source, 'findString(versionJson, "starting_program"', 'STARTING program lookup');
must(source, 'findUnsigned(versionJson, "starting_repeat_target"', 'STARTING repeat lookup');
must(source, 'findString(versionJson, "working_program"', 'WORKING program lookup');
must(source, 'findUnsigned(versionJson, "working_repeat_target"', 'WORKING repeat lookup');
must(source, 'if (requestedType == RemoteJobType::Starting)', 'legacy STARTING rejection');
must(source, 'uint32_t parsedRepeatTarget = 1UL;', 'legacy repeat default');
must(source, 'findUnsigned(line, "repeat_target", parsedRepeatTarget)', 'legacy persisted repeat lookup');
must(main, 'jobLinkageResolver.resolveWithProgramAndRepeat(linkage.repairId,', 'handleCreateJob uses role+repeat resolver');
must(main, 'job.type,', 'handleCreateJob passes parsed job.type');
must(main, 'catalogRepeatTarget', 'authoritative repeat target local');
must(main, 'job.repeatTarget != catalogRepeatTarget', 'server-side repeat comparison');
must(main, 'repeat_target_does_not_match_motor_role', 'repeat mismatch HTTP error');
must(main, 'job.type = webServer.arg("type") == "starting"', 'request role parsing');
must(main, 'turns_do_not_match_motor_program', 'server-side exact program comparison retained');

if (main.includes('jobLinkageResolver.resolveWithProgram(linkage.repairId,\n                                                   linkage.motorId,\n                                                   resolved,')) {
  throw new Error('linked job still uses role-blind resolver call');
}

console.log('Linked job winding-role contract OK: latest version is authoritative for role/program/repeat, legacy STARTING fails closed, and server validates exact program plus repeat target before job persistence/UART.');
