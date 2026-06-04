#!/usr/bin/env python3
"""Generate small PCM firmware sound assets from candidate audio files."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT_DIR = ROOT / "firmware/bisc8_fortune/main/generated"
DEFAULT_PREVIEW_DIR = ROOT / "assets/sounds/firmware"
SAMPLE_RATE = 16000
CHANNELS = 2


@dataclass(frozen=True)
class Cue:
    name: str
    symbol: str
    source: Path
    gain: float


CUES = (
    Cue("boot", "kSoundBoot", ROOT / "assets/candidate/full-reboot.mp3", 0.58),
    Cue("oracle_button", "kSoundOracleButton", ROOT / "assets/candidate/oracle-button.ogg", 0.68),
    Cue("voice_submit", "kSoundVoiceSubmit", ROOT / "assets/candidate/oracle-audio-before.ogg", 0.68),
    Cue("shutdown", "kSoundShutdown", ROOT / "assets/candidate/shutdown.ogg", 0.62),
)


def run(command: list[str]) -> str:
    completed = subprocess.run(command, check=True, text=True, capture_output=True)
    return completed.stdout.strip()


def duration_seconds(path: Path) -> float:
    value = run([
        "ffprobe",
        "-v",
        "error",
        "-show_entries",
        "format=duration",
        "-of",
        "default=noprint_wrappers=1:nokey=1",
        str(path),
    ])
    return float(value)


def convert_to_raw(cue: Cue, raw_path: Path, wav_path: Path) -> float:
    duration = duration_seconds(cue.source)
    fade_out_start = max(0.0, duration - 0.08)
    audio_filter = (
        f"volume={cue.gain},"
        "afade=t=in:st=0:d=0.015,"
        f"afade=t=out:st={fade_out_start:.3f}:d=0.08"
    )
    base = [
        "ffmpeg",
        "-v",
        "error",
        "-y",
        "-i",
        str(cue.source),
        "-ac",
        str(CHANNELS),
        "-ar",
        str(SAMPLE_RATE),
        "-af",
        audio_filter,
    ]
    run(base + ["-f", "s16le", str(raw_path)])
    run(base + ["-sample_fmt", "s16", str(wav_path)])
    return duration


def c_array(data: bytes) -> str:
    lines = []
    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--preview-dir", type=Path, default=DEFAULT_PREVIEW_DIR)
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    args.preview_dir.mkdir(parents=True, exist_ok=True)

    generated = []
    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        for cue in CUES:
            if not cue.source.exists():
                raise FileNotFoundError(cue.source)
            raw_path = tmp_dir / f"{cue.name}.s16le"
            wav_path = args.preview_dir / f"{cue.name}.wav"
            duration = convert_to_raw(cue, raw_path, wav_path)
            data = raw_path.read_bytes()
            generated.append((cue, duration, data, wav_path))

    header = """#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bisc8 {

struct SoundAsset {
    const uint8_t *data;
    size_t bytes;
    const char *name;
};

extern const SoundAsset kSoundBoot;
extern const SoundAsset kSoundOracleButton;
extern const SoundAsset kSoundVoiceSubmit;
extern const SoundAsset kSoundShutdown;

}  // namespace bisc8
"""
    (args.out_dir / "sound_assets.h").write_text(header, encoding="utf-8")

    parts = [
        '#include "generated/sound_assets.h"',
        "",
        "namespace bisc8 {",
        "namespace {",
        "",
    ]
    for cue, _duration, data, _wav_path in generated:
        parts.append(f"alignas(4) const uint8_t {cue.symbol}Data[] = {{")
        parts.append(c_array(data))
        parts.append("};")
        parts.append("")
    parts.append("}  // namespace")
    parts.append("")
    for cue, duration, data, _wav_path in generated:
        parts.append(
            f'const SoundAsset {cue.symbol} = '
            f'{{{cue.symbol}Data, sizeof({cue.symbol}Data), "{cue.name}"}};'
        )
        print(f"{cue.name}: {duration:.3f}s, {len(data)} bytes")
    parts.append("")
    parts.append("}  // namespace bisc8")
    parts.append("")
    (args.out_dir / "sound_assets.cpp").write_text("\n".join(parts), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
