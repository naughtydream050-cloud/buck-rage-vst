#!/usr/bin/env node
import { createHash } from 'node:crypto';
import { readFileSync, readdirSync, statSync } from 'node:fs';
import { join, relative, resolve } from 'node:path';
import { execFileSync } from 'node:child_process';

const fail = code => { throw new Error(code); };
const sha = data => createHash('sha256').update(data).digest('hex');
const fileSha = path => sha(readFileSync(path));
const evidencePath = process.env.XY_LIVE_R2_EVIDENCE_PATH;
if (!evidencePath) fail('XY_LIVE_R2_EVIDENCE_PATH_REQUIRED');
const evidence = JSON.parse(readFileSync(evidencePath, 'utf8'));
const root = process.cwd();
const sourceHead = execFileSync('git', ['rev-parse', 'HEAD'], { cwd: root, encoding: 'utf8' }).trim();
if (execFileSync('git', ['status', '--porcelain'], { cwd: root, encoding: 'utf8' }).trim()) fail('SOURCE_TREE_DIRTY');
if (evidence.schemaVersion !== 1 || evidence.sourceHead !== sourceHead) fail('SOURCE_HEAD_MISMATCH');
if (evidence.ci?.conclusion !== 'success' || evidence.ci?.sourceHead !== sourceHead) fail('CI_SOURCE_NOT_VERIFIED');
if (evidence.ci?.aggregateValidation !== 'success') fail('CI_AGGREGATE_NOT_PASS');

const planPath = join(root, 'docs', 'XY_PAD_LIVE_R2_PLAN.md');
const criteriaPath = join(root, 'docs', 'XY_PAD_LIVE_R2_AC.json');
if (fileSha(planPath).toLowerCase() !== evidence.planSha256?.toLowerCase()) fail('PLAN_HASH_MISMATCH');
if (fileSha(criteriaPath).toLowerCase() !== evidence.criteriaSha256?.toLowerCase()) fail('CRITERIA_HASH_MISMATCH');

function treeSha(path) {
  if (!statSync(path).isDirectory()) return fileSha(path);
  const files = [];
  const walk = (dir, prefix = '') => {
    for (const item of readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
      const name = `${prefix}${item.name}`;
      if (item.isSymbolicLink()) fail('ARTIFACT_SYMLINK_FORBIDDEN');
      if (item.isDirectory()) walk(join(dir, item.name), `${name}/`);
      else if (item.isFile()) files.push(`${name}\0${fileSha(join(dir, item.name))}`);
      else fail('ARTIFACT_SPECIAL_FILE_FORBIDDEN');
    }
  };
  walk(path);
  if (!files.length) fail('ARTIFACT_EMPTY');
  return sha(files.join('\n'));
}

const artifactRoot = resolve(evidence.artifact?.bundlePath ?? '');
const binaryPath = resolve(evidence.artifact?.binaryPath ?? '');
if (!artifactRoot || !binaryPath || treeSha(artifactRoot) !== evidence.artifact?.treeSha256?.toLowerCase())
  fail('ARTIFACT_TREE_SHA_MISMATCH');
if (fileSha(binaryPath).toLowerCase() !== evidence.artifact?.binarySha256?.toLowerCase())
  fail('ARTIFACT_BINARY_SHA_MISMATCH');
if (evidence.artifact?.binaryMachine !== 'AMD64' || evidence.artifact?.sourceHead !== sourceHead)
  fail('ARTIFACT_ARCH_OR_SOURCE_MISMATCH');
const sourceHeadText = readFileSync(evidence.artifact.sourceHeadPath, 'utf8');
if (!sourceHeadText.split(/\r?\n/).some(line => line === `SOURCE_HEAD=${sourceHead}`)) fail('ARTIFACT_SOURCE_HEAD_FILE_MISMATCH');

const expectedExitMarker = {
  'xy-engine': 'XY_BUILD_EXIT=0 XY_TEST_EXIT=0',
  'scratch-engine': 'SCRATCH_BUILD_EXIT=0 SCRATCH_TEST_EXIT=0',
  'runtime-smoke': 'SMOKE_BUILD_EXIT=0 SMOKE_TEST_EXIT=0',
  'state-model': 'STATE_BUILD_EXIT=0 STATE_TEST_EXIT=0'
};
const logs = {};
for (const [suite, entry] of Object.entries(evidence.logs ?? {})) {
  const path = resolve(entry.path);
  const text = readFileSync(path, 'utf8');
  if (fileSha(path).toLowerCase() !== entry.sha256?.toLowerCase()) fail(`LOG_SHA_MISMATCH_${suite}`);
  const lines = text.split(/\r?\n/);
  const passCount = lines.filter(line => /^PASS\b/.test(line)).length;
  const failCount = lines.filter(line => /^FAIL\b/.test(line)).length;
  if (entry.passCount !== passCount || entry.failCount !== failCount || failCount !== 0) fail(`LOG_ASSERTION_COUNT_MISMATCH_${suite}`);
  if (expectedExitMarker[suite] && !lines.includes(expectedExitMarker[suite])) fail(`TEST_EXIT_NOT_ZERO_${suite}`);
  logs[suite] = { text, lines, passCount, failCount };
}

const acMap = {
  'AC-01': [['state-model', '64-timeline-slots-are-independent'], ['xy-engine', 'all-64-bar-states-ready'], ['xy-engine', 'all-64-bars-replay-their-own-stereo-capture']],
  'AC-02': [['xy-engine', 'first-visit-bar4-live-wet-after-20ms-history-plus-1024-sample-pipeline-and-8ms-ramp']],
  'AC-03': [['xy-engine', 'first-visit-bar4-live-wet-after-20ms-history-plus-1024-sample-pipeline-and-8ms-ramp']],
  'AC-04': [['xy-engine', 'live-grains-read-forward-and-reverse-within-valid-serial-range-and-snapshot-source'], ['xy-engine', 'forward-reverse-linear-read-matches-scalar-reference-and-clamps-endpoints']],
  'AC-05': [['xy-engine', 'live-stationary-xy-freezes-a-bounded-grain-and-holds-it-across-blocks'], ['xy-engine', 'live-fold-jump-starts-transition-on-the-next-sample-without-hop-delay'], ['xy-engine', 'xy-live-r2-rendered-transition-matches-equal-power-reference']],
  'AC-06': [['xy-engine', 'pitch-top-plus-12-semitones'], ['xy-engine', 'pitch-bottom-minus-12-semitones'], ['xy-engine', 'pitch-middle-zero-semitones-preserves-frequency']],
  'AC-07': [['xy-engine', 'pitch-plus-12-is-independent-of-forward-x-slope'], ['xy-engine', 'pitch-minus-12-is-independent-of-reverse-x-slope'], ['xy-engine', 'live-pitch-y-does-not-change-x-direction-or-sample-clock']],
  'AC-08': [['xy-engine', 'live-pitch-neutral-identity-has-exact-1024-sample-delay']],
  'AC-09': [['xy-engine', 'depth-zero-bit-exact-dry'], ['runtime-smoke', 'xy-ready-bypass-excludes-xy-and-matches-preset-only'], ['runtime-smoke', 'xy-depth-zero-is-bit-exact-dry-before-and-after-ready']],
  'AC-10': [['runtime-smoke', 'ac10-cold-history-unavailable-xy-preserves-active-preset-bit-exact-per-sample'], ['runtime-smoke', 'ac10-source-silence-unavailable-xy-preserves-nonzero-preset-tail-per-sample'], ['runtime-smoke', 'ac10-active-and-same-bar-resumed-xy-contribute-in-every-ten-ms-window']],
  'AC-11': [['runtime-smoke', 'confirmed-loop-reuses-ready-bar-at-loop-origin'], ['runtime-smoke', 'v2-host-sync-off-keeps-internal-timeline-origin'], ['xy-engine', 'all-64-bars-replay-their-own-stereo-capture']],
  'AC-12': [['xy-engine', 'stop-keeps-ready-bar-but-does-not-play-stale-xy'], ['xy-engine', 'seek-away-and-return-preserves-completed-bar-capture'], ['xy-engine', 'timing-generation-invalidates-old-render-before-recapture']],
  'AC-13': [['xy-engine', 'stereo-phase-offset-preserved-through-linked-reverse-read'], ['xy-engine', 'live-source-ended-fades-actual-xy-contribution-without-a-hard-audio-cut'], ['xy-engine', 'correlated-layer-peak-measured-no-limiter']],
  'AC-14': [['xy-engine', 'live-xy-processing-performs-no-heap-allocation'], ['xy-engine', 'audio-process-path-performs-no-global-heap-allocation'], ['xy-engine', 'fixed-64-bar-tables-do-not-consume-windows-thread-stack']],
  'AC-15': [['xy-engine', 'XY_BUILD_EXIT=0 XY_TEST_EXIT=0'], ['scratch-engine', 'SCRATCH_BUILD_EXIT=0 SCRATCH_TEST_EXIT=0'], ['runtime-smoke', 'SMOKE_BUILD_EXIT=0 SMOKE_TEST_EXIT=0'], ['state-model', 'STATE_BUILD_EXIT=0 STATE_TEST_EXIT=0']],
  'AC-16': [['gate-tests', 'tests 10'], ['gate-tests', 'pass 10'], ['gate-tests', 'fail 0']]
};

const [kind, id] = process.argv.slice(2);
if (kind === '--ac') {
  const required = acMap[id];
  if (!required) fail('UNKNOWN_AC');
  for (const [suite, assertion] of required) {
    const text = suite === 'gate-tests' ? readFileSync(evidence.gateTestLogPath, 'utf8') : logs[suite]?.text;
    if (!text || !text.split(/\r?\n/).some(line => line.includes(assertion))) fail(`AC_ASSERTION_NOT_EXECUTED_${id}_${suite}`);
  }
  const criterion = JSON.parse(readFileSync(criteriaPath, 'utf8')).criteria.find(item => item.id === id);
  if (!criterion || !Array.isArray(criterion.check)) fail(`AC_NOT_IN_FIXED_MANIFEST_${id}`);
  console.log(`PASS ${id} source=${sourceHead} run=${evidence.ci.runId} artifact=${evidence.artifact.treeSha256}`);
  process.exit(0);
}
if (kind === '--suite') {
  const suites = {
    'xy-engine': ['xy-engine'], 'scratch-engine': ['scratch-engine'],
    'runtime-smoke': ['runtime-smoke'], 'state-model': ['state-model'],
    aggregate: ['xy-engine', 'scratch-engine', 'runtime-smoke', 'state-model']
  }[id];
  if (!suites) fail('UNKNOWN_SUITE');
  if (suites.some(suite => !logs[suite] || logs[suite].failCount !== 0 || logs[suite].passCount <= 0))
    fail(`CI_SUITE_NOT_PASS_${id}`);
  console.log(`PASS suite=${id} source=${sourceHead} run=${evidence.ci.runId}`);
  process.exit(0);
}
fail('USAGE: --ac AC-01 | --suite xy-engine|scratch-engine|runtime-smoke|state-model|aggregate');
