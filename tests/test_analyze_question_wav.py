"""The question-WAV analyzer must correctly name the defect it sees.

Synthetic signals (clean / clipped / weak / dropout-ridden / dead-channel) stand
in for real recordings so the heuristics that will diagnose the emailed
domanda.wav are themselves trustworthy.
"""

import importlib.util
import math
import struct
import tempfile
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/analyze_question_wav.py"

spec = importlib.util.spec_from_file_location("analyze_question_wav", TOOL)
analyze = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analyze)


def write_wav(path, samples, rate=16000, channels=1):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b"".join(struct.pack("<h", int(s)) for s in samples))


def sine(amp, freq=220, rate=16000, secs=1.0):
    n = int(rate * secs)
    return [max(-32768, min(32767, int(amp * math.sin(2 * math.pi * freq * i / rate)))) for i in range(n)]


def test_clean_signal_reads_as_ok(tmp_path):
    p = tmp_path / "clean.wav"
    write_wav(p, sine(16000))
    m = analyze.analyze_wav(str(p))["metrics"]
    assert any("OK" in v for v in m["verdict"])
    assert m["clip_pct"] < 0.5


def test_clipped_signal_is_flagged(tmp_path):
    p = tmp_path / "clip.wav"
    # Drive a sine well past full scale, then hard-clamp -> flat-topped rails.
    n = 16000
    samples = [max(-32768, min(32767, int(60000 * math.sin(2 * math.pi * 220 * i / 16000)))) for i in range(n)]
    write_wav(p, samples)
    m = analyze.analyze_wav(str(p))["metrics"]
    assert any("CLIPPING" in v for v in m["verdict"])
    assert m["clip_pct"] > 0.5


def test_weak_signal_is_flagged(tmp_path):
    p = tmp_path / "weak.wav"
    write_wav(p, sine(400))
    m = analyze.analyze_wav(str(p))["metrics"]
    assert any("WEAK" in v for v in m["verdict"])


def test_dropouts_are_flagged(tmp_path):
    p = tmp_path / "drop.wav"
    s = sine(16000, secs=2.0)
    # Punch five 100 ms holes of exact silence into otherwise healthy audio.
    for k in range(5):
        start = 3000 + k * 5000
        for i in range(start, start + 1600):
            s[i] = 0
    write_wav(p, s)
    m = analyze.analyze_wav(str(p))["metrics"]
    assert any("DROPOUT" in v for v in m["verdict"])
    assert m["dropouts"] >= 3


def test_stereo_dead_channel_is_noted(tmp_path):
    p = tmp_path / "stereo.wav"
    left = sine(16000)
    interleaved = []
    for s in left:
        interleaved.append(s)  # L has signal
        interleaved.append(0)  # R is dead (mono codec in one slot)
    write_wav(p, interleaved, channels=2)
    info = analyze.analyze_wav(str(p))
    report = analyze.format_report(info)
    assert "near-silent" in report
    assert info["left"]["peak"] > 1000
    assert info["right"]["peak"] == 0


def test_non_16bit_is_reported_not_crashed(tmp_path):
    p = tmp_path / "eight.wav"
    with wave.open(str(p), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(1)  # 8-bit
        w.setframerate(16000)
        w.writeframes(bytes([128] * 1000))
    info = analyze.analyze_wav(str(p))
    assert "error" in info
    assert "16-bit" in info["error"]
