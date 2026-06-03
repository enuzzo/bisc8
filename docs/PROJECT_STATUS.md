# Bisc8 Project Status

Date: 2026-06-03

## Hardware

- Board: Waveshare ESP32-C6-ePaper-1.54, black-and-white e-paper variant.
- Display: 200x200 1-bit e-paper.
- Main controls: BOOT on GPIO9, PWR on GPIO2.
- I2C bus: SCL GPIO8, SDA GPIO18.
- IO expander: TCA9554 at `0x20`.
- Audio: ES8311 codec path through the board support package.

## Firmware Structure

- `app_main`: boot sequence, event loop, serial command routing, power-state orchestration.
- `BoardSupport`: I2C recovery, TCA9554 setup, power rails, deep sleep wake masks.
- `DisplayService`: LVGL/e-paper screens, Bisc8 boot/off/idle/fortune/mic/error layouts.
- `FortuneService`: random fortune picker backed by generated flash-safe data.
- `AudioService`: beep generation and microphone record/playback test.
- `ButtonController`: BOOT click, PWR click, PWR long press.
- `DebugSerial`: structured logs, serial commands, framebuffer dump.

## Implemented Features

- ESP-IDF firmware under `firmware/bisc8_fortune`.
- LVGL-driven 200x200 e-paper UI.
- Bisc8 boot screen with cookie mark, project name, and Netmilk Studio credit.
- Fortune randomizer from `assets/grimorio.txt`, currently 888 lines.
- Beep on fortune generation.
- Microphone record/playback test.
- Serial debug commands: `DEBUG 0`, `DEBUG 1`, `STATUS`, `SNAP`, `FORTUNE`, `MIC`, `HELP`.
- Snapshot capture tool that writes 1-bit PNGs from the real framebuffer.
- Static tests for display structure, fortune generation, snapshot PNG generation, and power-state guardrails.

## Current Power Behavior

- BOOT click shows a new fortune and plays a beep.
- PWR click runs the mic test.
- PWR long press shows a Bisc8 power-off screen:
  - `Bisc8`
  - `by Netmilk Studio`
  - `Press PWR to turn me on`
- PWR long press then enters deep sleep wakeable only by PWR.
- Idle timeout after 180000 ms enters deep sleep wakeable by BOOT or PWR.
- Deep sleep powers down only the e-paper rail and keeps I2C peripheral rails alive.

## Hardware Lessons Learned

- A previous deep-sleep path powered down audio/VBAT rails and could leave I2C devices holding SCL/SDA low.
- The recovery firmware logs I2C levels before TCA9554 init:
  - `I2C levels released`
  - `I2C levels after-clock`
  - `I2C levels after-stop`
- If both SCL and SDA stay at `0`, a real physical power cycle is needed.
- The board can enter ROM download mode if BOOT/GPIO9 is low during reset.
- Serial tooling should keep DTR asserted and RTS released for normal command/snapshot use on this setup.

## Latest Runtime Check

After the user restarted the board, serial `STATUS` returned:

```text
[STATUS] state=idle board=ready audio=ready debug=on fortune_count=888 fortune_presses=3 mic_tests=0 free_heap=180396
```

Latest snapshot captured:

```text
screenshots/epaper/bisc8_20260603_193314.png
```

## Known Follow-Ups

- Test PWR long-press power-off screen and PWR wake on hardware.
- Test 3-minute idle deep sleep and wake from both BOOT and PWR.
- Measure current draw in active idle, e-paper held image, and deep sleep if an inline USB/battery meter is available.
- Consider a short serial command to shorten idle timeout during QA.
- Decide whether to keep screenshots in git long-term or archive only selected visual milestones.
