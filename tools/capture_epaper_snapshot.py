#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import time
import zlib
from datetime import datetime
from pathlib import Path

import serial


DEFAULT_PORT = "/dev/cu.usbmodem83201"
DEFAULT_OUTPUT_DIR = Path("screenshots/epaper")
SNAP_BEGIN_RE = re.compile(r"^\[SNAP\] BEGIN\s+(\d+)\s+(\d+)\s+(\d+)\s*$")


def decode_snapshot_hex(payload: str) -> tuple[int, int, bytes]:
    lines = payload.splitlines()
    width = height = expected_len = None
    hex_chunks: list[str] = []
    capturing = False

    for line in lines:
        stripped = line.strip()
        if not capturing:
            match = SNAP_BEGIN_RE.match(stripped)
            if match:
                width = int(match.group(1))
                height = int(match.group(2))
                expected_len = int(match.group(3))
                capturing = True
            continue

        if stripped.startswith("[SNAP] END"):
            break
        if stripped and all(ch in "0123456789abcdefABCDEF" for ch in stripped):
            hex_chunks.append(stripped)

    if width is None or height is None or expected_len is None:
        raise ValueError("SNAP begin marker not found")

    data = bytes.fromhex("".join(hex_chunks))
    if len(data) != expected_len:
        raise ValueError(f"SNAP length mismatch: got {len(data)}, expected {expected_len}")
    return width, height, data


def _png_chunk(kind: bytes, payload: bytes) -> bytes:
    crc = zlib.crc32(kind)
    crc = zlib.crc32(payload, crc) & 0xFFFFFFFF
    return len(payload).to_bytes(4, "big") + kind + payload + crc.to_bytes(4, "big")


def write_png_1bit(path: Path, width: int, height: int, data: bytes) -> None:
    row_stride = (width + 7) // 8
    expected_len = row_stride * height
    if len(data) < expected_len:
        raise ValueError(f"Frame buffer too small: got {len(data)}, expected {expected_len}")

    scanlines = bytearray()
    for y in range(height):
        start = y * row_stride
        scanlines.append(0)  # PNG filter type 0
        scanlines.extend(data[start:start + row_stride])

    ihdr = (
        width.to_bytes(4, "big")
        + height.to_bytes(4, "big")
        + bytes([1, 0, 0, 0, 0])  # 1-bit grayscale, no interlace
    )
    png = (
        b"\x89PNG\r\n\x1a\n"
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", zlib.compress(bytes(scanlines), level=9))
        + _png_chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def capture_snapshot(
    port: str,
    baud: int,
    timeout: float,
    settle: float,
    before_command: str | None = None,
    before_delay: float = 0.0,
) -> str:
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.2
    ser.dsrdtr = False
    ser.rtscts = False
    ser.dtr = True
    ser.rts = False
    with ser:
        ser.dtr = True
        ser.rts = False
        if settle > 0:
            time.sleep(settle)
        ser.reset_input_buffer()
        if before_command:
            command = before_command.strip()
            if command:
                ser.write(command.encode("utf-8") + b"\n")
                ser.flush()
                if before_delay > 0:
                    time.sleep(before_delay)
                ser.reset_input_buffer()
        ser.write(b"SNAP\n")
        deadline = time.monotonic() + timeout
        lines: list[str] = []
        while time.monotonic() < deadline:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            lines.append(line)
            if line.startswith("[SNAP] END"):
                return "\n".join(lines)
    raise TimeoutError(f"Timed out waiting for SNAP data on {port}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture a Bisc8 e-paper framebuffer snapshot.")
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=8.0)
    parser.add_argument("--settle", type=float, default=4.0, help="Seconds to wait after opening serial before sending SNAP.")
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--payload-file", type=Path, help="Decode a saved SNAP payload instead of reading serial.")
    parser.add_argument("--before-command", help="Optional serial command to send before SNAP, for example FORTUNE.")
    parser.add_argument("--before-delay", type=float, default=0.0, help="Seconds to wait after --before-command before SNAP.")
    args = parser.parse_args()

    if args.payload_file:
        payload = args.payload_file.read_text(encoding="utf-8")
    else:
        payload = capture_snapshot(args.port, args.baud, args.timeout, args.settle, args.before_command, args.before_delay)

    width, height, data = decode_snapshot_hex(payload)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = args.out_dir / f"bisc8_{timestamp}.png"
    write_png_1bit(out, width, height, data)
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
