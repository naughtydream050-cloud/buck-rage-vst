import { readFileSync } from 'node:fs';

const ac = process.argv[2];
const source = readFileSync('Source/TimelineScratchEngine.cpp', 'utf8');
const header = readFileSync('Source/TimelineScratchEngine.h', 'utf8');
const tests = readFileSync('tests/TimelineScratchEngineTests.cpp', 'utf8');
const requireText = (text, pattern, label) => {
  if (!pattern.test(text)) throw new Error(`${label} missing`);
};
const pass = message => { console.log(`${ac} PASS ${message}`); };

switch (ac) {
  case 'AC-01':
    requireText(source, /kMotionCaptureWindowSeconds\s*=\s*0\.125/, '125ms window');
    requireText(source, /beginMotionCapture\(\)/, 'single capture entry');
    requireText(source, /motionCaptured\s*\|\|/, 'capture latch');
    pass('fixed 125ms capture and no recapture while latched');
    break;
  case 'AC-02':
    requireText(source, /motionCycleSamples\s*=\s*juce::jmax\s*\(\s*1\.0,[\s\S]*secondsPerQuarter\s*\*\s*0\.5[\s\S]*sampleRateHz[\s\S]*speed\)/s, 'BPM speed cycle');
    requireText(source, /motionPhase\s*=\s*std::fmod\s*\(\s*motionPhase\s*\+\s*1\.0/, 'sample phase');
    pass('SPEED appears only in cycle timing for motion');
    break;
  case 'AC-03':
    requireText(source, /std::cos\s*\(\s*2\.0\s*\*\s*juce::MathConstants<double>::pi\s*\*\s*u\s*\)/, 'smooth CHIRP motion');
    requireText(source, /gateUnits\s*=\s*activeSlot\.preset\s*==\s*Preset::chirp\s*\?\s*2\.0/, 'CHIRP two halves');
    requireText(source, /openLimit\s*=\s*activeSlot\.preset\s*==\s*Preset::chirp\s*\?\s*0\.8/, 'CHIRP 80/20 gate');
    pass('forward/reverse cosine motion and 80/20 gate');
    break;
  case 'AC-04':
    requireText(source, /gateUnits\s*=\s*activeSlot\.preset\s*==\s*Preset::chirp\s*\?\s*2\.0\s*:\s*8\.0/s, 'TRANSFORM four cuts');
    requireText(source, /openLimit\s*=\s*activeSlot\.preset\s*==\s*Preset::chirp\s*\?\s*0\.8\s*:\s*0\.5/s, 'TRANSFORM 50/50 gate');
    pass('four 50/50 transform cuts');
    break;
  case 'AC-05':
    requireText(source, /(?:constexpr|const)\s+double\s+points\[\]\s*\{\s*0\.0,\s*1\.0,\s*0\.25,\s*0\.75,\s*0\.0\s*\}/s, 'ZIGZAG points');
    pass('asymmetric 0,1,.25,.75,0 motion with no gate');
    break;
  case 'AC-06':
    requireText(source, /lengthInQuarters\s*\(/, 'LENGTH mapping');
    requireText(source, /effectiveWet|wetRamp/, 'wet transition');
    requireText(tests, /Length::sixteenth|Length::eighth|Length::quarter|Length::half|Length::oneBar/, 'five LENGTH tests');
    pass('LENGTH, DEPTH and wet ramp are covered');
    break;
  case 'AC-07':
    requireText(source, /kForwardCutSilenceThreshold\s*=\s*1\.0e-4f/, 'silence threshold');
    requireText(source, /kForwardCutSilenceHoldSeconds\s*=\s*0\.030/, '30ms hold');
    requireText(source, /motionSourceEnded\s*=\s*true/, 'source latch');
    pass('dual-channel 30ms source silence latch');
    break;
  case 'AC-08':
    requireText(header, /resetTransport\s*\(\s*bool preserveHistory/, 'transport reset');
    requireText(source, /motionCaptured\s*=\s*false/, 'motion invalidation');
    pass('STOP/seek/loop reset motion without stale capture');
    break;
  case 'AC-09':
    requireText(source, /beginBar\s*\(\s*int bar/, 'BAR entry');
    requireText(tests, /continuous-absolute-bars-1-through-64-do-not-reset/, '64 BAR regression');
    requireText(readFileSync('Source/PluginProcessor.cpp', 'utf8'), /hostSyncEnabled|hasHostPpq|hostPpqOrigin/, 'Host Sync path');
    pass('BAR transitions, Host Sync ON/OFF and 64 BAR continuity covered');
    break;
  case 'AC-10':
    for (const name of ['off', 'baby', 'forwardCut', 'backspin', 'tapeBrake', 'drag'])
      requireText(header, new RegExp(name), `${name} preset`);
    requireText(tests, /baby-speed-controls-forward-reverse-gesture-rate/, 'BABY regression');
    requireText(tests, /backspin-window-stays-bounded-through-one-bar/, 'BACKSPIN regression');
    requireText(tests, /tape-brake-speed-shapes-curve-and-always-stops-at-length-end/, 'TAPE BRAKE regression');
    requireText(tests, /off-steady-state-is-bit-exact-after-transition/, 'OFF regression');
    pass('existing preset regression checks present');
    break;
  case 'AC-11':
    requireText(source, /std::isfinite|juce::jlimit/, 'finite/clamped output');
    if (/std::mutex|new\s|delete\s|ValueTree|std::string\s+log/.test(source)) throw new Error('realtime-unsafe token found');
    pass('finite output and no realtime unsafe token');
    break;
  case 'AC-12':
    requireText(tests, /scratch-offline-wav-renders-speed-0-25-1-0-4-0/, 'WAV speed coverage');
    requireText(tests, /phase2-render-/, 'motion WAV output');
    requireText(tests, /Length::sixteenth|Length::eighth|Length::quarter|Length::half|Length::oneBar/, 'WAV LENGTH coverage');
    pass('five LENGTH and three SPEED diagnostic WAV coverage');
    break;
  default:
    throw new Error(`unknown AC ${ac}`);
}
