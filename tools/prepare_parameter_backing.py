"""Restore reference corners; remove only baked parameter values."""
from pathlib import Path
import re, json
from PIL import Image, ImageChops

root=Path(__file__).resolve().parents[1]
runtime=root/'Resources/ui-v2/runtime-1024'
reference=Image.open(root/'Resources/ui-v2/reference/master-timeline-reference-1024x683.png').convert('RGBA')
path=runtime/'static/static_faceplate_1024x683.png'
face=Image.open(path).convert('RGBA'); before=face.copy()
ring=Image.open(runtime/'standalone/knob_ring_60.png').convert('RGBA')
layout=(root/'Source/GeneratedLayout.h').read_text()
def bounds(name):
    return tuple(map(int,re.search(name+r'\(\)\s*\{\s*return\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)',layout).groups()))
def box(b):
    x,y,w,h=b; return x,y,x+w,y+h
rows=[]
for name,old in [('speed',(742,563,48,16)),('pitch',(793,563,48,16)),('depth',(848,563,48,16))]:
    b=bounds(name+'KnobBounds'); assert ring.size==b[2:]
    patch=reference.crop(box(b))
    # The base owns its opaque pixels; reference artwork owns its clear corners.
    patch.putalpha(ImageChops.invert(ring.getchannel('A')))
    face.paste(patch,box(b))
    face.paste(reference.crop(box(old)),box(old))
    r=bounds(name+'ReadoutBounds')
    # The fixed box frame stays in the reference. Reconstruct its empty inner
    # surface from clean rows immediately above and below the baked glyphs.
    patch=reference.crop(box(r))
    for x in range(r[2]):
        top=reference.getpixel((r[0]+x,570)); bottom=reference.getpixel((r[0]+x,588))
        for y in range(r[3]):
            t=(y+1)/18
            patch.putpixel((x,y),tuple(round(top[c]*(1-t)+bottom[c]*t) for c in range(3))+(255,))
    face.paste(patch,box(r))
    rows.append({'control':name,'knob':b,'readout':r,'native_ring':ring.size})
diff=ImageChops.difference(before,face)
diff.paste((0,0,0,0),(740,511,897,591))
assert all(c.getextrema()[1]==0 for c in diff.split()),'Changed outside parameter region'
face.save(path)
out=root/'reports/latest/parameter-backing'; out.mkdir(parents=True,exist_ok=True)
composite=Image.alpha_composite(Image.new('RGBA',face.size,(0,0,0,255)),face)
for row in rows: composite.alpha_composite(ring,tuple(row['knob'][:2]))
preview=Image.new('RGB',(164*3,117))
for i,im in enumerate([reference,Image.alpha_composite(Image.new('RGBA',face.size,(0,0,0,255)),before),composite]):
    preview.paste(im.crop((735,480,899,597)),(164*i,0))
preview.resize((984,234)).save(out/'reference-before-after.png')
(out/'measurement.json').write_text(json.dumps({'controls':rows,'outside_parameter_changed_pixels':0,'scope':'backing alpha and readout geometry','host_validation':'NOT TESTED'},indent=2),encoding='utf8')
print(json.dumps(rows))
