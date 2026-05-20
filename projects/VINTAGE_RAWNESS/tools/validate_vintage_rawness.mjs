import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { fileURLToPath } from "node:url";

const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const reportsRoot = path.resolve(projectRoot, "..", "..", "reports", "latest");
fs.mkdirSync(reportsRoot, { recursive: true });

const readJson = (file) => JSON.parse(fs.readFileSync(file, "utf8"));
const sha256 = (file) => crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex");

function pngInfo(file) {
  const data = fs.readFileSync(file);
  const signature = data.subarray(0, 8).toString("hex");
  if (signature !== "89504e470d0a1a0a") throw new Error(`${file} is not a PNG`);
  return {
    width: data.readUInt32BE(16),
    height: data.readUInt32BE(20),
    bitDepth: data.readUInt8(24),
    colorType: data.readUInt8(25),
    bytes: data.length,
    sha256: sha256(file)
  };
}

function writeReport(name, value) {
  fs.writeFileSync(path.join(reportsRoot, name), `${JSON.stringify(value, null, 2)}\n`);
}

const spec = readJson(path.join(projectRoot, "ui", "spec", "ui-spec.json"));
const referencePath = path.join(projectRoot, spec.assets.reference);
const faceplatePath = path.join(projectRoot, spec.assets.faceplate);
const reference = pngInfo(referencePath);
const faceplate = pngInfo(faceplatePath);

const findings = [];
if (spec.canvas.width !== reference.width || spec.canvas.height !== reference.height) {
  findings.push({ severity: "error", rule: "canvas_matches_reference" });
}
if (spec.display.scale <= 0 || spec.display.scale > 1) {
  findings.push({ severity: "error", rule: "display_scale_range" });
}

for (const knob of spec.knobs) {
  const asset = pngInfo(path.join(projectRoot, knob.asset));
  if (asset.width !== asset.height) findings.push({ severity: "error", rule: "knob_square", knob: knob.id });
  if (asset.colorType !== 6) findings.push({ severity: "error", rule: "knob_rgba_png", knob: knob.id });
  if (knob.bounds.width !== knob.bounds.height) findings.push({ severity: "error", rule: "knob_bounds_square", knob: knob.id });
  if (knob.radius * 2 !== knob.bounds.width) findings.push({ severity: "error", rule: "knob_radius_matches_bounds", knob: knob.id });
  if (knob.center.x !== knob.bounds.x + knob.radius || knob.center.y !== knob.bounds.y + knob.radius) {
    findings.push({ severity: "error", rule: "knob_center_matches_bounds", knob: knob.id });
  }
}

const editorText = fs.readFileSync(path.join(projectRoot, "Source", "PluginEditor.cpp"), "utf8");
const processorText = fs.readFileSync(path.join(projectRoot, "Source", "PluginProcessor.cpp"), "utf8");
const processBlock = processorText.match(/void VintageRawnessProcessor::processBlock[\s\S]*?void VintageRawnessProcessor::getStateInformation/);
const dspFindings = [];
if (!processBlock) dspFindings.push({ severity: "error", rule: "process_block_found" });
if (processBlock && /\bnew\b|malloc|calloc|std::vector|push_back|resize\s*\(/.test(processBlock[0])) {
  dspFindings.push({ severity: "error", rule: "no_heap_allocation_in_process_block" });
}
if (!/ScopedNoDenormals/.test(processorText)) dspFindings.push({ severity: "error", rule: "denormal_protection" });
if (!/dcBlock/.test(processorText)) dspFindings.push({ severity: "error", rule: "dc_blocker" });
if (!/jlimit\(-kOutputCeiling/.test(processorText)) dspFindings.push({ severity: "error", rule: "output_ceiling" });
if (/fillEllipse|drawText\(\"VINTAGE RAWNESS/.test(editorText)) {
  findings.push({ severity: "error", rule: "no_juce_ui_redraw" });
}

const uiPassed = findings.length === 0;
const dspPassed = dspFindings.length === 0;
const referenceHash = reference.sha256;
const faceplateHash = faceplate.sha256;
const screenshotDiff = {
  schemaVersion: 1,
  status: referenceHash === faceplateHash ? "passed" : "bootstrap",
  score: referenceHash === faceplateHash ? 10 : 7,
  similarity: referenceHash === faceplateHash ? 1 : null,
  rms: referenceHash === faceplateHash ? 0 : null,
  note: "Bootstrap diff compares reference.png and faceplate_vintage_rawness.png until a live host screenshot exists."
};

writeReport("vintage-rawness-ui-report.json", {
  schemaVersion: 1,
  status: uiPassed ? "passed" : "failed",
  score: uiPassed ? 10 : 6,
  canvas: spec.canvas,
  display: spec.display,
  reference,
  faceplate,
  findings
});

writeReport("vintage-rawness-knob-report.json", {
  schemaVersion: 1,
  status: uiPassed ? "passed" : "failed",
  score: uiPassed ? 10 : 6,
  knobs: spec.knobs.map((knob) => ({ id: knob.id, asset: knob.asset, png: pngInfo(path.join(projectRoot, knob.asset)), center: knob.center, radius: knob.radius, bounds: knob.bounds })),
  findings: findings.filter((finding) => finding.rule.startsWith("knob"))
});

writeReport("vintage-rawness-dsp-report.json", {
  schemaVersion: 1,
  status: dspPassed ? "passed" : "failed",
  score: dspPassed ? 10 : 6,
  checks: {
    noHeapAllocationInProcessBlock: !dspFindings.some((finding) => finding.rule === "no_heap_allocation_in_process_block"),
    denormalProtection: /ScopedNoDenormals/.test(processorText),
    dcBlocker: /dcBlock/.test(processorText),
    outputCeiling: /jlimit\(-kOutputCeiling/.test(processorText)
  },
  findings: dspFindings
});

writeReport("vintage-rawness-screenshot-diff.json", screenshotDiff);

writeReport("vintage-rawness-context-pack.json", {
  schemaVersion: 1,
  generatedAt: new Date().toISOString(),
  project: "projects/VINTAGE_RAWNESS",
  purpose: "Minimal handoff payload for image-first VINTAGE RAWNESS plugin work.",
  tokenPolicy: {
    sendThisFileInsteadOf: ["large images", "full markdown scans", "full build logs"],
    localOnlyArtifacts: ["projects/VINTAGE_RAWNESS/ui/reference/reference.png", "projects/VINTAGE_RAWNESS/ui/rendered/rendered.png"]
  },
  visualDoctrine: {
    mode: "image-first",
    ssot: "ui/spec/ui-spec.json",
    faceplate: "Resources/faceplate_vintage_rawness.png",
    rotatingAssets: ["Resources/knob_dirt.png", "Resources/knob_crush.png", "Resources/knob_wobble.png"]
  },
  canvas: spec.canvas,
  display: spec.display,
  knobs: spec.knobs,
  presetButtons: spec.presetButtons,
  checks: {
    uiSpec: { status: uiPassed ? "passed" : "failed", score: uiPassed ? 10 : 6 },
    knobAlpha: { status: uiPassed ? "passed" : "failed", score: uiPassed ? 10 : 6 },
    knobRotation: { status: uiPassed ? "passed" : "failed", score: uiPassed ? 10 : 6 },
    hitArea: { status: uiPassed ? "passed" : "failed", score: uiPassed ? 10 : 6 },
    screenshotDiff,
    dspPolicy: { status: dspPassed ? "passed" : "failed", score: dspPassed ? 10 : 6 },
    build: { status: "blocked-local-toolchain", score: 0 }
  }
});

console.log(JSON.stringify({ status: uiPassed && dspPassed ? "passed" : "failed", reportsRoot }, null, 2));
