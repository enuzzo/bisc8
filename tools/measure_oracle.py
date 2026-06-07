#!/usr/bin/env python3
"""Measure Bisc8 voice-oracle latency per phase from the [ORACLE] serial logs.

Triggers ambient queries over the serial console (no real voice needed: the mic
picks up room noise, which whisper usually turns into a short word) and parses
the device's own `time=` numbers plus host wall-clock offsets from VOICE STOP.
This is how we proved the latency is network/audio-transfer bound, not GPT or our
own processing.

Usage:
    python3 tools/measure_oracle.py [record_seconds] [successful_runs] [tag]
    # e.g. python3 tools/measure_oracle.py 3.0 4 after-instant

Needs only pyserial; use the system python3 (the idf env lacks it on some setups).
"""
import glob
import re
import statistics
import sys
import time

import serial  # pyserial

PORTS = glob.glob("/dev/cu.usbmodem*") or glob.glob("/dev/ttyUSB*") or glob.glob("/dev/ttyACM*")
if not PORTS:
    print("no serial port found (is the device plugged in?)")
    sys.exit(1)

RECORD_S = float(sys.argv[1]) if len(sys.argv) > 1 else 3.0
WANT = int(sys.argv[2]) if len(sys.argv) > 2 else 4
TAG = sys.argv[3] if len(sys.argv) > 3 else "run"

ser = serial.Serial(PORTS[0], 115200, timeout=0.3)


def one_query():
    time.sleep(0.4)
    ser.reset_input_buffer()
    ser.write(b"VOICE START\n")
    time.sleep(RECORD_S)
    ser.write(b"VOICE STOP\n")
    t0 = time.monotonic()
    rec = {"stt_ms": None, "brain_ms": None, "tts_ms": None, "tts_status": None,
           "brain_at": None, "tts_at": None, "done_at": None, "empty": False}
    while time.monotonic() - t0 < 130:
        try:
            ln = ser.readline().decode("utf-8", "replace").rstrip()
        except Exception:
            continue
        if not ln or "ORACLE" not in ln:
            continue
        off = time.monotonic() - t0
        print(f"  [{off:6.1f}s] {ln.split('[ORACLE]', 1)[1].strip()[:118]}")
        m = re.search(r"stt model=\S+ wav=\d+B status=\d+ time=(\d+)ms", ln)
        if m:
            rec["stt_ms"] = int(m.group(1))
        if "empty transcript" in ln:
            rec["empty"] = True
            return rec
        if "brain POST" in ln:
            rec["brain_at"] = off
        m = re.search(r"brain ok .*time=(\d+)ms", ln)
        if m:
            rec["brain_ms"] = int(m.group(1))
        m = re.search(r"tts .*status=(\d+) bytes=\d+ time=(\d+)ms", ln)
        if m:
            rec["tts_status"] = int(m.group(1))
            rec["tts_ms"] = int(m.group(2))
            rec["tts_at"] = off
        if "tts ok" in ln or "tts failed" in ln:
            rec["done_at"] = off
            return rec
    return rec


results = []
attempts = 0
while len(results) < WANT and attempts < WANT * 4:
    attempts += 1
    print(f"--- {TAG} attempt {attempts} ---")
    r = one_query()
    if r["empty"]:
        print("  (empty transcript, retry)")
        continue
    if r["stt_ms"] is not None:
        results.append(r)
ser.close()


def col(k):
    return [r[k] for r in results if r.get(k) is not None]


print(f"\n===== SUMMARY {TAG}  (n={len(results)}) =====")
for k, label, unit in [("stt_ms", "STT (device)", "ms"), ("brain_ms", "brain (device)", "ms"),
                       ("tts_ms", "TTS (device)", "ms"),
                       ("done_at", "answer audio ready", "s from VOICE STOP")]:
    v = col(k)
    if v:
        print(f"  {label:22s} min={min(v):8.0f} med={statistics.median(v):8.0f} max={max(v):8.0f}  {unit}")
for r in results:
    if r["stt_ms"] is not None:
        # Perceived TEXT-answer latency ~ stt + brain (the screen shows it before TTS).
        text_at = (r["stt_ms"] + (r["brain_ms"] or 0)) / 1000.0
        print(f"  run: stt={r['stt_ms']}ms brain={r['brain_ms']}ms tts={r['tts_ms']}ms "
              f"tts_status={r['tts_status']}  text≈{text_at:.1f}s  audio≈{r['done_at']}s")
