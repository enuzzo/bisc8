#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_SOURCE = Path("assets/grimorio.txt")
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


def generate_header(fortunes: list[str]) -> str:
    entries = "\n".join(f'    "{_escape_c_string(fortune)}",' for fortune in fortunes)
    return f"""#pragma once

#include <stddef.h>

namespace bisc8 {{

static constexpr const char *kFortunes[] = {{
{entries}
}};

static constexpr size_t kFortuneCount = {len(fortunes)};

}}  // namespace bisc8
"""


def write_header(source: Path, output: Path) -> None:
    fortunes = parse_fortunes(source)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generate_header(fortunes), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Bisc8 firmware fortune data.")
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    write_header(args.source, args.output)
    print(f"Generated {args.output} from {args.source}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
