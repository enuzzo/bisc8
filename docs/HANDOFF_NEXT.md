# Handoff (next session)

Last session: device-states block of the redesign. Date: 2026-06-05.

## State: DONE + pushed
- Commit `7677b56` on `main`, pushed to origin. Working tree clean (only untracked
  `screenshots/epaper/` and `docs/gpt-suggestions/`, neither mine).
- Host tests: **70 passed** (`.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests/`).

## What shipped (all 4 items)
1. Animated speaker glyph on `ShowVoiceSpeaking` (`display_service.cpp`), bounded
   `lv_timer` pulsing 3 sound-wave bars. Driven by new `AudioService` playback hook
   (`SetPlaybackObserver` -> `DisplayService::OnPlaybackState`). Recording hook
   (`SetRecordingObserver` -> `OnListeningState`) is the clean seam for a future
   "ti ascolto" state; audio pipeline NOT built (by design).
2. Dedicated screens: `ShowNoWifi`, `ShowLowBattery`, `ShowFirstRun`. Low-battery
   auto-triggers at boot/wake when `BatteryLevel() <= 12%`; first-run when
   `g_fortune_count == 0` (both in `app_main.cpp` resting-screen override). Copy
   IT/EN/ES in `localization.cpp`, no long dashes.
3. E-ink refresh policy in `lvgl_flush_cb` (`display_service.cpp`): FULL on responso/
   voice reveal + forced FULL after 8 partials (anti-ghost). New `EPD_DisplayFull()`
   in `components/port_bsp/port_display.cpp`.
4. Footer battery icon (`BuildFooterBattery`, fill width tracks charge).
- Serial: `SCREEN NOWIFI|LOWBATT|FIRSTRUN|SPEAK` forces each screen for SNAP.
- New test file `tests/test_device_states_static.py`.

## Validated on hardware (first flash) via SNAP
intro (intact), no-wifi, low-battery, first-run, speaking (cone-only + cone+waves).
Refresh policy confirmed in `[EPD]` logs: `full (reveal)` -> `partial n=1..8` ->
`full (anti-ghost)`.

## OPEN / TODO when back at the bench
- **RE-FLASH + replug.** The post-commit flash wrote+verified OK, but the USB-JTAG
  port did NOT re-enumerate after `hard_reset` (no VID 303a on host). Physically
  replug the cable, then:
  ```
  export BISC8_IDF_TOOLS_PATH="$PWD/.espressif" && . tools/idf_env.sh
  idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodem14201 flash
  ```
  The flashed build is functionally identical to the validated one (only version
  string differs), so this is just to confirm clean serial + run the committed
  build version. See memory `bisc8-usb-jtag-reenumeration`.
- Optional polish (deferred): the no-wifi glyph is a touch dense; could widen the
  arc spacing. Re-SNAP `SCREEN SPEAK` after the animation settles for a clean
  cone+waves shot (live animation can corrupt a streamed dump; retry or wait ~8s).
- No-wifi screen has no organic trigger yet (connectivity always opens setup
  portal when offline; left untouched per brief "don't touch core logic"). Reachable
  via `SCREEN NOWIFI`.

## Reminders
- Build/flash + host-test gotchas are in memory (`bisc8-build-flash-workflow`,
  `bisc8-running-host-tests`, `bisc8-usb-jtag-reenumeration`).
