#!/usr/bin/env python
import gerberex, os
from gerberex import DxfFile, GerberComposition, DrillComposition

exts = ['GTL', 'GTO', 'GTS', 'GBL', 'GBO', 'GBS']
boards=[
    ('../../pcb/pico2/CAMOutputs/rcstick-f-pico2.', 0, 0, 0),
]
outline = gerberex.read('../../pcb/pico2/CAMOutputs/rcstick-f-pico2.GML')
outputs = 'outputs-pico2/pcb'

if not os.path.isdir('outputs-pico2'):
    os.mkdir('outputs-pico2')

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
        pass
    else:
        ctx.merge(outline)
    ctx.dump(outputs + '.' + ext)
    print(' end', flush=True)
