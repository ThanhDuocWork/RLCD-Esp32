#!/usr/bin/env python3
"""Convert a color image to a 1-bit C header for ST7305/RLCD assets."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover - helper script message
    print("Pillow is required: python -m pip install pillow", file=sys.stderr)
    raise


def make_identifier(value: str) -> str:
    ident = re.sub(r"[^0-9A-Za-z_]", "_", value)
    if not ident or ident[0].isdigit():
        ident = f"img_{ident}"
    return ident.lower()


def convert_image(path: Path, width: int, height: int, threshold: int, dither: bool, invert: bool) -> bytes:
    image = Image.open(path).convert("L")
    image = image.resize((width, height), Image.Resampling.LANCZOS)

    if dither:
        image = image.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
        pixels = image.convert("L")
    else:
        pixels = image.point(lambda p: 255 if p >= threshold else 0, mode="L")

    row_stride = (width + 7) // 8
    data = bytearray(row_stride * height)

    for y in range(height):
        for x in range(width):
            on = pixels.getpixel((x, y)) >= 128
            if invert:
                on = not on
            if on:
                data[y * row_stride + (x // 8)] |= 1 << (7 - (x % 8))

    return bytes(data)


def write_header(output: Path, name: str, width: int, height: int, data: bytes) -> None:
    guard = f"{name.upper()}_H"
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        f"#define {name.upper()}_WIDTH {width}",
        f"#define {name.upper()}_HEIGHT {height}",
        f"#define {name.upper()}_DATA_SIZE {len(data)}",
        "",
        f"static const uint8_t {name}_data[{len(data)}] = {{",
    ]

    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        values = ", ".join(f"0x{byte:02X}" for byte in chunk)
        lines.append(f"    {values},")

    lines.extend([
        "};",
        "",
    ])

    output.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert color images to row-major 1bpp C headers.")
    parser.add_argument("input", type=Path, help="Input image path, for example PNG/JPG/BMP")
    parser.add_argument("output", type=Path, help="Output .h path")
    parser.add_argument("--width", type=int, default=300, help="Output width, default 300")
    parser.add_argument("--height", type=int, default=400, help="Output height, default 400")
    parser.add_argument("--threshold", type=int, default=128, help="Black/white threshold for non-dither mode")
    parser.add_argument("--dither", action="store_true", help="Use Floyd-Steinberg dithering")
    parser.add_argument("--invert", action="store_true", help="Invert output bits")
    parser.add_argument("--name", help="C symbol base name. Defaults to output filename")
    args = parser.parse_args()

    if args.width <= 0 or args.height <= 0:
        parser.error("width and height must be positive")
    if not 0 <= args.threshold <= 255:
        parser.error("threshold must be between 0 and 255")

    name = make_identifier(args.name or args.output.stem)
    data = convert_image(args.input, args.width, args.height, args.threshold, args.dither, args.invert)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_header(args.output, name, args.width, args.height, data)
    print(f"Wrote {args.output} ({args.width}x{args.height}, {len(data)} bytes, symbol {name}_data)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
