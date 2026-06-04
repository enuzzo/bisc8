#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FLASH_DIR = ROOT / "public" / "flash" / "firmware"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def checked_artifact(build_dir: Path, relative_path: str) -> Path:
    source = build_dir / relative_path
    if source.is_symlink():
        raise RuntimeError(f"Refusing to copy symlinked artifact: {source}")
    if not source.is_file():
        raise FileNotFoundError(source)

    resolved_build = build_dir.resolve()
    resolved_source = source.resolve()
    if resolved_build not in resolved_source.parents:
        raise RuntimeError(f"Artifact escapes build directory: {source}")
    return source


def copy_artifact(build_dir: Path, relative_path: str, name: str) -> None:
    source = checked_artifact(build_dir, relative_path)
    FLASH_DIR.mkdir(parents=True, exist_ok=True)
    destination = FLASH_DIR / name
    shutil.copy2(source, destination)
    print(f"{name}: sha256={sha256(destination)}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Copy Bisc8 ESP-IDF binaries into the public web flasher folder.")
    parser.add_argument("--build-dir", required=True, type=Path, help="ESP-IDF build directory.")
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    copy_artifact(build_dir, "bootloader/bootloader.bin", "bootloader.bin")
    copy_artifact(build_dir, "partition_table/partition-table.bin", "partition-table.bin")
    copy_artifact(build_dir, "bisc8_fortune.bin", "bisc8_fortune.bin")
    print(f"Copied web flash artifacts to {FLASH_DIR}")


if __name__ == "__main__":
    main()
