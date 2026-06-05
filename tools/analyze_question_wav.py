#!/usr/bin/env python3
"""Diagnose a Bisc8 question recording (the emailed domanda.wav).

Why: the audio Bisc8 sends to speech-to-text is intermittently gibberish even
though the early record->playback loopback sounded clean. Clipping (mic gain too
hot), a too-weak/far signal, dropouts (codec/flash stalls dropping samples) and a
wrong real sample rate all sound "bad" but need OPPOSITE fixes. This tool reads
the actual WAV and measures which one it is -- the desktop twin of the on-device
[VOICEDIAG] log line -- so the fix targets the real cause instead of a guess.

Usage:
    python3 tools/analyze_question_wav.py domanda.wav [more.wav ...]

Stdlib only (wave + array + math); runs with any python3, no install.
"""

from __future__ import annotations

import array
import math
import sys
import wave


# Heuristic thresholds (16-bit full scale = 32768).
CLIP_LEVEL = 32000          # |sample| at/above this counts as near-full-scale
CLIP_PCT_BAD = 0.5          # >0.5% near-full-scale samples => distortion likely
RAIL_RUN_MS = 1.0           # a flat run at full scale this long => hard clipping
WEAK_PEAK_DBFS = -20.0      # speech should peak well above this
WEAK_RMS_DBFS = -40.0       # below this the mic is too far / gain too low
ZERO_RUN_MS = 4.0           # exact-zero run longer than this, mid-clip => dropout
DC_OFFSET_BAD = 600         # large mean => DC bias / wrong slot extraction


def _dbfs(level: float) -> float:
    """Amplitude (0..32768) to dBFS. Guards log(0)."""
    return 20.0 * math.log10(level / 32768.0) if level > 1.0 else -120.0


def _longest_run(samples, predicate) -> int:
    """Longest run length (in samples) for which predicate(sample) holds."""
    longest = 0
    run = 0
    for s in samples:
        if predicate(s):
            run += 1
            if run > longest:
                longest = run
        else:
            run = 0
    return longest


def _count_runs(samples, predicate, min_len: int) -> int:
    """How many runs satisfying predicate reach at least min_len samples."""
    count = 0
    run = 0
    for s in samples:
        if predicate(s):
            run += 1
            if run == min_len:
                count += 1
        else:
            run = 0
    return count


def analyze_samples(mono, sample_rate: int) -> dict:
    """Core metrics + verdict over a single channel of int samples."""
    n = len(mono)
    if n == 0:
        return {"frames": 0, "verdict": ["empty"]}

    peak = 0
    total = 0
    sumsq = 0.0
    clipped = 0
    for s in mono:
        a = -s if s < 0 else s
        if a > peak:
            peak = a
        total += s
        sumsq += float(s) * float(s)
        if a >= CLIP_LEVEL:
            clipped += 1

    rms = math.sqrt(sumsq / n)
    dc = total / n
    clip_pct = 100.0 * clipped / n
    rail_run = _longest_run(mono, lambda s: (s >= CLIP_LEVEL or s <= -CLIP_LEVEL))
    rail_ms = 1000.0 * rail_run / sample_rate
    zero_run = _longest_run(mono, lambda s: s == 0)
    zero_ms = 1000.0 * zero_run / sample_rate
    dropouts = _count_runs(mono, lambda s: s == 0, max(1, int(ZERO_RUN_MS * sample_rate / 1000)))

    verdict = []
    if clip_pct >= CLIP_PCT_BAD or rail_ms >= RAIL_RUN_MS:
        verdict.append("CLIPPING (mic gain too hot -> lower in_gain)")
    if _dbfs(peak) < WEAK_PEAK_DBFS or _dbfs(rms) < WEAK_RMS_DBFS:
        verdict.append("WEAK/FAR (gain too low or mic distant -> raise in_gain)")
    if dropouts >= 3:
        verdict.append("DROPOUTS (silent gaps mid-audio -> codec/flash stalls)")
    if abs(dc) >= DC_OFFSET_BAD:
        verdict.append("DC OFFSET (bias / wrong slot extraction)")
    if not verdict:
        verdict.append("levels look OK (if still gibberish, suspect sample RATE)")

    return {
        "frames": n,
        "duration_s": n / sample_rate,
        "peak": peak,
        "peak_dbfs": _dbfs(peak),
        "rms_dbfs": _dbfs(rms),
        "clip_pct": clip_pct,
        "rail_ms": rail_ms,
        "zero_run_ms": zero_ms,
        "dropouts": dropouts,
        "dc_offset": dc,
        "verdict": verdict,
    }


def analyze_wav(path: str) -> dict:
    """Read a WAV file and return header info + per-(mono)channel metrics."""
    with wave.open(path, "rb") as w:
        channels = w.getnchannels()
        sampwidth = w.getsampwidth()
        sample_rate = w.getframerate()
        nframes = w.getnframes()
        raw = w.readframes(nframes)

    info = {
        "path": path,
        "channels": channels,
        "sampwidth_bytes": sampwidth,
        "sample_rate": sample_rate,
        "frames": nframes,
        "duration_s": (nframes / sample_rate) if sample_rate else 0.0,
    }
    if sampwidth != 2:
        info["error"] = f"only 16-bit PCM supported (got {sampwidth * 8}-bit)"
        return info

    samples = array.array("h")
    samples.frombytes(raw)
    if sys.byteorder == "big":
        samples.byteswap()

    if channels >= 2:
        # Downmix to mono exactly like the firmware ((L+R)/2) for the headline
        # metrics, but keep each channel so a dead/garbage slot is visible.
        left = samples[0::channels]
        right = samples[1::channels]
        mono = array.array("h", [int((left[i] + right[i]) / 2) for i in range(len(left))])
        info["metrics"] = analyze_samples(mono, sample_rate)
        info["left"] = analyze_samples(left, sample_rate)
        info["right"] = analyze_samples(right, sample_rate)
    else:
        info["metrics"] = analyze_samples(samples, sample_rate)
    return info


def format_report(info: dict) -> str:
    lines = [f"== {info['path']} =="]
    lines.append(
        f"  format    : {info['sample_rate']} Hz, {info['channels']} ch, "
        f"{info['sampwidth_bytes'] * 8}-bit, {info['frames']} frames, {info['duration_s']:.2f} s"
    )
    if "error" in info:
        lines.append(f"  ERROR     : {info['error']}")
        return "\n".join(lines)

    def block(label, m):
        return (
            f"  {label:<9}: peak={m['peak']} ({m['peak_dbfs']:.1f} dBFS)  "
            f"rms={m['rms_dbfs']:.1f} dBFS  clip={m['clip_pct']:.2f}%  "
            f"rail={m['rail_ms']:.1f}ms  zero_run={m['zero_run_ms']:.0f}ms  "
            f"dropouts={m['dropouts']}  dc={m['dc_offset']:.0f}"
        )

    lines.append(block("signal", info["metrics"]))
    if "left" in info:
        lines.append(block("  L", info["left"]))
        lines.append(block("  R", info["right"]))
        lp, rp = info["left"]["peak"], info["right"]["peak"]
        if max(lp, rp) > 0 and min(lp, rp) < 0.1 * max(lp, rp):
            lines.append("  NOTE     : one channel is near-silent -> mono codec; "
                         "use a single channel, do not average")
    lines.append("  verdict  : " + "; ".join(info["metrics"]["verdict"]))
    lines.append("  (compare duration above to how long you actually spoke; a big "
                 "mismatch => real capture rate != header rate)")
    return "\n".join(lines)


def main(argv) -> int:
    paths = argv[1:]
    if not paths:
        print(__doc__)
        return 2
    for path in paths:
        try:
            info = analyze_wav(path)
        except (wave.Error, FileNotFoundError, OSError) as exc:
            print(f"== {path} ==\n  ERROR     : {exc}")
            continue
        print(format_report(info))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
