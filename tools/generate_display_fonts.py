#!/usr/bin/env python3
"""Generate Bisc8 LVGL display fonts (Pixelify Sans, 1-bit, Latin-1).

The device and the web config share the Pixelify Sans pixel typeface. These
are rendered at 1 bit per pixel so they stay crisp on the 1-bit e-paper (no
anti-aliasing to threshold away), matching the Macintosh System 6 look.
"""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / "assets/fonts/PixelifySans-Regular.ttf"
OUT_DIR = ROOT / "firmware/bisc8_fortune/components/externlib/ui_res"
LATIN_1_RANGE = "0x20-0xFF"


@dataclass(frozen=True)
class FontSpec:
    name: str
    size: int
    output: str


FONTS = (
    FontSpec("bisc8_font_small", 16, "bisc8_font_small.c"),
    FontSpec("bisc8_font_body", 20, "bisc8_font_body.c"),
    FontSpec("bisc8_font_title", 30, "bisc8_font_title.c"),
)

# Older Montserrat font units to remove so the ui_res glob stops compiling them.
STALE = ("bisc8_font_body_16.c", "bisc8_font_ui_14.c", "bisc8_font_title_25.c")


def generate(spec: FontSpec) -> None:
    output = OUT_DIR / spec.output
    command = [
        "npx",
        "--yes",
        "lv_font_conv@1.5.1",
        "--no-compress",
        "--no-prefilter",
        "--bpp",
        "1",
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
    for stale in STALE:
        (OUT_DIR / stale).unlink(missing_ok=True)
    for spec in FONTS:
        generate(spec)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
