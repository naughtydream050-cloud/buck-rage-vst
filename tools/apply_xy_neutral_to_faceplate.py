"""Apply the approved neutral XY panel without regenerating the faceplate."""
from pathlib import Path
import argparse
import numpy as np
from PIL import Image

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--base', type=Path, default=root / 'Resources/ui-v2/runtime-1024/static/static_faceplate_1024x683.png')
args = parser.parse_args()
target = root / 'Resources/ui-v2/runtime-1024/static/static_faceplate_1024x683.png'
neutral = root / 'reports/latest/xy-neutral/xy-pad-neutral-confirmed.png'
base = np.array(Image.open(args.base).convert('RGBA'))
patch = np.array(Image.open(neutral).convert('RGBA'))
if base.shape != (683, 1024, 4) or patch.shape != (160, 190, 4):
    raise SystemExit('Unexpected faceplate or confirmed neutral XY dimensions.')
result = base.copy()
result[430:590, 42:232] = patch
y, x = np.indices(base.shape[:2])
outside = ~((x >= 42) & (x < 232) & (y >= 430) & (y < 590))
if np.any(base[outside] != result[outside]):
    raise SystemExit('XY-external pixels changed.')
Image.fromarray(result).save(target)
print('XY_OUTSIDE_DIFF=0')
