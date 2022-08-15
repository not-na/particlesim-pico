#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
from pathlib import Path
import argparse
from typing import Tuple, List

import png

VERSION_STR = "0.2.0"

DEFAULT_SIZE = 32
MAX_PARTICLES = 512

HEADER_TEMPLATE = f"""#pragma once

#include "pico/stdlib.h"

// WARNING: This file has been autogenerated, do not edit directly!
// Generated by png_to_header.py {VERSION_STR}

#define {{name}}_PARTICLE_COUNT {{particlecount}}

const uint32_t {{name}}[] = {{{{
{{data}}
}}}};

// Stored as x1, y1, color1, x2, y2, color2, ...
// Could be a 2D Array, but the type of the list of different particle sets caused issues
const uint32_t {{name}}_PARTICLES[{{name}}_PARTICLE_COUNT*3] = {{{{
{{particles}}}}}};
"""

DATA_LINE_PREFIX = b"    "

PARTICLE_ENTRY = "    {x}, {y}, 0x{color:08x},\n"


GAMMA = 2.4


def gamma(s: float) -> float:
    # Based on https://stackoverflow.com/a/61138576
    # TODO: find right values for LED matrix panels
    if s <= 0.04045:
        return s / 12.92
    else:
        return ((s+0.055)/1.055)**2.4


def gamma8b(s: int) -> int:
    return int(255*gamma(s/255))


def convert_png(filename: str, size: int, gamma_correct: bool) -> Tuple[str, List[Tuple[int, int, int]]]:
    r = png.Reader(filename=filename)
    w, h, row, info = r.asRGBA8()

    if w != size or h != size:
        print(f"Expected PNG of size {size}x{size}, but got {w}x{h} instead!")
        raise ValueError("Invalid size")

    dat = bytearray(DATA_LINE_PREFIX)

    particles = []

    x = 0
    y = 0
    for ro in row:
        # Iterate four at a time, based on https://stackoverflow.com/a/3415150
        ir = iter(ro)
        for c in zip(ir, ir, ir, ir):
            r, g, b, a = c

            if gamma_correct:
                r = gamma8b(r)
                g = gamma8b(g)
                b = gamma8b(b)

            cn = r << 0 | g << 8 | b << 16

            if a != 0xFF:
                # Particle
                particles.append((x, y, cn))
                cn = 0  # Remove from static background

            dat.extend(bytearray(f"0x{cn:08X}, ", encoding="ascii"))

            x += 1

        dat.extend(b"\n")
        dat.extend(DATA_LINE_PREFIX)

        y += 1
        x = 0

    return str(dat.decode()), particles


def convert_file(ifile: str, ofile: str, size: int, gamma_correct: bool, name=None) -> None:
    ifile = os.path.abspath(ifile)
    ofile = os.path.abspath(ofile)

    if name is None:
        name = ".".join(os.path.basename(ifile).split(".")[0:-1])
        name = name.replace(".", "_").replace("-", "_").replace(" ", "_").upper()

    data, particles = convert_png(ifile, size, gamma_correct)

    if len(particles) > MAX_PARTICLES:
        raise ValueError(f"Got {particles} particles, but only {MAX_PARTICLES} are supported!"
                         f"\nPlease check your image transparency layer.")

    out = HEADER_TEMPLATE.format(
        name=name,
        data=data,
        particlecount=len(particles),
        particles="".join([PARTICLE_ENTRY.format(x=x, y=y, color=color) for x, y, color in particles])
    )

    with open(ofile, "w") as f:
        f.write(out)


def conv_stages(args):
    if args.directory is not None:
        files = os.listdir(args.directory)
        headers = []
        for fname in files:
            if fname.endswith(".png"):
                print(f"Converting {fname}")
                headers.append(fname.replace(".png", ".h"))
                fname = os.path.join(args.directory, fname)
                convert_file(fname, fname.replace(".png", ".h"), args.size, args.gamma)

        headers.sort()
        stages_include = f"// Autogenerated by png_to_header.py v{VERSION_STR}\n// Do not edit manually!\n\n"
        stages_include += "\n".join(f"#include \"{header}\"" for header in headers)
        with open(os.path.join(args.directory, "img_all.h"), "w") as f:
            f.write(stages_include)

    else:
        # Single file only
        convert_file(args.file, args.file.replace(".png", ".h"), args.size, args.gamma)


GOL_TEMPLATE = f"""#pragma once

#include "pico/stdlib.h"

// WARNING: This file has been autogenerated, do not edit directly!
// Generated by png_to_header.py {VERSION_STR}

const uint8_t {{name}}[] = {{{{
{{data}}
}}}};
"""

GOL_LINE_PREFIX = b"    "


def convert_golfile(ifile: Path, ofile: Path, size: int, name=None) -> None:
    if name is None:
        name = ifile.stem
        name = name.replace(".", "_").replace("-", "_").replace(" ", "_").upper()

    # Read in PNG
    r = png.Reader(filename=ifile)
    w, h, row, info = r.asRGBA8()

    if w != size or h != size:
        print(f"Expected PNG of size {size}x{size}, but got {w}x{h} instead!")
        raise ValueError("Invalid size")

    dat = bytearray(GOL_LINE_PREFIX)

    x = 0
    y = 0
    for ro in row:
        # Iterate four at a time, based on https://stackoverflow.com/a/3415150
        ir = iter(ro)
        for c in zip(ir, ir, ir, ir):
            r, g, b, a = c

            byte = 0
            if r >= 0x80:
                byte = 0xFF

            dat.extend(bytearray(f"0x{byte:02X}, ", encoding="ascii"))

            x += 1

        dat.extend(b"\n")
        dat.extend(GOL_LINE_PREFIX)

        y += 1
        x = 0

    out = GOL_TEMPLATE.format(
        name=name,
        data=dat.decode(),
    )

    ofile.write_text(out)


def conv_gol(args):
    if args.directory is not None:
        headers = []
        meta_header = (args.directory / "gol_all.h").resolve()

        for f in args.directory.glob("*.png"):
            print(f"Converting {f.name}")
            convert_golfile(f, f.with_suffix(".h"), args.size)
            headers.append(f.with_suffix(".h").name)

        headers.sort()
        gol_include = f"// Autogenerated by png_to_header.py v{VERSION_STR}\n// Do not edit manually!\n\n"
        gol_include += "\n".join(f"#include \"{header}\"" for header in headers)
        meta_header.write_text(gol_include)
    else:
        # Single file only
        convert_golfile(args.file, args.file.with_suffix(".h"), args.size)


def conv_auto(args):
    args.gamma = True  # Force gamma on

    # Compile stages
    print("Compiling stages...")
    args.directory = "images/"
    conv_stages(args)

    # Compile universes
    print("Compiling universes...")
    args.directory = Path("gol/")
    conv_gol(args)


if __name__ == "__main__":
    if not (Path.cwd() / "particlesim.cpp").exists():
        print("Please make sure to run this script from the directory where particlesim.cpp is located")
        sys.exit(1)

    parser = argparse.ArgumentParser(description=f"Convert PNG files to headers\nVersion {VERSION_STR}")
    parser.add_argument("--size", action="store", default=DEFAULT_SIZE)
    subparsers = parser.add_subparsers(required=True)

    parser_stages = subparsers.add_parser("stages", description="Compile stages")
    parser_stages.add_argument("--gamma", action="store_true")

    stages_dirfile = parser_stages.add_mutually_exclusive_group(required=True)
    stages_dirfile.add_argument("--directory", action="store", default=None)
    stages_dirfile.add_argument("--file", action="store")

    parser_gol = subparsers.add_parser("gol", description="Compile Game of Life initial universes")

    gol_dirfile = parser_gol.add_mutually_exclusive_group(required=True)
    gol_dirfile.add_argument("--directory", action="store", default=None, type=Path)
    gol_dirfile.add_argument("--file", action="store", type=Path)

    parser_auto = subparsers.add_parser("auto", description="Auto-compile everything")

    parser_stages.set_defaults(func=conv_stages)
    parser_gol.set_defaults(func=conv_gol)
    parser_auto.set_defaults(func=conv_auto)

    args = parser.parse_args()
    args.func(args)
