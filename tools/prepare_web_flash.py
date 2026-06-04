#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FLASH_DIR = ROOT / "public" / "flash" / "firmware"


def copy_artifact(source: Path, name: str) -> None:
    if not source.exists():
        raise FileNotFoundError(source)
    FLASH_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, FLASH_DIR / name)


def main() -> None:
    parser = argparse.ArgumentParser(description="Copy Bisc8 ESP-IDF binaries into the public web flasher folder.")
    parser.add_argument("--build-dir", required=True, type=Path, help="ESP-IDF build directory.")
    args = parser.parse_args()

    build_dir = args.build_dir
    copy_artifact(build_dir / "bootloader" / "bootloader.bin", "bootloader.bin")
    copy_artifact(build_dir / "partition_table" / "partition-table.bin", "partition-table.bin")
    copy_artifact(build_dir / "bisc8_fortune.bin", "bisc8_fortune.bin")
    print(f"Copied web flash artifacts to {FLASH_DIR}")


if __name__ == "__main__":
    main()
