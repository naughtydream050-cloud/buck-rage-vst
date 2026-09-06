"""Prepare only OUTPUT assets; preserve every pixel outside the OUTPUT panel."""
from pathlib import Path
import hashlib
import json
import re
from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / 'Resources/ui-v2/runtime-1024'
REPORT = ROOT / 'reports/latest/output-geometry'
REPORT.mkdir(parents=True, exist_ok=True)
reference_path = ROOT / 'Resources/ui-v2/reference/master-timeline-reference-1024x683.png'
reference = Image.open(reference_path).convert('RGBA')
face_path = RUNTIME / 'static/static_faceplate_1024x683.png'
face = Image.open(face_path).convert('RGBA')
before = face.copy()
layout = (ROOT / 'Source/GeneratedLayout.h').read_text()

def bounds(name):
    m = re.search(name + r'\(\)\s*\{\s*return\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)', layout)
    assert m, name
    return tuple(map(int, m.groups()))

def box(b):
    x,y,w,h = b
    return x,y,x+w,y+h

def gold(c):
    return c[0] > 80 and c[1] > 50 and c[0]-c[2] > 20

led_path = RUNTIME / 'meter_led_strip.png'
led = Image.open(led_path).convert('RGBA')
points = [(x,y) for y in range(led.height) for x in range(led.width) if gold(led.getpixel((x,y)))]
crop = (min(x for x,y in points), min(y for x,y in points), max(x for x,y in points)+1, max(y for x,y in points)+1)
native = led.crop(crop)
# Restore all previously cleared OUTPUT rectangles from the exact reference.
# No global faceplate recomposition: unrelated approved fixes stay untouched.
old = [(935,419,12,174),(971,419,12,174),(923,601,39,21),(960,601,39,21)]
for b in old:
    face.paste(reference.crop(box(b)), box(b))
rows=[]
for side,window in [('L',(936,960)),('R',(970,993))]:
    b = bounds('output'+side+'Bounds')
    # Independent reference evidence: bottom illuminated segment edges.
    xs = [x for x in range(*window) if gold(reference.getpixel((x,592)))]
    assert (b[0],b[2]) == (min(xs), max(xs)-min(xs)+1)
    assert b[1:4:2] == (419,174)
    prepared = native.resize((b[2],b[3]), Image.Resampling.LANCZOS)
    prepared.save(RUNTIME / ('output_meter_'+('left' if side=='L' else 'right')+'.png'))
    rows.append({'channel':side,'bounds':b,'reference_visible_center_x':(min(xs)+max(xs))/2,'readout':bounds('output'+side+'ReadoutBounds')})
for name in ['outputLBounds','outputRBounds','outputLReadoutBounds','outputRReadoutBounds']:
    b=bounds(name)
    # Retain original RGB underneath the alpha mask.
    patch=reference.crop(box(b)); patch.putalpha(0)
    face.paste(patch, box(b))
panel=bounds('outputPanelBounds')
diff=ImageChops.difference(before,face)
diff.paste((0,0,0,0),box(panel))
assert not diff.getbbox(), 'Non-OUTPUT pixel changed'
face.save(face_path)
black=Image.new('RGBA',face.size,(0,0,0,255))
composite=Image.alpha_composite(black,face)
for side in ['L','R']:
    b=bounds('output'+side+'Bounds')
    composite.alpha_composite(Image.open(RUNTIME/('output_meter_'+('left' if side=='L' else 'right')+'.png')), (b[0],b[1]))
comparison=Image.new('RGB',(panel[2]*3,panel[3]))
for i,im in enumerate([reference,Image.alpha_composite(black,before),composite]):
    comparison.paste(im.crop(box(panel)).convert('RGB'),(i*panel[2],0))
comparison.resize((panel[2]*9,panel[3]*3)).save(REPORT/'reference-before-after.png')
data={'reference':str(reference_path),'reference_sha256':hashlib.sha256(reference_path.read_bytes()).hexdigest(),'source_led_sha256':hashlib.sha256(led_path.read_bytes()).hexdigest(),'source_visible_crop':crop,'channels':rows,'outside_output_changed_pixels':0,'kind':'asset composite; JUCE/FL verification still required','worker':'Codex','token_savings_baseline':'unknown','token_savings_percentage':None}
(REPORT/'measurement.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(data,ensure_ascii=False))
