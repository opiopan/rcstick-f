#!/usr/bin/env python
from gerber import load_layer
from gerber.render import RenderSettings, theme
from gerber.render.cairo_backend import GerberCairoContext
from PIL import Image, ImageDraw, ImageFilter, ImageOps

prefix = 'outputs-pico/pcb'
mcolor = (5.0/255.0, 30.0/255.0, 187/255.0)
bcolor = (6.0/300.0, 13.0/300.0, 106/300.0)

metal_settings = RenderSettings(color=mcolor)
bg_settings = RenderSettings(color=bcolor)
silk_settings = RenderSettings(color=theme.COLORS['white'], alpha=0.80)
hmap_settings = RenderSettings(color=(1.0, 1.0, 1.0))
hmapbg_settings = RenderSettings(color=(0, 0, 0))
mask_settings = RenderSettings(color=(0, 0, 0))
maskbg_settings = RenderSettings(color=(1.0, 1.0, 1.0))

blur_radius = 4

#-------------------------------------------------------------
# generate top base image
#-------------------------------------------------------------
ctx = GerberCairoContext(scale=80)
print('### Top Base Image ###')
print('loading... ', end='', flush=True)
copper = load_layer(prefix + '.GTL')
silk = load_layer(prefix + '.GTO')
print(' end', flush=True)
 
print('drawing... ', end='', flush=True)
ctx.render_layer(copper, settings=metal_settings, bgsettings=bg_settings)
ctx.render_layer(silk, settings=silk_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
ctx.dump('outputs-pico/pcb-top-base.png')
print(' end', flush=True)

#-------------------------------------------------------------
# generate top height map image
#-------------------------------------------------------------
ctx.clear()
print('\n### Top height map Image ###')
print('drawing... ', end='', flush=True)
ctx.render_layer(copper, settings=hmap_settings, bgsettings=hmapbg_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
path = 'outputs-pico/pcb-top-hmap.png'
ctx.dump(path)
image = Image.open(path)
image.filter(ImageFilter.GaussianBlur(blur_radius)).save(path)
print(' end', flush=True)

#-------------------------------------------------------------
# generate top mask image
#-------------------------------------------------------------
ctx.clear()
print('\n### Top Mask Image ###')
print('loading... ', end='', flush=True)
mask = load_layer(prefix + '.GTS')
print(' end', flush=True)

print('drawing... ', end='', flush=True)
ctx.render_layer(mask, settings=mask_settings, bgsettings=maskbg_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
ctx.dump('outputs-pico/pcb-top-mask.png')
print(' end', flush=True)

#-------------------------------------------------------------
# generate bottom base image
#-------------------------------------------------------------
ctx.clear()
print('\n### Bottom Base Image ###')
print('loading... ', end='', flush=True)
copper = load_layer(prefix + '.GBL')
silk = load_layer(prefix + '.GBO')
print(' end', flush=True)

print('drawing... ', end='', flush=True)
ctx.render_layer(copper, settings=metal_settings, bgsettings=bg_settings)
ctx.render_layer(silk, settings=silk_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
ctx.dump('outputs-pico/pcb-bottom-base.png')
print(' end', flush=True)

#-------------------------------------------------------------
# generate bottom height map image
#-------------------------------------------------------------
ctx.clear()
print('\n### Bottom height map Image ###')
print('drawing... ', end='', flush=True)
ctx.render_layer(copper, settings=hmap_settings, bgsettings=hmapbg_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
path = 'outputs-pico/pcb-bottom-hmap.png'
ctx.dump(path)
image = Image.open(path)
image.filter(ImageFilter.GaussianBlur(blur_radius)).save(path)

print(' end', flush=True)

#-------------------------------------------------------------
# generate bottom mask image
#-------------------------------------------------------------
ctx.clear()
print('\n### Top Mask Image ###')
print('loading... ', end='', flush=True)
mask = load_layer(prefix + '.GBS')
print(' end', flush=True)

print('drawing... ', end='', flush=True)
ctx.render_layer(mask, settings=mask_settings, bgsettings=maskbg_settings)
print(' end', flush=True)

print('dumping... ', end='', flush=True)
ctx.dump('outputs-pico/pcb-bottom-mask.png')
print(' end', flush=True)

#print('flipping... ', end='', flush=True)
base_image = Image.open('outputs-pico/pcb-bottom-base.png')
hmap_image = Image.open('outputs-pico/pcb-bottom-hmap.png')
mask_image = Image.open('outputs-pico/pcb-bottom-mask.png')
ImageOps.mirror(base_image).save('outputs-pico/pcb-bottom-base.png')
ImageOps.mirror(hmap_image).save('outputs-pico/pcb-bottom-hmap.png')
ImageOps.mirror(mask_image).save('outputs-pico/pcb-bottom-mask.png')
