# Bisc8 AI Handoff Guide

This document is written for future AI agents and contributors who need to continue Bisc8 without rediscovering the project shape from scratch.

## Project Summary

Bisc8 is ESP-IDF firmware for the Waveshare ESP32-C6-ePaper-1.54 black-and-white board. The product is a small e-paper oracle with two buttons, audio cues, localized UI, Wi-Fi setup, a local web portal, and a planned OpenAI-powered voice-oracle flow.

The current firmware is useful as an offline fortune oracle and as a product setup scaffold. The online voice oracle is partially scaffolded but not complete.

## Current Truth About OpenAI

OpenAI STT/Responses/TTS transport is not implemented yet.

Implemented pieces:

- The local web UI stores an OpenAI API key and model choices in NVS.
- Defaults exist for transcription, response, speech, and voice settings.
- BOOT hold/release enters the voice-oracle UI flow.
- `AudioService` records a 16 kHz mono WAV into flash spool storage.
- The recorded path is `spool://question.wav`.
- `VoiceOracleService` defines the intended response contract and logs the planned API paths.

Not implemented yet:

- No HTTPS multipart upload to OpenAI speech-to-text.
- No Responses API request/JSON parsing.
- No OpenAI text-to-speech request.
- No playback of generated TTS audio.
- No email relay HTTP POST.

The relevant placeholder is `VoiceOracleService::AskFromRecordedAudio()` in `firmware/bisc8_fortune/main/voice_oracle_service.cpp`. It logs `audio/transcriptions`, `responses`, and `audio/speech`, fills empty response fields, and returns `ESP_ERR_NOT_FINISHED`.

## Hardware

- Board: Waveshare ESP32-C6-ePaper-1.54.
- Display: 200x200 1-bit e-paper.
- Main buttons: BOOT on GPIO9, PWR on GPIO2.
- I2C: SCL GPIO8, SDA GPIO18.
- IO expander: TCA9554 at `0x20`.
- Audio codec path: ES8311 through the board support package.
- Flash profile: 16 MB.

Important behavior:

- Holding BOOT during reset may put the ESP32-C6 in ROM download mode.
- The firmware includes I2C recovery before peripheral init because previous deep-sleep rail shutdowns could leave I2C lines stuck.
- Flashing normally should not erase NVS unless you explicitly erase or use the public web flasher new-install flow.

## Repository Map

- `firmware/bisc8_fortune`: ESP-IDF firmware project.
- `firmware/bisc8_fortune/main/app_main.cpp`: boot sequence, event loop, serial command routing, power behavior.
- `firmware/bisc8_fortune/main/app_config.*`: NVS settings schema, defaults, secret masking.
- `firmware/bisc8_fortune/main/connectivity_service.*`: Wi-Fi scan/reconnect, SoftAP setup mode, status state.
- `firmware/bisc8_fortune/main/web_portal.*`: local setup portal HTML, API routes, settings save/reset.
- `firmware/bisc8_fortune/main/captive_dns_service.*`: DNS responder for captive portal fallback.
- `firmware/bisc8_fortune/main/display_service.*`: LVGL/e-paper screen rendering.
- `firmware/bisc8_fortune/main/localization.*`: display and web string tables for `en`, `es`, `it`.
- `firmware/bisc8_fortune/main/fortune_service.*`: non-repeating random fortune selection.
- `firmware/bisc8_fortune/main/audio_service.*`: codec init, audio cues, mic test, voice recording to flash spool.
- `firmware/bisc8_fortune/main/voice_oracle_service.*`: planned OpenAI voice-oracle contract; currently incomplete.
- `firmware/bisc8_fortune/main/email_service.*`: planned relay handoff; currently incomplete.
- `firmware/bisc8_fortune/main/generated`: generated firmware data for fortunes, logo, fonts, and sounds.
- `assets`: source grimoire, candidate sounds, sound previews, logo source.
- `public/flash`: static ESP Web Tools flashing page and manifest.
- `tools`: asset generation, snapshot, web-flash preparation, and ESP-IDF environment helper scripts.
- `tests`: static and generation tests that protect product behavior and documentation.

## Boot And Runtime Flow

High-level boot flow in `app_main.cpp`:

1. Create the event queue and service objects.
2. Load NVS settings with `ConfigStore`.
3. Initialize board support and recover I2C if needed.
4. Initialize display and show the boot logo.
5. Initialize audio and start the boot cue asynchronously.
6. Keep the boot splash visible for at least 5 seconds.
7. Show the localized intro screen.
8. If BOOT is held, force Wi-Fi setup mode.
9. Otherwise scan for known Wi-Fi networks and connect only to visible saved SSIDs.
10. If online, briefly show connected SSID and IP, then return to the intro screen.
11. If no saved network connects, start SoftAP/captive portal.
12. Initialize buttons and enter the event loop.

Runtime button behavior:

- BOOT click: draw a localized fortune and play the oracle button cue.
- BOOT hold: begin voice recording.
- BOOT release: finish recording, show "Cooking", play the voice submit cue, call the unfinished oracle service.
- BOOT long press: begin voice/dictation mode.
- PWR triple click: full config reset, then reopen setup portal.
- BOOT + PWR long hold: legacy full config reset fallback, but on the current board this can be intercepted as a reboot before firmware sees the event; prefer PWR triple click or serial `CONFIG RESET`.
- PWR click: show Wi-Fi/status instructions and IP.
- PWR long press: show power-off screen, play shutdown cue, then deep sleep.

## Configuration

Config is stored in NVS namespace `bisc8`.

Core limits:

- `kMaxWifiCredentials = 8`.
- `kWifiAttemptTimeoutMs = 5000`.
- `kVoiceRecordLimitMs = 15000`.
- `kMaxScreenAnswerChars = 100`.
- `kConfigSchemaVersion = 2`.

Settings:

- Language.
- Up to 8 Wi-Fi credentials.
- OpenAI API key and model names.
- Email recipient.
- Optional email relay URL/token.

Secrets are masked in UI/API responses. They are still stored on the device. Production firmware should use flash encryption before storing real user secrets.

Build-time relay defaults:

- `CONFIG_BISC8_EMAIL_RELAY_URL`
- `CONFIG_BISC8_EMAIL_RELAY_TOKEN`

Do not commit production relay tokens.

## Wi-Fi And Captive Portal

The Wi-Fi manager scans first, then tries only saved SSIDs that are visible in scan results. Each known network gets a 5 second attempt. The display shows the attempted SSID/countdown during visible reconnect attempts.

If no known network connects:

- SoftAP starts as `Bisc8-XXXX`.
- Portal URL is `http://192.168.4.1`.
- DNS captive redirect listens on UDP 53.
- Common captive probe paths redirect to `/`.
- PWR status gives manual fallback instructions because phone captive detection can be unreliable.
- Saving Wi-Fi tests the submitted credentials with STA before persisting them; on success `/api/status` returns `reboot_required`, the UI shows "Reboot now", and `/api/reboot` responds before calling `esp_restart()`.
- The setup portal intentionally does not require a PIN; the product favors low-friction local setup while the SoftAP is active.
- API responses mask stored secrets, so status and scan do not copy sensitive values back to the browser.

Web routes:

- `/`
- `/api/status`
- `/api/wifi/scan`
- `/api/wifi/credentials`
- `/api/settings`
- `/api/openai`
- `/api/email`
- `/api/smtp`
- `/api/reset`
- `/api/reboot`

The email UI still uses some historical route naming, but the current product direction is recipient-first email plus optional relay fields, not end-user SMTP configuration.

## Display And Localization

First boot defaults to English. Saved language can be `en`, `es`, or `it`. Display strings, web UI strings, and grimoire data are localized.

The display uses generated Montserrat Medium LVGL fonts with Latin-1 coverage (`0x20-0xFF`) so Italian and Spanish accents remain intact.

Display states include:

- Boot.
- Intro.
- Fortune.
- Wi-Fi setup.
- Wi-Fi connecting/status.
- Listening.
- Cooking.
- Thinking.
- Speaking.
- Error.
- Power off.

The boot splash is intentionally held for at least 5 seconds while the boot cue plays in the background.

## Audio

Audio cues are generated into firmware arrays as 16 kHz, 16-bit, stereo PCM. This avoids needing an OGG or MP3 decoder on the device.

Current cues:

- `boot`
- `oracle_button`
- `voice_submit`
- `shutdown`

Voice recording:

- Recording is triggered by holding BOOT.
- Release stops recording.
- Hard limit is 15 seconds.
- Stereo codec input is downmixed to mono.
- WAV data is written into the raw `spool` flash partition.
- The logical path returned by `AudioService::FinishVoiceRecording()` is `spool://question.wav`.

## Partition Table

The 16 MB flash partition table:

- `nvs`: `0x9000`, size `0x6000`.
- `phy_init`: `0xf000`, size `0x1000`.
- `factory`: `0x10000`, size `0x600000`.
- `assets`: `0x610000`, size `0x500000`, reserved for future media/data.
- `spool`: `0xb10000`, size `0x4f0000`, raw temporary audio/payload storage.

The app partition has substantial free space, but keep checking size after adding HTTP/TLS/OpenAI parsing.

## Build And Flash

Use the repository environment helper:

```sh
source tools/idf_env.sh
python "$IDF_PATH/tools/idf_tools.py" install --targets esp32c6
python "$IDF_PATH/tools/idf_tools.py" install-python-env
idf.py -C firmware/bisc8_fortune build
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodemXXXX flash
```

`tools/idf_env.sh` keeps Intel and Apple Silicon ESP-IDF tool caches separate:

- Intel: `~/.espressif-bisc8-x86_64`
- Apple Silicon: `~/.espressif-bisc8-arm64`

Do not share one `.espressif` tool cache across host architectures. ESP-IDF extracts same-version tools into the same subdirectory names even when the host binaries differ.

## Asset Pipelines

Fortunes:

```sh
python3 tools/generate_fortunes.py
```

Logo:

```sh
python3 tools/generate_logo_assets.py
```

Sounds:

```sh
python3 tools/generate_sound_assets.py
```

Display fonts:

```sh
python3 tools/generate_display_fonts.py
```

Public web flasher artifacts:

```sh
python3 tools/prepare_web_flash.py --build-dir path/to/bisc8-fortune-build
```

## Tests And Verification

Run this before claiming completion:

```sh
.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests -q
```

For firmware changes, also build:

```sh
source tools/idf_env.sh
idf.py -C firmware/bisc8_fortune build
```

For hardware changes, also flash and inspect serial logs:

```sh
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodemXXXX flash
```

When using the Codex sandbox on macOS, ESP-IDF may need an escalated run because `psutil` calls `sysctl()` during CMake.

## Known Incomplete Work

Highest-priority unfinished items:

1. Implement OpenAI STT/Responses/TTS HTTPS transport in `VoiceOracleService`.
2. Stream or read `spool://question.wav` from flash into the transcription request.
3. Parse the model JSON contract robustly and enforce the 100 character screen answer in firmware.
4. Store or stream generated TTS audio and play it through `AudioService`.
5. Implement `EmailService` HTTPS relay POST and degraded text-only fallback.
6. Add richer local web UI validation.
7. Add hardware QA for online IP splash, captive portal stability, and deep-sleep reconnect.
8. Decide whether to keep the public repository private until a secret/history scan is complete.

## Git Hygiene

Current working branch for this effort has been `codex/wifi-voice-oracle`.

Before merging:

- Make sure generated build directories are not staged.
- Leave user-supplied untracked logo/reference assets untouched unless explicitly asked.
- Run tests.
- Push the feature branch.
- Merge to `main`.
- Run tests again on `main`.
- Push `main`.

Do not erase NVS during normal flash unless the task explicitly requires a clean first-boot setup test.
