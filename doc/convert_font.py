#!/usr/bin/env python3
import argparse
import pathlib
import re
import sys

from PIL import Image, ImageDraw, ImageFont


ASCII_FIRST = 32
ASCII_LAST = 126
SIZES = [32, 48, 64]

FONT_BPP = 2
PIXELS_PER_BYTE = 8 // FONT_BPP     # 4 pixels/byte for 2 bpp


def sanitize_name(name: str) -> str:
    name = re.sub(r"[^0-9a-zA-Z_]", "_", name)

    if not name:
        name = "font"

    if name[0].isdigit():
        name = "_" + name

    return name


def bytes_per_row(width: int) -> int:
    """Number of bytes needed for one 2-bpp glyph row."""
    return (width + PIXELS_PER_BYTE - 1) // PIXELS_PER_BYTE


def pack_row_2bpp(pixels):
    """
    Pack four 2-bpp pixels per byte, MSB first.

    pixels [p0, p1, p2, p3] become:
    byte = p0<<6 | p1<<4 | p2<<2 | p3
    """
    packed = []
    value = 0
    count = 0

    for pixel in pixels:
        value = (value << 2) | (pixel & 0x03)
        count += 1

        if count == PIXELS_PER_BYTE:
            packed.append(value)
            value = 0
            count = 0

    # Pad unused pixels at the right side with zero alpha.
    if count:
        value <<= 2 * (PIXELS_PER_BYTE - count)
        packed.append(value)

    return packed


def grayscale_to_2bpp(value: int) -> int:
    """
    Map Pillow grayscale coverage 0..255 to alpha 0..3.

    0       -> 0
    85      -> 1
    170     -> 2
    255     -> 3
    """
    return (value * 3 + 127) // 255


def render_glyph(font, ch: str, font_height: int):
    advance = int(round(font.getlength(ch)))

    if advance <= 0:
        advance = max(1, font_height // 4)

    # Extra room lets Pillow render glyphs with overhang safely.
    canvas_w = max(advance + 8, font_height * 2)
    canvas_h = font_height

    image = Image.new("L", (canvas_w, canvas_h), 0)
    draw = ImageDraw.Draw(image)

    bbox = font.getbbox(ch)

    if bbox is None:
        bbox = (0, 0, 0, 0)

    left, top, right, bottom = bbox

    # Keep the leftmost glyph pixel at x = 0 when it has a negative bearing.
    draw_x = -left
    draw_y = 0

    draw.text((draw_x, draw_y), ch, fill=255, font=font)

    # Bitmap width follows advance. This matches the existing C layout.
    glyph_w = max(1, advance)
    glyph = image.crop((0, 0, glyph_w, canvas_h))

    packed = []

    for y in range(canvas_h):
        row_pixels = []

        for x in range(glyph_w):
            coverage_8bpp = glyph.getpixel((x, y))
            coverage_2bpp = grayscale_to_2bpp(coverage_8bpp)
            row_pixels.append(coverage_2bpp)

        packed.extend(pack_row_2bpp(row_pixels))

    return glyph_w, advance, packed


def format_u8_array(data, cols=16):
    lines = []

    for i in range(0, len(data), cols):
        chunk = data[i:i + cols]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk))

    return ",\n".join(lines) if lines else "    0x00"


def format_u16_array(data, cols=10):
    lines = []

    for i in range(0, len(data), cols):
        chunk = data[i:i + cols]
        lines.append("    " + ", ".join(str(value) for value in chunk))

    return ",\n".join(lines) if lines else "    0"


def write_header(out_path, symbol_name, pixel_height,
                 glyph_offsets, glyph_widths, glyph_advances, bitmap):

    safe_symbol = sanitize_name(symbol_name)
    upper_symbol = safe_symbol.upper()

    guard = upper_symbol + "_H"
    row_bytes_macro = upper_symbol + "_MAX_ROW_BYTES"

    max_width = max(glyph_widths) if glyph_widths else 1
    max_row_bytes = bytes_per_row(max_width)

    text = f"""\
#ifndef {guard}
#define {guard}

#include <stdint.h>

#ifndef BITMAP_FONT_T_DEFINED
#define BITMAP_FONT_T_DEFINED

typedef struct
{{
    uint8_t height;
    uint8_t first_char;
    uint8_t last_char;
    uint8_t bpp;
    const uint16_t *offsets;
    const uint8_t  *widths;
    const uint8_t  *advances;
    const uint8_t  *bitmap;
}} bitmap_font_t;

#endif

#define {upper_symbol}_HEIGHT        {pixel_height}
#define {upper_symbol}_BPP           {FONT_BPP}
#define {row_bytes_macro} {max_row_bytes}

static const uint16_t {safe_symbol}_offsets[{len(glyph_offsets)}] =
{{
{format_u16_array(glyph_offsets)}
}};

static const uint8_t {safe_symbol}_widths[{len(glyph_widths)}] =
{{
{format_u8_array(glyph_widths)}
}};

static const uint8_t {safe_symbol}_advances[{len(glyph_advances)}] =
{{
{format_u8_array(glyph_advances)}
}};

static const uint8_t {safe_symbol}_bitmap[{len(bitmap)}] =
{{
{format_u8_array(bitmap)}
}};

static const bitmap_font_t {safe_symbol} =
{{
    {pixel_height},
    {ASCII_FIRST},
    {ASCII_LAST},
    {FONT_BPP},
    {safe_symbol}_offsets,
    {safe_symbol}_widths,
    {safe_symbol}_advances,
    {safe_symbol}_bitmap
}};

#endif /* {guard} */
"""

    out_path.write_text(text, encoding="utf-8")


def generate_font(font_path, out_dir, base_name, pixel_height):
    print(f"Generating {pixel_height}px 2-bpp font from {font_path}")

    font = ImageFont.truetype(str(font_path), pixel_height)

    offsets = []
    widths = []
    advances = []
    bitmap = []

    for code in range(ASCII_FIRST, ASCII_LAST + 1):
        ch = chr(code)

        glyph_w, advance, packed = render_glyph(font, ch, pixel_height)

        # This is a byte offset, exactly as expected by draw_char().
        offsets.append(len(bitmap))
        widths.append(glyph_w)
        advances.append(advance)
        bitmap.extend(packed)

    symbol_name = f"{sanitize_name(base_name)}_{pixel_height}"
    out_path = out_dir / f"{symbol_name}.h"

    write_header(
        out_path,
        symbol_name,
        pixel_height,
        offsets,
        widths,
        advances,
        bitmap,
    )

    print(
        f"Wrote {out_path} "
        f"({len(bitmap)} bitmap bytes, {FONT_BPP} bpp)"
    )


def main():
    parser = argparse.ArgumentParser(
        description="Convert a TTF font into 2-bpp anti-aliased C font headers."
    )

    parser.add_argument("font", help="Path to a TTF or OTF font file")
    parser.add_argument(
        "-o",
        "--outdir",
        default="generated_fonts",
        help="Output directory",
    )
    parser.add_argument(
        "-n",
        "--name",
        default="vga_font",
        help="Base output symbol/file name",
    )

    args = parser.parse_args()

    font_path = pathlib.Path(args.font)

    if not font_path.is_file():
        print(f"ERROR: font file not found: {font_path}", file=sys.stderr)
        return 1

    out_dir = pathlib.Path(args.outdir)
    out_dir.mkdir(parents=True, exist_ok=True)

    for size in SIZES:
        generate_font(font_path, out_dir, args.name, size)

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
