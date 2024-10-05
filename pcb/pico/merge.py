#!/usr/bin/env python3
import os, sys, gerberex
from gerberex import DxfFile, GerberComposition, DrillComposition
sys.path.append(os.path.join(os.path.dirname(__file__), '../pcbalib'))
import jlcpcb

exts = ['GTL', 'GTO', 'GTP', 'GTS', 'GBL', 'GBO', 'GBP', 'GBS', 'TXT']
boards=[
    ('CAMOutputs/rcstick-f-pico.', 27.3, 18.6, 0),
]
outline = 'production/outline.dxf'
mousebites = 'production/mousebites.dxf'
productid = ('production/productid.gbr', 4.5, 7, 0)
asmtool_drill = 'production/asmtool.txt'
asmtool_mask = 'production/asmtool.gbr'
asmtools = [(3.85, 3.85), (3.85, 70 - 3.85), (70 - 3.85, 3.85), (70 - 3.85, 70 - 3.85)]
outputs = 'outputs/rcstick-f-pico'
cpl = 'outputs/CPL.csv'

if not os.path.isdir('outputs'):
    os.mkdir('outputs')

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
        for asmtool in asmtools:
            file = gerberex.read(asmtool_drill)
            file.to_metric()
            file.offset(asmtool[0], asmtool[1])
            ctx.merge(file)
    else:
        if ext == 'GTS' or ext == 'GBS':
            for asmtool in asmtools:
                file = gerberex.read(asmtool_mask)
                file.to_metric()
                file.offset(asmtool[0], asmtool[1])
                ctx.merge(file)
        elif ext == 'GTO':
            file = gerberex.read(productid[0])
            file.to_metric()
            file.rotate(productid[3])
            file.offset(productid[1], productid[2])
            ctx.merge(file)
        #file = gerberex.read(outline)
        #ctx.merge(file)
    ctx.dump(outputs + '.' + ext)
    print(' end', flush=True)

print('generating CPL: ', end='', flush=True)
offset_angles = {
    'LED-SMD_L1.6-W0.8-R-RD': 180,
    'SOT-23-3_L2.9-W1.3-P1.90-LS2.4-BR': -90,
    'USB-C-SMD_C561769': 180,
}
offsets = {
    'USB-C-SMD_C561769': [-0.34, 0.28],
}
ctx = jlcpcb.Composition()
ctx.setBom('BOM.csv', offset_angles, offsets)
for board in boards:
    print('.', end='', flush=True)
    file = jlcpcb.MountFile()
    file.load(board[0] + 'mnt', jlcpcb.TOP)
    file.rotate(board[3])
    file.offset(board[1], board[2])
    ctx.merge(file)
    file = jlcpcb.MountFile()
    file.load(board[0] + 'mnb', jlcpcb.BOTTOM)
    file.rotate(board[3])
    file.offset(board[1], board[2])
    ctx.merge(file)
ctx.dump(cpl)
print(' end', flush=True)

print('generating GML: ', end='', flush=True)
file = gerberex.read(outline)
file.write(outputs + '.GML')
print('.', end='', flush=True)
ctx = GerberComposition()
base = gerberex.rectangle(width=70, height=70, left=0, bottom=0, units='metric')
base.draw_mode = DxfFile.DM_FILL
ctx.merge(base)
file.to_metric()
file.draw_mode = DxfFile.DM_FILL
file.negate_polarity()
ctx.merge(file)
ctx.dump(outputs + '-fill.GML')

print('. end', flush=True)
