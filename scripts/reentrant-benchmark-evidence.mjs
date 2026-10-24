const expectedRates = [44100, 48000, 96000];
const expectedModes = [64, 128, 256, 512, 1024, -1];

export function validateReentrantBenchmarkEvidence(text) {
  if (typeof text !== 'string'
      || !text.includes('METRIC benchmark_schedule class=MMCSS-Pro-Audio process_priority=Normal host_cadence=sample_clock pitch_semitones=12'))
    return { pass: false, reason: 'MMCSS_PRO_AUDIO_NOT_RECORDED' };

  const rows = [];
  const pattern = /^METRIC reentrant_benchmark rate=(\d+) mode=(-?\d+) p99_deadline_ratio=([0-9.eE+-]+) max_deadline_ratio=([0-9.eE+-]+) max_wall_thread_cycles=(\d+) max_wall_sample=(\d+) max_wall_writer_index=(\d+) avg_thread_cpu_to_callback_wall=([0-9.eE+-]+) events=(\d+) samples=(\d+) event_rate=([0-9.eE+-]+) callback_shape=one_host_block status=(PASS|FAIL)$/;
  for (const line of text.split(/\r?\n/)) {
    if (!line.startsWith('METRIC reentrant_benchmark rate=')) continue;
    const match = line.match(pattern);
    if (!match) return { pass: false, reason: 'MALFORMED_BENCHMARK_ROW' };
    const [, rateText, modeText, p99Text, maxText, cyclesText, maxSampleText, writerText, cpuAverageText, eventsText, samplesText, eventRateText, status] = match;
    const row = {
      rate: Number(rateText), mode: Number(modeText), p99: Number(p99Text), max: Number(maxText),
      cpuAverage: Number(cpuAverageText),
      maxCycles: Number(cyclesText), maxSample: Number(maxSampleText), writerIndex: Number(writerText),
      events: Number(eventsText), samples: Number(samplesText), eventRate: Number(eventRateText), status
    };
    if (![row.rate, row.mode, row.p99, row.max, row.cpuAverage,
          row.events, row.samples, row.eventRate, row.maxCycles, row.maxSample, row.writerIndex].every(Number.isFinite)
        || row.cpuAverage < 0 || row.p99 < 0 || row.max < row.p99
        || row.maxCycles < 0 || row.maxSample < 0 || row.writerIndex < 0
        || row.samples < row.rate * 30 || row.events < 0 || row.events > row.samples
        || Math.abs(row.eventRate - row.events / row.samples) > 1e-6)
      return { pass: false, reason: 'INVALID_BENCHMARK_METRICS' };
    rows.push(row);
  }

  const keys = new Set(rows.map(row => `${row.rate}:${row.mode}`));
  const expectedKeys = new Set(expectedRates.flatMap(rate => expectedModes.map(mode => `${rate}:${mode}`)));
  if (rows.length !== 18 || keys.size !== 18 || [...expectedKeys].some(key => !keys.has(key)))
    return { pass: false, reason: 'BENCHMARK_MATRIX_INCOMPLETE' };
  if (rows.some(row => row.events / row.samples < 0.99))
    return { pass: false, reason: 'EVENT_RATE_BELOW_STRESS_TARGET' };
  if (rows.some(row => row.p99 > 0.50 || row.max > 0.80 || row.status !== 'PASS'))
    return { pass: false, reason: 'REALTIME_DEADLINE_EXCEEDED' };

  const summaries = text.split(/\r?\n/).filter(line => line.startsWith('METRIC reentrant_benchmark_complete '));
  if (summaries.length !== 1 || summaries[0] !== 'METRIC reentrant_benchmark_complete conditions=18 status=PASS')
    return { pass: false, reason: 'BENCHMARK_SUMMARY_NOT_PASS' };
  return { pass: true, reason: null, rows: rows.length };
}
