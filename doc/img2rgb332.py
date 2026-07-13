#!/usr/bin/env python3
"""
Convert an image to a C header file containing a RGB332 (8bpp) pixel array
for use with RP2350 HSTX / picodvi style framebuffers.

Usage:
    python img2rgb332.py input.png output.h [array_name] [--width W] [--height H]

Pixel format: RRRGGGBB
    R (3 bits) -> bits 7..5
    G (3 bits) -> bits 4..2
    B (2 bits) -> bits 1..0
"""

import sys
import argparse
from PIL import Image


def rgb888_to_rgb332(r, g, b):
    r3 = (r >> 5) & 0x07   # top 3 bits of R
    g3 = (g >> 5) & 0x07   # top 3 bits of G
    b2 = (b >> 6) & 0x03   # top 2 bits of B
    return (r3 << 5) | (g3 << 2) | b2


def convert_image(input_path, width=None, height=None):
    img = Image.open(input_path).convert("RGB")
    if width or height:
        w = width or img.width
        h = height or img.height
        img = img.resize((w, h), Image.LANCZOS)

    w, h = img.size
    pixels = list(img.getdata())

    data = bytearray(w * h)
    for i, (r, g, b) in enumerate(pixels):
        data[i] = rgb888_to_rgb332(r, g, b)

    return data, w, h


def write_header(data, w, h, array_name, output_path):
    with open(output_path, "w") as f:
        f.write(f"// Auto-generated RGB332 (8bpp) image data\n")
        f.write(f"// Format: RRRGGGBB (R bits 7..5, G bits 4..2, B bits 1..0)\n")
        f.write(f"// Source resolution: {w}x{h}\n")
        f.write("#pragma once\n\n")
        f.write(f"#define {array_name.upper()}_WIDTH  {w}\n")
        f.write(f"#define {array_name.upper()}_HEIGHT {h}\n\n")
        f.write(
            f'static char __attribute__((aligned(4), section(".data"))) '
            f'{array_name}[{w * h}] = {{\n'
        )

        bytes_per_line = 16
        for i in range(0, len(data), bytes_per_line):
            chunk = data[i:i + bytes_per_line]
            line = ", ".join(f"0x{b:02x}" for b in chunk)
            f.write(f"    {line},\n")

        f.write("};\n")


def main():
    parser = argparse.ArgumentParser(description="Convert image to RGB332 C array (.h file)")
    parser.add_argument("input", help="Input image file (png, jpg, bmp, ...)")
    parser.add_argument("output", help="Output .h file")
    parser.add_argument("array_name", nargs="?", default="pic_name", help="C array variable name")
    parser.add_argument("--width", type=int, default=None, help="Resize to this width")
    parser.add_argument("--height", type=int, default=None, help="Resize to this height")
    args = parser.parse_args()

    data, w, h = convert_image(args.input, args.width, args.height)
    write_header(data, w, h, args.array_name, args.output)

    print(f"Wrote {w}x{h} = {w*h} bytes to {args.output} as array '{args.array_name}'")


if __name__ == "__main__":
    main()
