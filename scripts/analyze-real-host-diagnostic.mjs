import { createReadStream } from 'node:fs';
import { readFile, writeFile } from 'node:fs/promises';
import { resolve, join } from 'node:path';

const folder = resolve(process.argv[2] || '.');
const receipt = JSON.parse(await readFile(join(folder, 'session.json'), 'utf8'));
if (!receipt.valid || receipt.droppedRecords !== 0 || receipt.sampleRecordBytes !== 164) {
  throw new Error('INVALID_DIAGNOSTIC_EVIDENCE: missing frames, I/O failure, drops or incompatible schema');
}
const blocks = (await readFile(join(folder, 'transport-events.jsonl'), 'utf8'))
  .trim().split(/\r?\n/).filter(Boolean).map(line => JSON.parse(line)).filter(x => x.type === 'block');
const groups = new Map();
const capacity = Math.ceil(receipt.sampleRate * 10) + 8;
const stamps = new Float64Array(capacity).fill(-1);
const prefixes = new Float64Array(capacity);
const inputPeaks = new Float32Array(capacity);
let cumulativeInputPower = 0, firstWriter = -1, count = 0, carry = Buffer.alloc(0), nonfinite = 0;
let lastClock = -1, lastWriter = -1;
const seenCaptures = new Set();

for await (const chunk of createReadStream(join(folder, 'samples.bin'))) {
  const bytes = carry.length ? Buffer.concat([carry, chunk]) : chunk;
  const end = bytes.length - bytes.length % 164;
  for (let p = 0; p < end; p += 164) {
    const clock = Number(bytes.readBigUInt64LE(p));
    const writer = Number(bytes.readBigUInt64LE(p + 16));
    const entry = Number(bytes.readBigUInt64LE(p + 24));
    const localBar = bytes.readInt32LE(p + 32), activeBar = bytes.readInt32LE(p + 36);
    const preset = bytes.readInt32LE(p + 40), resetCount = bytes.readUInt32LE(p + 44);
    const resetReason = bytes.readInt32LE(p + 48), flags = bytes.readInt32LE(p + 52);
    const doubles = Array.from({length: 10}, (_, i) => bytes.readDoubleLE(p + 56 + i * 8));
    const floats = Array.from({length: 7}, (_, i) => bytes.readFloatLE(p + 136 + i * 4));
    const [barPhase, effectPhase, historyValid, captureStart, captureEnd, motionPhase, cycle, gate, readPosition, readRate] = doubles;
    const [dryL, dryR, wetL, wetR, outL, outR, wetAmount] = floats;
    if (!doubles.concat(floats).every(Number.isFinite)) nonfinite++;
    if (clock !== lastClock + 1 || (lastWriter >= 0 && writer !== lastWriter + 1)) {
      throw new Error(`TRACE_SERIAL_MISMATCH at sample ${clock}`);
    }
    if (firstWriter < 0) firstWriter = writer;
    stamps[writer % capacity] = writer;
    prefixes[writer % capacity] = cumulativeInputPower;
    inputPeaks[writer % capacity] = Math.max(Math.abs(dryL), Math.abs(dryR));
    cumulativeInputPower += (dryL * dryL + dryR * dryR) / 2;
    stamps[(writer + 1) % capacity] = writer + 1;
    prefixes[(writer + 1) % capacity] = cumulativeInputPower;
    let g = groups.get(entry);
    if (!g) {
      g = { entry, bar: activeBar + 1, preset, firstSample: clock, lastSample: clock,
        samples: 0, localSlotMismatchSamples: 0, capturedSamples: 0, wetSamples: 0,
        gateOpenSamples: 0, gateOpenWetSamples: 0, sourceEndedSamples: 0,
        firstCaptureSample: null, captureStart: null, captureEnd: null, captureRms: null,
        capturePeak: null,
        captureEnergyStatus: 'NOT_CAPTURED', captureChangedWithinEntry: false,
        dryPower: 0, outputPower: 0, wetPower: 0, gateOpenDeltaPower: 0,
        dryPeak: 0, outputPeak: 0, resetCount, resetReason };
      groups.set(entry, g);
    }
    g.lastSample = clock; g.samples++;
    g.localSlotMismatchSamples += localBar !== activeBar ? 1 : 0;
    g.capturedSamples += (flags & 1) ? 1 : 0;
    g.wetSamples += wetAmount > .99 ? 1 : 0;
    g.sourceEndedSamples += (flags & 8) ? 1 : 0;
    g.dryPower += (dryL * dryL + dryR * dryR) / 2;
    g.outputPower += (outL * outL + outR * outR) / 2;
    g.dryPeak = Math.max(g.dryPeak, Math.abs(dryL), Math.abs(dryR));
    g.outputPeak = Math.max(g.outputPeak, Math.abs(outL), Math.abs(outR));
    const evaluated = Boolean(flags & 4);
    const open = evaluated && (!(flags & 2) || gate > .5);
    if (evaluated) g.wetPower += (wetL * wetL + wetR * wetR) / 2;
    if (open) {
      g.gateOpenSamples++;
      if (wetAmount > .99) g.gateOpenWetSamples++;
      g.gateOpenDeltaPower += ((outL - dryL) ** 2 + (outR - dryR) ** 2) / 2;
    }
    if (flags & 1) {
      if (g.firstCaptureSample !== null) g.captureChangedWithinEntry ||= captureStart !== g.captureStart || captureEnd !== g.captureEnd;
      if (!seenCaptures.has(entry)) {
        seenCaptures.add(entry);
        g.firstCaptureSample = clock; g.captureStart = captureStart; g.captureEnd = captureEnd;
        const start = Math.floor(captureStart), finish = Math.floor(captureEnd);
        if (finish > start && start >= firstWriter && stamps[start % capacity] === start && stamps[finish % capacity] === finish) {
          g.captureRms = Math.sqrt(Math.max(0, prefixes[finish % capacity] - prefixes[start % capacity]) / (finish - start));
          let peak = 0;
          for (let serial = start; serial < finish; serial++) {
            if (stamps[serial % capacity] !== serial) throw new Error('CAPTURE_WINDOW_SERIAL_MISMATCH');
            peak = Math.max(peak, inputPeaks[serial % capacity]);
          }
          g.capturePeak = peak;
          g.captureEnergyStatus = 'MEASURED_INTEGER_SAMPLE_WINDOW';
        } else g.captureEnergyStatus = 'UNKNOWN_WINDOW_NOT_IN_CAPTURED_PCM';
      }
    }
    lastClock = clock; lastWriter = writer; count++;
  }
  carry = Buffer.from(bytes.subarray(end));
}
if (carry.length || count !== receipt.framesWritten || nonfinite) throw new Error('INCOMPLETE_OR_NONFINITE_TRACE');
const barResults = [...groups.values()].map(g => {
  const {dryPower, outputPower, wetPower, gateOpenDeltaPower, ...result} = g;
  return {...result, dryRms: Math.sqrt(dryPower / g.samples), outputRms: Math.sqrt(outputPower / g.samples),
    wetRms: Math.sqrt(wetPower / g.samples), gateOpenDeltaRms: g.gateOpenSamples ? Math.sqrt(gateOpenDeltaPower / g.gateOpenSamples) : null};
});
const backwards = blocks.slice(1).filter((b, i) => b.ppqPresent && blocks[i].ppqPresent && b.ppq < blocks[i].ppq - 1e-5)
  .map(b => ({sampleClock: b.sampleClock, ppq: b.ppq, looping: b.looping, loopPointsPresent: b.loopPointsPresent,
    startBar: b.startBar + 1, originBefore: b.originBefore, originAfter: b.originAfter,
    resetRequested: b.resetRequested, resetReason: b.resetReason}));
const report = {schemaVersion: 1, diagnosticValid: true, sampleRate: receipt.sampleRate, sampleCount: count,
  droppedRecords: 0, blockCount: blocks.length, backwardsPpqEvents: backwards, barResults,
  interpretation: 'Measured diagnostic evidence only. Not repair AC PASS, root-cause confirmation, or listening approval.'};
await writeFile(join(folder, 'analysis.json'), JSON.stringify(report, null, 2));
console.log(JSON.stringify(report, null, 2));
