#!/usr/bin/env python3
"""Make the Steam Frame's menu tile: the loading screen tile with the Frame's button mapping on it.

Team Beef's release has the menu background as 12 tiles, assets/background/800_R_C_loading.tga, and
the controller help for the Quest is baked into the first one, 800_1_a_loading.tga. This reads that
tile from the release (the tiles are git-ignored, they aren't in the repo), takes the Quest text off
it and draws the Frame's mapping instead. The result goes into a build folder, never into the repo,
and gradle packs it into the Frame APK only (assembleFrame runs this by itself).

    tools/steam_frame/make_frame_tile.py [--src TILE] [--out FILE] [--preview DIR]

Needs Python 3 with Pillow (10.1 or newer) and numpy:

    python3 -m venv tools/steam_frame/.venv
    tools/steam_frame/.venv/bin/pip install pillow numpy

Exit status: 0 made the tile, 2 the source tile isn't there, 3 Pillow or numpy is missing.
The font is DIN Alternate Bold when the machine has it (macOS), else DejaVu Sans Bold, else the one
that comes with Pillow. Set L1VR_TILE_FONT to a .ttf to pick another.
"""
import argparse
import os
import sys

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DEFAULT_SRC = os.path.join(root, 'assets', 'background', '800_1_a_loading.tga')
DEFAULT_OUT = os.path.join(root, 'Projects', 'Android', 'build', 'frame-assets', 'background', '800_1_a_loading.tga')

# One line per thing, "Action = Button", as the Quest's text does. It's the map in VrFrameMap.c
# (docs/STEAM_FRAME_VR.md, Controls). Keep them in step.
LINES = [
    'Button Mapping (Steam Frame):',
    'Move = Left stick / Turn = Right stick',
    'Fire = Right trigger / Alt fire = Left trigger',
    'Jump = A / Use = X / Reload = Y',
    'Crouch = B (hold) / Click left stick',
    'Flashlight = Click right stick / View',
    'Weapons = LB / RB or D-pad left / right',
    'Last weapon = D-pad up',
    'Laser sight = D-pad down',
    'Crowbar = Grip behind your head',
    'Two handed guns = Left grip',
    'Right grip = Use (grab, push, pull)',
    'Pause = Menu / Quick save = Hold Menu',
    'Recentre = Hold View 1 s (3 s = set height)',
]
INK = (232, 232, 232)
PITCH = 11
X, Y, MAX_WIDTH = 18, 3, 232

FONTS = [
    '/System/Library/Fonts/Supplemental/DIN Alternate Bold.ttf',
    '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
    '/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf',
    '/usr/share/fonts/TTF/DejaVuSans-Bold.ttf',
]


def load_font(ImageFont, size):
    wanted = [os.environ['L1VR_TILE_FONT']] if os.environ.get('L1VR_TILE_FONT') else FONTS
    for path in wanted:
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size)


def dilate(mask, steps, np):
    # 3x3 box dilation, `steps` times
    for _ in range(steps):
        p = np.pad(mask, 1)
        out = np.zeros_like(mask)
        for dy in range(3):
            for dx in range(3):
                out |= p[dy:dy + mask.shape[0], dx:dx + mask.shape[1]]
        mask = out
    return mask


def clean_tile(rgb, np):
    """Take the baked-in text off: mask the bright glyphs and what's around them, then fill the mask
    from its edges (each masked pixel becomes the average of its neighbours, over and over)."""
    i = rgb.astype(np.int32)
    gray = np.rint((i[..., 0] * 299 + i[..., 1] * 587 + i[..., 2] * 114) / 1000.0).astype(np.int32)
    seed = gray >= 150
    seed[170:] = False
    mask = dilate((gray > 60) & dilate(seed, 2, np), 1, np)
    out = rgb.astype(np.float32)
    out[mask] = out[~mask].mean(axis=0)
    m = mask[..., None]
    for _ in range(400):
        p = np.pad(out, ((1, 1), (1, 1), (0, 0)), mode='edge')
        avg = (p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:]) / 4
        out = np.where(m, avg, out)
    return np.clip(out + 0.5, 0, 255).astype(np.uint8)


def draw_text(image, ImageDraw, ImageFont):
    draw = ImageDraw.Draw(image)
    for size in (10, 9, 8, 7):
        font = load_font(ImageFont, size)
        if all(draw.textlength(t, font=font) <= MAX_WIDTH for t in LINES):
            break
    for n, text in enumerate(LINES):
        draw.text((X, Y + n * PITCH), text, font=font, fill=INK)


def write_preview(folder, tile, src_dir, Image):
    """The whole 800x600 menu background with the new tile in it, and a zoom of the text."""
    os.makedirs(folder, exist_ok=True)
    full = Image.new('RGB', (800, 600))
    for r, row in enumerate('123'):
        for c, col in enumerate('abcd'):
            name = os.path.join(src_dir, '800_%s_%s_loading.tga' % (row, col))
            t = tile if (r, c) == (0, 0) else Image.open(name).convert('RGB') if os.path.exists(name) else None
            if t is not None:
                full.paste(t.convert('RGB'), (c * 256, r * 256))
    full.save(os.path.join(folder, 'compact_800x600.png'))
    full.crop((0, 0, 300, 180)).resize((600, 360), Image.LANCZOS).save(os.path.join(folder, 'zoom_header.png'))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--src', default=DEFAULT_SRC, help='the release tile 800_1_a_loading.tga')
    ap.add_argument('--out', default=DEFAULT_OUT, help='where the Frame tile is written')
    ap.add_argument('--preview', metavar='DIR', help='also write a 800x600 preview and a zoom of the text here')
    args = ap.parse_args()

    try:
        import numpy as np
        from PIL import Image, ImageDraw, ImageFont
    except ImportError as e:
        print('make_frame_tile: %s. Needs Pillow and numpy, see the top of this file.' % e, file=sys.stderr)
        return 3

    if not os.path.isfile(args.src):
        print('make_frame_tile: %s is not there. It comes from Team Beef\'s release APK '
              '(assets/background), see docs/STEAM_FRAME_VR.md.' % args.src, file=sys.stderr)
        return 2

    source = Image.open(args.src)
    if source.size != (256, 256):
        print('make_frame_tile: %s is %dx%d, expected 256x256' % (args.src, *source.size), file=sys.stderr)
        return 2
    alpha = source.getchannel('A') if 'A' in source.getbands() else None
    tile = Image.fromarray(clean_tile(np.asarray(source.convert('RGB')), np))
    draw_text(tile, ImageDraw, ImageFont)
    if alpha is not None:
        tile.putalpha(alpha)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    tile.save(args.out, format='TGA')
    print('make_frame_tile: wrote %s' % args.out)
    if args.preview:
        write_preview(args.preview, tile, os.path.dirname(os.path.abspath(args.src)), Image)
    return 0


if __name__ == '__main__':
    sys.exit(main())
