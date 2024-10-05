#!/usr/bin/env python3
import os
import sys
import csv
from gerberex.utility import rotate

TOP='Top'
BOTTOM='Bottom'

def normalize_angle(angle):
    while angle >= 360:
        angle -= 360
    while angle < 0:
        angle += 360
    return angle

class Component:
    def __init__(self, name, position, rotation, layer):
        self.name = name
        self.position = position
        self.rotation = rotation
        self.angle_offset = 0
        self.shift_offset = [0, 0]
        self.layer = layer

    def offset(self, dx, dy):
        self.position = (self.position[0] + dx, self.position[1] + dy)

    def rotate(self, angle, center=(0,0)):
        self.rotation = normalize_angle(self.rotation + angle)
        self.position = rotate(self.position[0], self.position[1],
                               angle, center)

    def str(self):
        angle = normalize_angle(self.rotation + self.angle_offset)
        shift = rotate(self.shift_offset[0], self.shift_offset[1],
                       angle, (0,0))
        return '{0},{1}mm,{2}mm,{3},{4}'.format(
            self.name,
            self.position[0] + shift[0],
            self.position[1] + shift[1],
            self.layer, angle)

class MountFile:
    def __init__(self):
        self.components = []

    def load(self, path, layer, exclude={}):
        self.components = []
        with open(path, 'r') as file:
            self.components = []
            for line in file:
                data = line.replace('\n', '').split()
                if not exclude or not data[0] in exclude:
                    c = Component(
                        data[0],
                        (float(data[1]), float(data[2])),
                        float(data[3]),
                        layer)
                    self.components.append(c)

    def offset(self, dx, dy):
        for c in self.components:
            c.offset(dx, dy)

    def rotate(self, angle, center=(0,0)):
        for c in self.components:
            c.rotate(angle, center)


class Composition:
    def __init__(self):
        self.restrictions = {}
        self.mountfiles = []

    def setBom(self, path, angles = [], offsets = []):
        with open(path, 'r') as file:
            bom = csv.reader(file)
            title = True
            for data in bom:
                if title:
                    title = False
                else:
                    names = data[1].split(',')
                    footprint = data[2]
                    angle = angles[footprint] if footprint in angles else 0
                    offset = offsets[footprint] if footprint in offsets else [0, 0]
                    for name in names:
                        self.restrictions[name.replace(' ', '')] = [angle, offset]

    def merge(self, file):
        self.mountfiles.append(file)

    def dump(self, path):
        with open(path, 'w') as out:
            out.write('Designator,Mid X,Mid Y,Layer,Rotation\n')
            for f in self.mountfiles:
                for c in f.components:
                    if not self.restrictions or c.name in self.restrictions:
                        c.angle_offset = self.restrictions[c.name][0]
                        c.shift_offset = self.restrictions[c.name][1]
                        out.write('{}\n'.format(c.str()))
