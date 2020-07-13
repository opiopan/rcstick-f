#!/usr/bin/env python
import gerberex
from gerberex import DxfFile, GerberComposition, DrillComposition

exts = ['GTL', 'GTO', 'GTP', 'GTS', 'GBL', 'GBO', 'GBP', 'GBS', 'TXT']
boards=[
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 0, 68, 0),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 20, 68, 0),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 40, 68, 0),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 60, 68, 0),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 80, 68, 0),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 15.4, 32, 180),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 35.4, 32, 180),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 55.4, 32, 180),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 75.4, 32, 180),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 95.4, 32, 180),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 32, 42.3, 90),
    ('../tiny/CAMOutputs/rcstick-f-tiny.', 63.4, 57.7, -90),
]
outline = 'outline-tiny.dxf'
mousebites = 'mousebites-tiny.dxf'
outputs = 'outputs-tiny/rcstick-panelized'

for ext in exts:
    print('merging %s: ' % ext ,end='', flush=True)
    if ext == 'TXT':
        ctx = DrillComposition()
    else:
        ctx = GerberComposition()
    for board in boards:
        file = gerberex.read(board[0] + ext)
        file.to_metric()
        if ext == 'GTO' or ext == 'GBO':
            for adef in file.aperture_defs:
                if adef.shape == 'C' and adef.modifiers[0][0] < 0.12:
                    adef.modifiers[0] = (0.12,)
                elif adef.shape == 'R' and adef.modifiers[0][1] < 0.05:
                    adef.modifiers[0] = (adef.modifiers[0][0], 0.05)
        file.rotate(board[3])
        file.offset(board[1], board[2])
        ctx.merge(file)
        print('.', end='', flush=True)
    if ext == 'TXT':
        file = gerberex.read(mousebites)
        file.draw_mode = DxfFile.DM_MOUSE_BITES
        file.width = 0.5
        file.format = (3, 3)
        ctx.merge(file)
    else:
        file = gerberex.read(outline)
        ctx.merge(file)
    ctx.dump(outputs + '.' + ext)
    print(' end', flush=True)

print('generating GML: ', end='', flush=True)
file = gerberex.read(outline)
file.write(outputs + '.GML')
print('.', end='', flush=True)
ctx = GerberComposition()
base = gerberex.rectangle(width=100, height=100, left=0, bottom=0, units='metric')
base.draw_mode = DxfFile.DM_FILL
ctx.merge(base)
file.to_metric()
file.draw_mode = DxfFile.DM_FILL
file.negate_polarity()
ctx.merge(file)
ctx.dump(outputs + '-fill.GML')

print('. end', flush=True)
