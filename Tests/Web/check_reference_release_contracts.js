const fs = require('fs');

const workflow = fs.readFileSync('.github/workflows/reference-sd-release.yml', 'utf8');

function requireText(needle, message) {
  if (!workflow.includes(needle)) throw new Error(message);
}

function forbidText(needle, message) {
  if (workflow.includes(needle)) throw new Error(message);
}

requireText('workflow_dispatch:', 'SD release must support explicit manual dispatch');
requireText('push:', 'SD release must support an auditable request commit on the source branch');
requireText('- cmp-protocol-v1', 'Release-request push must be limited to the source-of-truth branch');
requireText('- .github/release/reference-sd-request.json',
  'Release-request push must be limited to the single explicit request file');
requireText("jq -r '.source_run_id'", 'Push publication must read an explicit source run ID');
requireText('GITHUB_EVENT_NAME', 'Workflow must distinguish dispatch from request commits');
forbidText('\n  schedule:', 'SD release must never publish on a schedule');
requireText('actions: read', 'SD release needs bounded Actions artifact read permission');
requireText('contents: write', 'SD release needs contents permission to create a GitHub Release');
requireText('cancel-in-progress: false', 'An active immutable release must not be cancelled by another request');
requireText('source run is not completed GREEN', 'Release must reject a non-GREEN source run');
requireText('.github/workflows/reference-legacy-import.yml',
  'Release must accept only Reference Legacy Import Check evidence');
requireText('cmp-protocol-v1', 'Release must require the source-of-truth branch');
requireText('head_repository.full_name', 'Release must reject artifacts from another repository');
requireText('coilmaster-web-sd-bundle-$head_sha',
  'Release must download the artifact bound to the exact source SHA');
requireText('persist-credentials: false', 'Exact source checkout must not persist broad git credentials');
requireText('manifest workflow run ID does not match source run',
  'Release must bind manifest provenance to the selected run');
requireText('tools/build_web_bundle_manifest.py', 'Release must verify the complete payload hash');
requireText('--verify', 'Release must invoke manifest verification mode');
requireText('tools/check_web_bundle_offline.py', 'Release must re-run offline dependency closure');
requireText('sha256sum', 'Release must publish a checksum for the permanent ZIP');
requireText('refusing to overwrite immutable release',
  'Release must fail closed instead of replacing an existing release');
requireText('gh release create', 'Workflow must publish a GitHub Release');
requireText('--target "$SOURCE_HEAD_SHA"', 'Release tag must point to the verified source commit');

console.log('Reference SD release contracts OK: publication is manual, GREEN-source-bound, reverified and immutable.');
