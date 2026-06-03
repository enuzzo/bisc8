# Bisc8

Bisc8 is an ESP-IDF firmware for the Waveshare ESP32-C6-ePaper-1.54 black-and-white board. It turns the device into a small e-paper fortune teller with BOOT/PWR controls, audio feedback, serial debugging, and framebuffer snapshots.

## Current Behavior

- BOOT click: pick a random non-repeating fortune from the generated grimorio data and play a beep.
- PWR click: run the microphone record/playback test.
- PWR long press: show the Bisc8 power-off prompt, then enter deep sleep wakeable by PWR.
- Idle timeout: after 3 minutes with no button or serial events, enter deep sleep wakeable by BOOT or PWR.
- Serial commands: `DEBUG 0`, `DEBUG 1`, `STATUS`, `SNAP`, `FORTUNE`, `MIC`, `HELP`.

## Firmware

The ESP-IDF project lives in:

```sh
firmware/bisc8_fortune
```

Build and flash:

```sh
cd firmware/bisc8_fortune
export IDF_TOOLS_PATH=/Users/enuzzo/Library/CloudStorage/Dropbox/Mitnick/bisc8/.espressif
source ../../.esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem83201 flash
```

The local ESP-IDF checkout, toolchain cache, build products, and managed components are intentionally ignored by git.

## Fortune Data

Source fortunes are stored in:

```sh
assets/grimorio.txt
```

Generate firmware flash data:

```sh
python3 tools/generate_fortunes.py
```

Recommended text length is around 60-90 characters, with a hard practical maximum of 120 characters per line for the current 200x200 e-paper layout.

## Snapshot Tool

Capture the real e-paper framebuffer as a 1-bit PNG:

```sh
python3 tools/capture_epaper_snapshot.py --port /dev/cu.usbmodem83201
```

Snapshots are written to:

```sh
screenshots/epaper/
```

## Tests

```sh
python3 -m pytest tests/test_display_design_static.py tests/test_generate_fortunes.py tests/test_snapshot_png.py -q
```

Latest local result before repository initialization:

```text
18 passed
```
