'use strict';

const fs = require('fs');

const archiveHeader = fs.readFileSync('firmware/esp32/src/CM_AutonomousWindingArchive.h', 'utf8');
const projection = fs.readFileSync('firmware/esp32/src/CM_AutonomousWindingProjection.cpp', 'utf8');
const web = fs.readFileSync('firmware/esp32/src/CM_AutonomousWindingWeb.cpp', 'utf8');

function requireText(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`${label}: missing ${needle}`);
}

requireText(archiveHeader, 'RoleOccupied', 'occupied canonical role result');
requireText(archiveHeader, 'StartingRequiresWorking', 'starting-only fail-closed result');
requireText(archiveHeader, 'ProjectionFailed', 'projection failure result');
requireText(archiveHeader, 'bool replaceExisting', 'explicit replacement argument');

requireText(projection, 'findAutonomousProjection(sessionId', 'session/run/role retry lookup');
requireText(projection, 'projectedMotorId == motorId', 'retry motor identity guard');
requireText(projection, 'if (!latestFound && role == "STARTING")', 'starting requires working');
requireText(projection, 'targetOccupied && !replaceExisting', 'silent overwrite rejection');
requireText(projection, 'next.working = latest.working;', 'working role preservation');
requireText(projection, 'next.starting = latest.starting;', 'starting role preservation');
requireText(projection, 'next.previousVersionId = latestVersionId;', 'append-only predecessor link');
requireText(projection, 'sourceAutonomousSessionId = sessionId', 'source session provenance');
requireText(projection, 'sourceAutonomousRunId = runId', 'source run provenance');
requireText(projection, 'sourceAutonomousRole = role', 'source role provenance');
requireText(projection, 'ensureCanonicalProjection(', 'canonical-first projection helper');
requireText(projection, 'if (assignmentId != 0UL) return AutonomousWindingAssignResult::Assigned;', 'assignment-only historical backfill retry');

requireText(web, 'role != "WORKING" && role != "STARTING"', 'canonical role allowlist');
if (web.includes('role != "WORKING" && role != "STARTING" && role != "AUXILIARY"')) {
  throw new Error('AUXILIARY must not be accepted by motor-card assignment endpoints');
}
requireText(web, 'replace_existing', 'explicit replacement request flag');
requireText(web, 'motor_winding_role_occupied', 'occupied-role HTTP conflict');
requireText(web, 'starting_requires_existing_working_winding', 'starting-only HTTP conflict');
requireText(web, 'motor_winding_projection_failed', 'projection failure HTTP error');

console.log('Autonomous winding motor projection contracts OK');
