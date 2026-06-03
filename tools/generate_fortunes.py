#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_SOURCE = Path("assets/grimorio.txt")
DEFAULT_SOURCES_BY_LANGUAGE = {
    "it": Path("assets/grimorio.txt"),
    "en": Path("assets/grimorio.en.txt"),
    "es": Path("assets/grimorio.es.txt"),
}
DEFAULT_OUTPUT = Path("firmware/bisc8_fortune/main/generated/fortune_data.h")
MAX_FORTUNE_CHARS = 120


def parse_fortunes(path: Path, max_chars: int = MAX_FORTUNE_CHARS) -> list[str]:
    fortunes: list[str] = []
    for line_no, raw_line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), start=1):
        line = raw_line.strip()
        if not line:
            continue
        if len(line) > max_chars:
            raise ValueError(f"{path} line {line_no} is {len(line)} chars; hard max is {max_chars}")
        fortunes.append(line)
    if not fortunes:
        raise ValueError(f"{path} does not contain any fortune lines")
    return fortunes


def _escape_c_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def _symbol_suffix(language: str) -> str:
    return "" if language == "it" else f"_{language}"


def generate_header(fortunes: list[str], language: str = "it") -> str:
    suffix = _symbol_suffix(language)
    entries = "\n".join(f'    "{_escape_c_string(fortune)}",' for fortune in fortunes)
    return f"""#pragma once

#include <stddef.h>

namespace bisc8 {{

static constexpr const char *kFortunes{suffix}[] = {{
{entries}
}};

static constexpr size_t kFortuneCount{suffix} = {len(fortunes)};
static constexpr char kFortuneLanguage{suffix}[] = "{language}";

}}  // namespace bisc8
"""


def write_header(source: Path, output: Path, language: str = "it") -> None:
    fortunes = parse_fortunes(source)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generate_header(fortunes, language=language), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Bisc8 firmware fortune data.")
    parser.add_argument("--language", choices=sorted(DEFAULT_SOURCES_BY_LANGUAGE), default="it")
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    if args.source == DEFAULT_SOURCE and args.language != "it":
        args.source = DEFAULT_SOURCES_BY_LANGUAGE[args.language]
    write_header(args.source, args.output, language=args.language)
    print(f"Generated {args.output} from {args.source}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
