#!/usr/bin/env node
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const repoRoot = path.resolve(projectRoot, "../..");

function arg(name, fallback) {
  const index = process.argv.indexOf(`--${name}`);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

function rel(file) {
  return path.relative(repoRoot, file).replaceAll("\\", "/");
}

function read(file) {
  return fs.readFileSync(file, "utf8");
}

function writeJson(file, payload) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
}

function processBlock(text) {
  const match = /\bprocessBlock\s*\([^)]*\)/s.exec(text);
  if (!match) return "";
  const open = text.indexOf("{", match.index + match[0].length);
  if (open < 0) return "";
  let depth = 0;
  for (let index = open; index < text.length; index += 1) {
    if (text[index] === "{") depth += 1;
    if (text[index] === "}") depth -= 1;
    if (depth === 0) return text.slice(open, index + 1);
  }
  return "";
}

const outPath = path.resolve(repoRoot, arg("out", "reports/latest/rude-hype/dsp-macro-report.json"));
const processorCppPath = path.join(projectRoot, "Source", "PluginProcessor.cpp");
const processorHPath = path.join(projectRoot, "Source", "PluginProcessor.h");
const notesPath = path.join(projectRoot, "docs", "dsp-preview-notes.md");
const cpp = read(processorCppPath);
const header = read(processorHPath);
const block = processBlock(cpp);
const notes = fs.existsSync(notesPath) ? read(notesPath) : "";
const findings = [];

const requiredSignals = [
  ["shout-smoothing", "shoutSmooth"],
  ["burn-smoothing", "burnSmooth"],
  ["upper-mid-band", "kShoutFocusLowHz"],
  ["exciter-layer", "exciter"],
  ["stereo-width", "width"],
  ["transistor-layer", "transistorSat"],
  ["wavefold-layer", "wavefold"],
  ["downsample-flavor", "downsampleFlavor"],
  ["tape-compression", "tapeEnv"],
  ["fizz-layer", "fizz"],
  ["dc-blocker", "kDcBlockerPole"],
  ["output-ceiling", "kOutputCeiling"],
  ["denormal-protection", "ScopedNoDenormals"],
];

for (const [rule, token] of requiredSignals) {
  if (!cpp.includes(token) && !header.includes(token)) {
    findings.push({ severity: "error", rule, message: `Missing DSP contract token: ${token}` });
  }
}

const forbiddenInProcessBlock = [
  ["heap-new", /\bnew\b/],
  ["malloc", /\b(?:malloc|calloc|realloc|free)\s*\(/],
  ["vector-resize", /\b(?:resize|push_back|emplace_back)\s*\(/],
  ["fft", /\bFFT\b/i],
  ["file-io", /\b(?:File|FileInputStream|FileOutputStream)\b/],
  ["lock", /\b(?:lock|mutex|MessageManagerLock)\b/i],
];

for (const [rule, pattern] of forbiddenInProcessBlock) {
  if (pattern.test(block)) {
    findings.push({ severity: "error", rule, message: `Forbidden processBlock pattern: ${rule}` });
  }
}

for (const term of ["SHOUT Response", "BURN Response", "Parameter Mapping", "Realtime Safety"]) {
  if (!notes.includes(term)) {
    findings.push({ severity: "error", rule: "dsp-notes", message: `Missing notes section: ${term}` });
  }
}

const score = Math.max(0, 10 - findings.filter((finding) => finding.severity === "error").length * 2);
const report = {
  schemaVersion: 1,
  generatedAt: new Date().toISOString(),
  project: rel(projectRoot),
  status: score >= 9 ? "passed" : "failed",
  score,
  checks: {
    noClippingExplosion: cpp.includes("kOutputCeiling") && block.includes("jlimit"),
    noDcBuildup: cpp.includes("kDcBlockerPole") && block.includes("dcY1"),
    parameterSmoothing: cpp.includes("shoutSmooth") && cpp.includes("burnSmooth"),
    noHeapAllocationInProcessBlock: !/\bnew\b|\b(?:malloc|calloc|realloc|free)\s*\(/.test(block),
  },
  macroLayers: {
    shout: ["soft saturation", "upper-mid exciter", "micro motion", "soft clip", "stereo width"],
    burn: ["band split", "transistor saturation", "wavefold", "mild downsample", "tape compression", "fizz", "clip"],
  },
  findings,
};

writeJson(outPath, report);
console.log(JSON.stringify({ out: rel(outPath), status: report.status, score: report.score }));
process.exit(report.status === "passed" ? 0 : 1);
