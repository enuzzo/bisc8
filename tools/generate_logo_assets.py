#!/usr/bin/env python3
"""Generate the Bisc8 boot logo firmware image from the logo PNG."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "assets/logo/logo_min.png"
DEFAULT_OUT_DIR = ROOT / "firmware/bisc8_fortune/main/generated"
SIZE = 96
THRESHOLD = 210
WHITE_RGB565 = (0xFF, 0xFF)
BLACK_RGB565 = (0x00, 0x00)


def run(command: list[str], *, binary: bool = False) -> bytes | str:
    completed = subprocess.run(command, check=True, capture_output=True)
    if binary:
        return completed.stdout
    return completed.stdout.decode("utf-8").strip()


def read_scaled_grayscale(source: Path) -> bytes:
    data = run(
        [
            "ffmpeg",
            "-v",
            "error",
            "-i",
            str(source),
            "-vf",
            f"scale={SIZE}:{SIZE}:flags=lanczos,format=gray",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "gray",
            "-",
        ],
        binary=True,
    )
    if len(data) != SIZE * SIZE:
        raise ValueError(f"expected {SIZE * SIZE} grayscale bytes, got {len(data)}")
    return data


def threshold_pixels(gray: bytes) -> list[int]:
    return [1 if value < THRESHOLD else 0 for value in gray]


def to_rgb565_bytes(bits: list[int]) -> bytes:
    out = bytearray()
    for bit in bits:
        out.extend(BLACK_RGB565 if bit else WHITE_RGB565)
    return bytes(out)


def c_array(data: bytes) -> str:
    lines = []
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    return "\n".join(lines)


def write_assets(out_dir: Path, rgb565: bytes) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    header = """#pragma once

#include <lvgl.h>

namespace bisc8 {

extern const lv_image_dsc_t kBisc8BootLogo;

}  // namespace bisc8
"""
    (out_dir / "logo_assets.h").write_text(header, encoding="utf-8")

    source = f"""#include "generated/logo_assets.h"

namespace bisc8 {{
namespace {{

alignas(4) const uint8_t kBisc8BootLogoData[] = {{
{c_array(rgb565)}
}};

}}  // namespace

const lv_image_dsc_t kBisc8BootLogo = {{
    {{LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_RGB565, 0, {SIZE}, {SIZE}, {SIZE * 2}, 0}},
    sizeof(kBisc8BootLogoData),
    kBisc8BootLogoData,
    nullptr,
    nullptr,
}};

}}  // namespace bisc8
"""
    (out_dir / "logo_assets.cpp").write_text(source, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    args = parser.parse_args()

    if not args.source.exists():
        raise FileNotFoundError(args.source)

    gray = read_scaled_grayscale(args.source)
    bits = threshold_pixels(gray)
    rgb565 = to_rgb565_bytes(bits)
    write_assets(args.out_dir, rgb565)
    black_pixels = sum(bits)
    print(f"logo: {args.source} -> {SIZE}x{SIZE}, threshold={THRESHOLD}, black_pixels={black_pixels}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
