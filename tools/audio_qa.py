#!/usr/bin/env python3
"""Bisc8 audio QA: one command to judge mic capture + spoken answer quality.

Three jobs, all stdlib-only (wave/array/math/urllib), no installs:

1. ANALYZE local WAVs (delegates the level metrics to analyze_question_wav and
   adds a high-band content check, so muffled/aliased audio is flagged too):

       python3 tools/audio_qa.py question.wav answer.wav

2. FETCH the latest full-quality recordings straight from the device portal
   (works in setup mode at 192.168.4.1 and on the home LAN) then analyze them:

       python3 tools/audio_qa.py --device 192.168.1.42

   Files land in ./audio_qa_out/ as bisc8-question.wav / bisc8-answer.wav.

3. SIMULATE the email attachment encoder on any local WAV — the exact integer
   math the firmware uses — in both flavors, so the email quality can be
   previewed (and A/B listened) without sending a single email:

       python3 tools/audio_qa.py --simulate-email voice_memo.wav

   Writes <name>.email-box.wav (current firmware: averaging/anti-aliased) and
   <name>.email-naive.wav (the old every-Nth decimator that sounded garbled).

Exit code: 0 = all PASS, 1 = warnings, 2 = failures/errors. Add --play on
macOS to hear each analyzed file (afplay).
"""

from __future__ import annotations

import argparse
import array
import math
import pathlib
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
import wave

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from tools.analyze_question_wav import analyze_wav, format_report  # noqa: E402

# Mirrors email_service.cpp: 8 kHz PCM16 review copies with these duration caps.
EMAIL_TARGET_RATE_HZ = 8000
EMAIL_QUESTION_CAP_S = 8
EMAIL_ANSWER_CAP_S = 12

# Speech recorded at 16 kHz should keep noticeable energy above 2.5 kHz
# (consonants). A near-zero ratio on a 16 kHz capture means the signal is
# muffled at the source (mic hole blocked, wrong slot, heavy filtering).
HIGH_BAND_HZ = 2500.0
HIGH_BAND_RATIO_WEAK = 0.02


def device_endpoints(host: str) -> list[tuple[str, str]]:
    base = host if host.startswith("http") else f"http://{host}"
    return [
        (f"{base}/api/audio/question.wav", "bisc8-question.wav"),
        (f"{base}/api/audio/answer.wav", "bisc8-answer.wav"),
    ]


def fetch_device_audio(host: str, out_dir: pathlib.Path) -> list[pathlib.Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    fetched = []
    for url, name in device_endpoints(host):
        target = out_dir / name
        try:
            with urllib.request.urlopen(url, timeout=30) as resp:
                target.write_bytes(resp.read())
            print(f"fetched {url} -> {target} ({target.stat().st_size} bytes)")
            fetched.append(target)
        except urllib.error.HTTPError as exc:
            print(f"skip {url}: HTTP {exc.code} ({'no recording yet' if exc.code == 404 else exc.reason})")
        except (urllib.error.URLError, OSError) as exc:
            print(f"skip {url}: {exc}")
    return fetched


def read_mono16(path: pathlib.Path) -> tuple[array.array, int]:
    """16-bit WAV -> (mono samples, rate). Stereo is downmixed like the firmware."""
    with wave.open(str(path), "rb") as w:
        channels = w.getnchannels()
        if w.getsampwidth() != 2:
            raise ValueError(f"{path}: only 16-bit PCM supported")
        rate = w.getframerate()
        raw = w.readframes(w.getnframes())
    samples = array.array("h")
    samples.frombytes(raw)
    if sys.byteorder == "big":
        samples.byteswap()
    if channels >= 2:
        left = samples[0::channels]
        right = samples[1::channels]
        samples = array.array("h", [int((left[i] + right[i]) / 2) for i in range(len(left))])
    return samples, rate


def write_mono16(path: pathlib.Path, samples, rate: int) -> None:
    data = array.array("h", samples)
    if sys.byteorder == "big":
        data.byteswap()
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(data.tobytes())


def high_band_ratio(samples, rate: int, cutoff_hz: float = HIGH_BAND_HZ) -> float:
    """Energy fraction above ~cutoff via a one-pole high-pass (cheap, stdlib).

    Not a spectrum analyzer: it answers "is there ANY consonant-range content",
    which is what separates a healthy 16 kHz capture from a muffled/aliased one.
    """
    n = len(samples)
    if n < 2:
        return 0.0
    rc = 1.0 / (2.0 * math.pi * cutoff_hz)
    dt = 1.0 / rate
    alpha = rc / (rc + dt)
    hp_energy = 0.0
    total_energy = 0.0
    prev_x = float(samples[0])
    prev_y = 0.0
    for i in range(1, n):
        x = float(samples[i])
        y = alpha * (prev_y + x - prev_x)
        hp_energy += y * y
        total_energy += x * x
        prev_x = x
        prev_y = y
    return (hp_energy / total_energy) if total_energy > 0 else 0.0


def downsample_ratio(source_rate: int, target_rate: int) -> int:
    if source_rate <= 0 or target_rate <= 0:
        return 1
    return max(1, source_rate // target_rate)


def email_downsample(samples, source_rate: int, *, cap_seconds: int, mode: str,
                     target_rate: int = EMAIL_TARGET_RATE_HZ) -> tuple[array.array, int]:
    """Reference twin of the firmware email encoder (email_service.cpp).

    mode="box"   — current firmware: average each group of `ratio` samples
                   (anti-aliasing low-pass), trailing partial group flushed.
    mode="naive" — the old every-Nth decimator, kept ONLY so the regression
                   can be reproduced and A/B listened to.
    target_rate  — override to 4000 to reproduce the legacy 4 kHz pipeline.
    Returns (samples, output_rate); output rate is source/ratio, as stamped
    in the firmware WAV header.
    """
    ratio = downsample_ratio(source_rate, target_rate)
    out_rate = source_rate // ratio
    cap_samples = target_rate * cap_seconds  # byte cap / 2, as in firmware
    out = array.array("h")
    if mode == "naive":
        for index, sample in enumerate(samples):
            if index % ratio != 0:
                continue
            if len(out) >= cap_samples:
                break
            out.append(sample)
        return out, out_rate
    if mode != "box":
        raise ValueError(f"unknown mode: {mode}")
    acc = 0
    acc_count = 0
    for sample in samples:
        acc += sample
        acc_count += 1
        if acc_count < ratio:
            continue
        if len(out) >= cap_samples:
            acc_count = 0
            break
        out.append(int(acc / ratio))  # C++ int division truncates toward zero too
        acc = 0
        acc_count = 0
    if acc_count > 0 and len(out) < cap_samples:
        out.append(int(acc / acc_count))  # ceil-division byte count: flush partial group
    return out, out_rate


def simulate_email(path: pathlib.Path, cap_seconds: int) -> list[pathlib.Path]:
    samples, rate = read_mono16(path)
    written = []
    variants = (
        # suffix, mode, target rate, cap — old4k is the exact pre-fix pipeline
        # (naive every-Nth at 4 kHz with the short 6 s question cap).
        ("email-old4k", "naive", 4000, min(cap_seconds, 6)),
        ("email-naive", "naive", EMAIL_TARGET_RATE_HZ, cap_seconds),
        ("email-box", "box", EMAIL_TARGET_RATE_HZ, cap_seconds),
    )
    for suffix, mode, target_rate, cap in variants:
        out_samples, out_rate = email_downsample(samples, rate, cap_seconds=cap, mode=mode,
                                                 target_rate=target_rate)
        target = path.with_suffix(f".{suffix}.wav")
        write_mono16(target, out_samples, out_rate)
        ratio = high_band_ratio(out_samples, out_rate, cutoff_hz=min(HIGH_BAND_HZ, out_rate / 4))
        print(f"wrote {target}  ({out_rate} Hz, {len(out_samples) / out_rate:.1f} s, hf_ratio={ratio:.3f})")
        written.append(target)
    print("listen: -old4k is what the firmware emailed before the fix, "
          "-box is what it sends now (-naive isolates the missing-filter half of the bug).")
    return written


def grade(info: dict, hf_ratio: float | None) -> tuple[str, list[str]]:
    """(PASS|WARN|FAIL, reasons) from the analyzer verdict + high-band check."""
    if "error" in info:
        return "FAIL", [info["error"]]
    verdict = info["metrics"]["verdict"]
    reasons = [v for v in verdict if not v.startswith("levels look OK")]
    failures = [v for v in reasons if v.startswith(("CLIPPING", "DROPOUTS"))]
    if hf_ratio is not None and hf_ratio < HIGH_BAND_RATIO_WEAK:
        reasons.append(
            f"MUFFLED (energy above {HIGH_BAND_HZ:.0f} Hz is {hf_ratio:.1%} -> blocked mic hole, "
            "wrong codec slot, or aliased/downsampled source)")
    if failures:
        return "FAIL", reasons
    if reasons:
        return "WARN", reasons
    return "PASS", ["levels and band content look healthy"]


def analyze_paths(paths: list[pathlib.Path], play: bool) -> int:
    worst = 0
    for path in paths:
        try:
            info = analyze_wav(str(path))
        except (wave.Error, FileNotFoundError, OSError, ValueError) as exc:
            print(f"== {path} ==\n  ERROR     : {exc}")
            worst = max(worst, 2)
            continue
        print(format_report(info))
        hf = None
        if "error" not in info:
            samples, rate = read_mono16(path)
            # Only meaningful when the capture has room above the cutoff.
            if rate >= 4 * HIGH_BAND_HZ / 2:
                hf = high_band_ratio(samples, rate)
                print(f"  high band : {hf:.1%} of energy above {HIGH_BAND_HZ:.0f} Hz")
        level, reasons = grade(info, hf)
        print(f"  GRADE     : {level} — " + "; ".join(reasons))
        worst = max(worst, {"PASS": 0, "WARN": 1, "FAIL": 2}[level])
        if play and shutil.which("afplay"):
            subprocess.run(["afplay", str(path)], check=False)
    return worst


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("wavs", nargs="*", help="local WAV files to analyze")
    parser.add_argument("--device", help="device IP/host: fetch question+answer WAVs from the portal first")
    parser.add_argument("--out-dir", default="audio_qa_out", help="where fetched files go (default: ./audio_qa_out)")
    parser.add_argument("--simulate-email", action="store_true",
                        help="write .email-box.wav / .email-naive.wav previews of the email encoder for each input")
    parser.add_argument("--answer", action="store_true",
                        help="with --simulate-email: use the answer duration cap (12 s) instead of the question one (8 s)")
    parser.add_argument("--play", action="store_true", help="play each analyzed file (macOS afplay)")
    args = parser.parse_args(argv[1:])

    paths = [pathlib.Path(p) for p in args.wavs]
    if args.device:
        paths += fetch_device_audio(args.device, pathlib.Path(args.out_dir))
    if not paths:
        parser.print_help()
        return 2

    if args.simulate_email:
        cap = EMAIL_ANSWER_CAP_S if args.answer else EMAIL_QUESTION_CAP_S
        derived = []
        for path in paths:
            derived += simulate_email(path, cap)
        paths += derived

    return analyze_paths(paths, args.play)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
