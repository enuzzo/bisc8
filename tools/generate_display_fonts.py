#!/usr/bin/env python3
"""Generate Bisc8 LVGL display fonts with Latin-1 glyph coverage."""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / "firmware/bisc8_fortune/managed_components/lvgl__lvgl/scripts/built_in_font/Montserrat-Medium.ttf"
OUT_DIR = ROOT / "firmware/bisc8_fortune/components/externlib/ui_res"
LATIN_1_RANGE = "0x20-0xFF"


@dataclass(frozen=True)
class FontSpec:
    name: str
    size: int
    output: str


FONTS = (
    FontSpec("bisc8_font_body_16", 16, "bisc8_font_body_16.c"),
    FontSpec("bisc8_font_ui_14", 14, "bisc8_font_ui_14.c"),
    FontSpec("bisc8_font_title_25", 25, "bisc8_font_title_25.c"),
)


def generate(spec: FontSpec) -> None:
    output = OUT_DIR / spec.output
    command = [
        "npx",
        "--yes",
        "lv_font_conv@1.5.1",
        "--no-compress",
        "--no-prefilter",
        "--bpp",
        "4",
        "--size",
        str(spec.size),
        "--font",
        str(FONT),
        "-r",
        LATIN_1_RANGE,
        "--format",
        "lvgl",
        "--lv-include",
        "lvgl.h",
        "-o",
        str(output),
        "--force-fast-kern-format",
    ]
    subprocess.run(command, check=True)
    normalize_generated_comment(output)


def normalize_generated_comment(output: Path) -> None:
    source = output.read_text(encoding="utf-8")
    source = source.replace(str(ROOT) + "/", "")
    output.write_text(source, encoding="utf-8")


def main() -> int:
    if not FONT.exists():
        raise FileNotFoundError(FONT)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for spec in FONTS:
        generate(spec)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
