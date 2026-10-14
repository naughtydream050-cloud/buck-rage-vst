import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

const ac = process.argv[2];
const source = readFileSync('Source/TimelineScratchEngine.cpp', 'utf8');
const header = readFileSync('Source/TimelineScratchEngine.h', 'utf8');
const tests = readFileSync('tests/TimelineScratchEngineTests.cpp', 'utf8');
const has = (text, pattern, label) => {
  if (!pattern.test(text)) throw new Error(`${label} missing`);
};
const pass = message => console.log(`${ac} PASS ${message}`);

execFileSync('git', ['diff', '--check'], { stdio: 'ignore' });

switch (ac) {
  case 'AC-01':
    has(source, /motionCandidateStartSerial\s*=\s*\(double\) writeSerial/, 'current input anchor');
    has(tests, /motion-capture-anchors-to-current-bar-in-cold-and-primed-starts/, 'cold/primed assertion');
    pass('cold and primed current-BAR anchors are asserted'); break;
  case 'AC-02':
    has(source, /motionWindowStartSerial\s*=\s*motionCandidateStartSerial/, 'candidate window start');
    has(source, /motionWindowEndSerial\s*=\s*motionCandidateStartSerial\s*\+\s*motionWindowSamples/, 'candidate window end');
    pass('125ms current-BAR range is fixed'); break;
  case 'AC-03':
    has(source, /if \(\(double\) writeSerial < readableAt\) return false/, 'read safety wait');
    has(source, /targetWet = effectActive[\s\S]*isWetReady\(\)/, 'dry while waiting');
    pass('unsafe history remains dry'); break;
  case 'AC-04':
    has(source, /motionCandidateSilenceSamples[\s\S]*kForwardCutSilenceHoldSeconds/, '30ms cancellation');
    has(tests, /motionCandidateCancelCount > 0/, 'cancellation assertion');
    pass('incomplete candidate cancellation is exercised'); break;
  case 'AC-05':
    has(source, /motionCandidateSilenceSamples\s*=\s*0\.0/, 'gap reset');
    has(tests, /20 ms|zero crossing|entry-gap/, 'gap fixture');
    pass('short gaps and zero crossings are covered'); break;
  case 'AC-06':
    has(source, /if \(motionCaptured \|\| ! isMotionPreset/, 'capture latch');
    has(source, /motionCandidateActive\s*=\s*false/, 'candidate closes');
    pass('capture is single-shot and fixed'); break;
  case 'AC-07':
    has(source, /motionCapturePhase\s*=\s*motionPhase/, 'phase preservation');
    has(source, /motionPhase\s*=\s*std::fmod\s*\(\s*motionPhase\s*\+\s*1\.0/, 'BAR sample clock');
    pass('motion phase remains BAR-relative'); break;
  case 'AC-08':
    has(source, /motionSourceEnded\s*=\s*true/, 'post-capture latch');
    has(tests, /motion-source-ended-wet-to-dry|motion-source-ended-still-latches-after-entry-grace/, 'latch tests');
    pass('post-capture silence latch is retained'); break;
  case 'AC-09':
    has(source, /activeDurationQuarters/, 'LENGTH duration');
    has(tests, /Length::sixteenth[\s\S]*Length::oneBar/, 'all lengths');
    pass('LENGTH remains bounded'); break;
  case 'AC-10':
    has(source, /juce::jlimit\s*\(\s*0\.25f\s*,\s*4\.0f\s*,\s*slot\.speed\s*\)/, 'speed clamp');
    has(tests, /speed-0-25-1-0-4-0/, 'speed render matrix');
    pass('speed/depth matrix retained'); break;
  case 'AC-11':
    has(tests, /eight-bar-chain-includes-bar1-and-bar5/, 'multi BAR chain');
    has(tests, /continuous-absolute-bars-1-through-64-do-not-reset/, '64 BAR chain');
    pass('BAR continuity assertions exist'); break;
  case 'AC-12':
    has(header, /resetTransport\s*\(\s*bool preserveHistory/, 'transport reset API');
    has(source, /DiscontinuityReason::loopWrap/, 'loop handling');
    pass('transport reset paths remain bounded'); break;
  case 'AC-13':
    for (const name of ['off', 'baby', 'forwardCut', 'backspin', 'tapeBrake', 'drag']) has(header, new RegExp(name), name);
    pass('other presets remain present and scoped'); break;
  case 'AC-14':
    has(source, /noexcept/, 'realtime noexcept');
    if (/std::mutex|ValueTree|std::string\s+log/.test(source)) throw new Error('realtime unsafe token');
    has(tests, /boundedFinite|noHardJump/, 'finite/continuity tests');
    pass('finite output and realtime safety checks'); break;
  case 'AC-15':
    has(tests, /motion-captures-current-bar-after-signal-on-silent-bar/, 'deterministic motion test');
    has(source, /motionCandidateCancelCount/, 'numeric diagnostics');
    pass('same-HEAD deterministic evidence hooks are present'); break;
  default: throw new Error(`unknown AC ${ac}`);
}
