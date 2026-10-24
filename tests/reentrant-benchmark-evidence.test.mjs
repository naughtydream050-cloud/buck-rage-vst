import assert from 'node:assert/strict';
import { validateReentrantBenchmarkEvidence } from '../scripts/reentrant-benchmark-evidence.mjs';

const rates = [44100, 48000, 96000];
const modes = [64, 128, 256, 512, 1024, -1];
const rows = rates.flatMap(rate => modes.map(mode => {
  const samples = rate * 30;
  const events = Math.floor(samples * 0.99609);
  return `METRIC reentrant_benchmark rate=${rate} mode=${mode} p99_deadline_ratio=0.25 max_deadline_ratio=0.75 max_wall_thread_cycles=100000 max_wall_sample=100000 max_wall_writer_index=1000 avg_thread_cpu_to_callback_wall=0.10 events=${events} samples=${samples} event_rate=${(events / samples).toFixed(6)} callback_shape=one_host_block status=PASS`;
}));
const valid = `METRIC benchmark_schedule class=MMCSS-Pro-Audio process_priority=Normal host_cadence=sample_clock pitch_semitones=12\n${rows.join('\n')}\nMETRIC reentrant_benchmark_complete conditions=18 status=PASS`;
assert.deepEqual(validateReentrantBenchmarkEvidence(valid), { pass: true, reason: null, rows: 18 });

const rejects = [
  [valid.replace('process_priority=Normal', 'process_priority=High'), 'MMCSS_PRO_AUDIO_NOT_RECORDED'],
  [valid.replace(' host_cadence=sample_clock', ''), 'MMCSS_PRO_AUDIO_NOT_RECORDED'],
  [valid.replace('pitch_semitones=12', 'pitch_semitones=0'), 'MMCSS_PRO_AUDIO_NOT_RECORDED'],
  [valid.replace('max_deadline_ratio=0.75', 'max_deadline_ratio=0.81'), 'REALTIME_DEADLINE_EXCEEDED'],
  [valid.replace('p99_deadline_ratio=0.25', 'p99_deadline_ratio=0.51'), 'REALTIME_DEADLINE_EXCEEDED'],
  [valid.replace('status=PASS', 'status=FAIL'), 'REALTIME_DEADLINE_EXCEEDED'],
  [valid.replace(rows[0], ''), 'BENCHMARK_MATRIX_INCOMPLETE'],
  [valid.replace(rows[0], rows[1]), 'BENCHMARK_MATRIX_INCOMPLETE'],
  [valid.replace(/event_rate=[0-9.]+/, 'event_rate=0.50'), 'INVALID_BENCHMARK_METRICS'],
  [valid.replace(/samples=\d+/, 'samples=1'), 'INVALID_BENCHMARK_METRICS'],
  [valid.replace('max_wall_thread_cycles=100000', 'max_wall_thread_cycles=-1'), 'MALFORMED_BENCHMARK_ROW'],
  [valid.replace('callback_shape=one_host_block', 'unrecognized=1 callback_shape=one_host_block'), 'MALFORMED_BENCHMARK_ROW'],
  [valid.replace('conditions=18 status=PASS', 'conditions=18 status=FAIL'), 'BENCHMARK_SUMMARY_NOT_PASS']
];
for (const [input, reason] of rejects)
  assert.equal(validateReentrantBenchmarkEvidence(input).reason, reason);
console.log(`PASS reentrant-benchmark-evidence-mutations rejected=${rejects.length}`);
