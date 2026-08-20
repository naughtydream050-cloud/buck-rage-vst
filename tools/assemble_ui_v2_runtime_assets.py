#!/usr/bin/env python3
"""Assemble native 1024 UI V2 assets from supplied V2 source assets only.

No FINAL MASTER image is opened or cropped.  Images are copied or resized once
offline to their final native runtime canvas; the plugin never rescales them.
"""
from __future__ import annotations
import hashlib, json, shutil
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / 'Resources/ui-v2/assets-raw-source-20260818'
MISSING = ROOT / 'Resources/ui-v2/source/incoming-missing-assets-20260820/ToyotomiHideyoshi_V2_missing_assets/runtime-1024'
APPROVED = ROOT / 'Resources/ui-v2/source/approved-standalone'
OUT = ROOT / 'Resources/ui-v2/runtime-1024'
MANIFEST = ROOT / 'ui/v2/runtime-manifest.json'
PRESETS = ['off','forward_cut','backspin','chirp','baby','transform','drag','zigzag','tape_brake','custom']
LENGTHS = ['1_16','1_8','1_4','1_2','1_bar']
TABS = [('1_16',105),('17_32',105),('33_48',106),('49_64',105)]

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def img(p): return Image.open(p).convert('RGBA')
def final(im, size): return im if im.size == size else im.resize(size, Image.Resampling.LANCZOS)
def save(im, rel, provenance, state, bounds):
    p=OUT/rel; p.parent.mkdir(parents=True, exist_ok=True); im.save(p)
    a=im.getchannel('A').getextrema(); return {'id':p.stem,'file':str(p.relative_to(ROOT)).replace('\\','/'),'provenance':provenance,'width':im.width,'height':im.height,'alpha':[a[0],a[1]],'state':state,'runtimeBounds':list(bounds),'sha256':sha(p)}
def shell(content, normal, selected, red=False):
    normal=final(normal,content.size); selected=final(selected,content.size)
    diff=ImageOps.grayscale(ImageChops.difference(normal.convert('RGB'),selected.convert('RGB'))).point(lambda v:255 if v>=12 else 0)
    inset=max(3,min(content.size)//12); ImageDraw.Draw(diff).rectangle((inset,inset,content.width-inset-1,content.height-inset-1),fill=0)
    if red:
        px=selected.load()
        for y in range(selected.height):
            for x in range(selected.width):
                r,g,b,a=px[x,y]
                if a and r>85 and g>55 and r>=b: px[x,y]=(min(236,int(r*1.1)),min(79,int(g*.42)),min(68,int(b*.42)),a)
    out=content.copy(); out.paste(selected,(0,0),diff); return out
def main():
    if OUT.exists(): shutil.rmtree(OUT)
    rows=[]
    # The supplied missing package is the only static V2 faceplate.
    rows.append(save(img(MISSING/'neutral_static_background_1024x683.png'),'static/neutral_static_background_1024x683.png','user-v2-missing-assets','neutral',(0,0,1024,683)))
    # Tabs: raw direct crops or prior mechanical state-shell results, normalised offline.
    for name,w in TABS:
        b=(0,0,w,27)
        for state in ('normal','selected'):
            source=RAW/f'tabs/tab_{name}_{state}.png'
            rows.append(save(final(img(source),(w,27)),f'tabs/tab_{name}_{state}.png','raw-final-master/mechanical-state','%s'%state,b))
    # Bars 1-16 are raw/mechanical; supplied package provides 17-64 neutral cells.
    refnormal=img(RAW/'bar-cells/bar_10_normal.png'); refselected=img(RAW/'bar-cells/bar_11_selected.png')
    for bar in range(1,65):
        base=img((RAW/f'bar-cells/bar_{bar:02d}_normal.png') if bar<=16 else (MISSING/f'bar-cells-17-64/bar_{bar:02d}_normal.png'))
        base=final(base,(56,80)); b=(0,0,56,80)
        rows.append(save(base,f'bars/bar_{bar:02d}_normal.png','raw-final-master' if bar<=16 else 'user-v2-missing-assets','normal',b))
        selected=shell(base,refnormal,refselected)
        playing=shell(base,refnormal,refselected,True)
        selected_playing=shell(selected,refnormal,refselected,True)
        rows.append(save(selected,f'bars/bar_{bar:02d}_selected.png','mechanical-state-shell','selected',b))
        rows.append(save(playing,f'bars/bar_{bar:02d}_playing.png','mechanical-state-shell','playing',b))
        rows.append(save(selected_playing,f'bars/bar_{bar:02d}_selected_playing.png','mechanical-state-shell','selected+playing',b))
    # Preset assets include complete shell/content pixels.  No C++ icon/text drawing.
    for preset in PRESETS:
        for state in ('normal','selected'):
            rows.append(save(final(img(RAW/f'presets/preset_{preset}_{state}.png'),(84,64)),f'presets/preset_{preset}_{state}.png','raw-final-master/mechanical-state',state,(0,0,84,64)))
    for length in LENGTHS:
        for state in ('normal','selected'):
            rows.append(save(final(img(RAW/f'length/length_{length}_{state}.png'),(32,26)),f'length/length_{length}_{state}.png','raw-final-master/mechanical-state',state,(0,0,32,26)))
    # Explicit user-approved standalone assets; final dimensions are native 1024 bounds.
    for file,size,bounds in [('knob_ring_60.png',(48,48),(0,0,48,48)),('knob_pointer_60.png',(48,48),(0,0,48,48)),('bypass_off.png',(80,31),(0,0,80,31)),('bypass_on.png',(80,31),(0,0,80,31)),('xy_neutral_base_288x256.png',(192,174),(0,0,192,174))]:
        rows.append(save(final(img(APPROVED/file),size),f'standalone/{file}','approved-standalone','neutral',bounds))
    for file in ['rec_normal.png','clear_normal.png','reset_view_normal.png']:
        source=img(MISSING/file); rows.append(save(source,f'xy-buttons/{file}','user-v2-missing-assets','normal',(0,0,source.width,source.height)))
    MANIFEST.parent.mkdir(parents=True,exist_ok=True)
    MANIFEST.write_text(json.dumps({'schemaVersion':2,'canvas':[1024,683],'runtimeScale':1.0,'finalMasterRecrop':'forbidden','image2Crop':'forbidden','assets':rows},ensure_ascii=False,indent=2),encoding='utf-8')
    print(f'assets={len(rows)} manifest={MANIFEST}')
if __name__=='__main__': main()
