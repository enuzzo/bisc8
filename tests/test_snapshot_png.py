from pathlib import Path

from tools.capture_epaper_snapshot import decode_snapshot_hex, write_png_1bit

ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT_TOOL = ROOT / "tools/capture_epaper_snapshot.py"


def test_decode_snapshot_hex_returns_expected_bytes():
    payload = """
noise
[SNAP] BEGIN 200 200 4
FF00
AA55
[SNAP] END
"""

    width, height, data = decode_snapshot_hex(payload)

    assert width == 200
    assert height == 200
    assert data == bytes([0xFF, 0x00, 0xAA, 0x55])


def test_write_png_1bit_writes_valid_png_signature(tmp_path: Path):
    out = tmp_path / "snap.png"
    data = bytes([0xFF] * (8 * 8 // 8))

    write_png_1bit(out, 8, 8, data)

    written = out.read_bytes()
    assert written.startswith(b"\x89PNG\r\n\x1a\n")
    assert b"IHDR" in written
    assert b"IDAT" in written


def test_snapshot_serial_keeps_board_out_of_download_mode():
    source = SNAPSHOT_TOOL.read_text(encoding="utf-8")

    assert "ser.dtr = True" in source
    assert "ser.rts = False" in source
