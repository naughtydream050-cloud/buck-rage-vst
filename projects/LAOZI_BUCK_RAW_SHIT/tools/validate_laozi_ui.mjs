import fs from "node:fs";
import path from "node:path";

const root = path.resolve(import.meta.dirname, "..");
const spec = JSON.parse(fs.readFileSync(path.join(root, "ui/spec/ui-spec.json"), "utf8"));
const required = [spec.assets.reference, spec.assets.faceplate, ...spec.knobs.map((knob) => knob.asset)];
const findings = [];
for (const file of required) if (!fs.existsSync(path.join(root, file))) findings.push(`missing: ${file}`);
if (spec.canvas.width !== 1280 || spec.canvas.height !== 905) findings.push("unexpected canvas dimensions");
if (spec.knobs.length !== 5) findings.push("expected five knobs");
for (const knob of spec.knobs) if (knob.bounds.width !== knob.bounds.height || knob.radius * 2 !== knob.bounds.width) findings.push(`invalid circular bounds: ${knob.id}`);
const status = findings.length === 0 ? "passed" : "failed";
const report = { schemaVersion: 1, status, score: status === "passed" ? 10 : 0, visualMode: "image-first", checks: { referencePresent: fs.existsSync(path.join(root, spec.assets.reference)), faceplatePresent: fs.existsSync(path.join(root, spec.assets.faceplate)), knobCount: spec.knobs.length, circularKnobBounds: findings.filter((item) => item.includes("circular")).length === 0 }, findings };
const reports = path.resolve(root, "../../reports/latest");
fs.mkdirSync(reports, { recursive: true });
fs.writeFileSync(path.join(reports, "laozi-buck-raw-shit-ui-report.json"), JSON.stringify(report, null, 2) + "\n");
fs.writeFileSync(path.join(reports, "laozi-buck-raw-shit-context-pack.json"), JSON.stringify({ schemaVersion: 1, project: "LAOZI_BUCK_RAW_SHIT", phase: "WINDOWS_VST3_UI_IMPLEMENTATION_AND_INTERACTION_VALIDATION", visualTruth: spec.assets.reference, uiSpec: "projects/LAOZI_BUCK_RAW_SHIT/ui/spec/ui-spec.json", status, uiReport: "reports/latest/laozi-buck-raw-shit-ui-report.json", dsp: "temporary output gain + bypass + atomic stereo meter only", hostValidation: "pending" }, null, 2) + "\n");
if (status !== "passed") throw new Error(findings.join("; "));
console.log(JSON.stringify({ status, findings }));
