#!/usr/bin/env python3
"""Prepare TF-card BMP images from PNG/JPG/BMP inputs."""

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


SUPPORTED_EXTENSIONS = {".bmp", ".jpg", ".jpeg", ".png"}


def make_bmp_name(path: Path) -> str:
    name = re.sub(r"[^0-9A-Za-z_]+", "_", path.stem).strip("_").lower()
    if not name:
        name = "image"
    return f"{name}.bmp"


def resize_cover(image: Image.Image, width: int, height: int) -> Image.Image:
    image = image.convert("RGB")
    src_w, src_h = image.size
    scale = max(width / src_w, height / src_h)
    resized = image.resize((round(src_w * scale), round(src_h * scale)), Image.Resampling.LANCZOS)
    left = (resized.width - width) // 2
    top = (resized.height - height) // 2
    return resized.crop((left, top, left + width, top + height))


def resize_contain(image: Image.Image, width: int, height: int, background: str) -> Image.Image:
    image = image.convert("RGB")
    image.thumbnail((width, height), Image.Resampling.LANCZOS)
    output = Image.new("RGB", (width, height), background)
    output.paste(image, ((width - image.width) // 2, (height - image.height) // 2))
    return output


def convert_one(input_path: Path, output_path: Path, width: int, height: int, mode: str, background: str) -> None:
    with Image.open(input_path) as image:
        if mode == "contain":
            output = resize_contain(image, width, height, background)
        else:
            output = resize_cover(image, width, height)

        output.save(output_path, "BMP")


def iter_inputs(path: Path) -> list[Path]:
    if path.is_file():
        return [path]

    return sorted(
        item for item in path.iterdir()
        if item.is_file() and item.suffix.lower() in SUPPORTED_EXTENSIONS
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert PNG/JPG/BMP images to TF-ready BMP files.")
    parser.add_argument("input", type=Path, help="Input image file or folder")
    parser.add_argument("output", type=Path, help="Output BMP file or output folder")
    parser.add_argument("--width", type=int, default=300, help="Output width, default 300")
    parser.add_argument("--height", type=int, default=400, help="Output height, default 400")
    parser.add_argument("--mode", choices=("cover", "contain"), default="cover", help="cover crops, contain pads")
    parser.add_argument("--background", default="white", help="Background color for contain mode")
    args = parser.parse_args()

    if args.width <= 0 or args.height <= 0:
        parser.error("width and height must be positive")

    inputs = iter_inputs(args.input)
    if not inputs:
        parser.error(f"no supported images found in {args.input}")

    output_is_file = len(inputs) == 1 and args.output.suffix.lower() == ".bmp"
    if not output_is_file:
        args.output.mkdir(parents=True, exist_ok=True)

    for input_path in inputs:
        if input_path.suffix.lower() not in SUPPORTED_EXTENSIONS:
            print(f"Skip unsupported file: {input_path}")
            continue

        output_path = args.output if output_is_file else args.output / make_bmp_name(input_path)
        convert_one(input_path, output_path, args.width, args.height, args.mode, args.background)
        print(f"{input_path} -> {output_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
