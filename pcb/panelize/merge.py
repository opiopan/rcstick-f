#!/usr/bin/env python
import gerberex
from gerberex import DxfFile, GerberComposition, DrillComposition

exts = ['GTL', 'GTO', 'GTP', 'GTS', 'GBL', 'GBO', 'GBP', 'GBS', 'TXT']
boards=[
    ('../small/CAMOutputs/rcstick-f-small.', 0, 60, 0),
    ('../small/CAMOutputs/rcstick-f-small.', 20, 60, 0),
    ('../small/CAMOutputs/rcstick-f-small.', 40, 60, 0),
    ('../small/CAMOutputs/rcstick-f-small.', 60, 60, 0),
    ('../small/CAMOutputs/rcstick-f-small.', 80, 60, 0),
    ('../small/CAMOutputs/rcstick-f-small.', 15.4, 45, 180),
    ('../small/CAMOutputs/rcstick-f-small.', 35.4, 45, 180),
    ('../small/CAMOutputs/rcstick-f-small.', 55.4, 45, 180),
    ('../small/CAMOutputs/rcstick-f-small.', 75.4, 45, 180),
    ('../small/CAMOutputs/rcstick-f-small.', 95.4, 45, 180),
]
outline = 'outline.dxf'
mousebites = 'mousebites.dxf'
outputs = 'outputs/rcstick-panelized'

for ext in exts:
    print('merging %s: ' % ext ,end='', flush=True)
    if ext == 'TXT':
        ctx = DrillComposition()
    else:
        ctx = GerberComposition()
    for board in boards:
        file = gerberex.read(board[0] + ext)
        file.to_metric()
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
