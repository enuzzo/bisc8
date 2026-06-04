# Bisc8

Bisc8 is an ESP-IDF firmware for the Waveshare ESP32-C6-ePaper-1.54 black-and-white board. It turns the device into a small e-paper fortune teller with BOOT/PWR controls, audio feedback, serial debugging, and framebuffer snapshots.

For future AI agents and contributors, start with the extended project map in [`docs/AI_HANDOFF.md`](docs/AI_HANDOFF.md).

## Current Behavior

- First boot defaults to English.
- Boot plays a longer background startup jingle and lands on the localized introductory oracle screen.
- BOOT click: pick a random non-repeating fortune from the generated grimoire data for the selected language and play the oracle-button cue.
- Hold BOOT to ask by recording a voice question. Release BOOT to enter the voice-oracle flow; OpenAI transport is still pending and currently returns an unconfigured/error state.
- On voice release, Bisc8 shows the localized voice flow with the English title "Cooking" and plays the voice-submit cue while the answer is prepared.
- BOOT long press forces Wi-Fi setup. BOOT + PWR long press performs a full configuration reset.
- PWR click: show localized Wi-Fi/status/setup instructions, including the connected SSID and device IP, or the `Bisc8-XXXX` setup hotspot and `http://192.168.4.1`.
- PWR long press: show the Bisc8 power-off prompt, play the shutdown cue, then enter deep sleep wakeable by PWR.
- Idle timeout: after 3 minutes with no button or serial events, enter deep sleep wakeable by BOOT or PWR.
- Serial commands: `DEBUG 0`, `DEBUG 1`, `STATUS`, `SNAP`, `FORTUNE`, `MIC`, `VOICE START`, `VOICE STOP`, `WIFI SETUP`, `WIFI RESET`, `CONFIG RESET`, `HELP`.
- Configuration is stored in NVS: language, up to 8 Wi-Fi credentials, OpenAI settings, email recipient, and optional email relay settings.
- On boot, Bisc8 scans for saved SSIDs, tries visible known networks for 5 seconds each, briefly shows the connected SSID and IP when online, and starts setup mode when none connects, while keeping the e-paper on the introductory screen unless setup was explicitly forced.
- Setup mode starts a `Bisc8-XXXX` SoftAP and an HTTP setup portal at `http://192.168.4.1`, then generates a short Setup PIN and shows it on the e-paper.
- Captive probe HTTP routes redirect to `/`, and setup mode runs a small DNS responder that points queries to `192.168.4.1`. Press PWR to show the e-paper fallback instructions because captive detection can be unreliable.

## Product Setup Roadmap

Bisc8 is moving toward a no-recompile product setup flow:

- The local web UI serves responsive forms for Wi-Fi, language, OpenAI API key, email, status, and reset, and reports online/setup mode with the active device address.
- The web UI asks for the Setup PIN shown on the e-paper before sensitive POST actions: Wi-Fi, OpenAI, email, and reset. Status and Wi-Fi scan remain available without exposing secrets.
- API responses mask stored secrets; blank secret fields keep the currently stored value.
- The next web UI milestone will add richer validation and hardware QA for captive portal behavior across phones and laptops.
- OpenAI, Wi-Fi, and email relay secrets are stored on the device. Enable flash encryption before production use.
- Email delivery uses a simple recipient-first product flow. Production builds can provide a default HTTPS relay URL/token, while public firmware keeps those values empty so no provider secrets are committed to GitHub.
- The relay is responsible for sending through a provider such as Resend, SendGrid, or Mailgun and for falling back to text-only email if audio attachments are too large.

## Voice Oracle Flow

The intended online flow is:

1. Hold BOOT and speak a question.
2. Release BOOT, or hit the 15 second recording limit.
3. Bisc8 uploads the recorded audio for speech-to-text.
4. The model detects the question language and writes a contextual oracle answer in that language.
5. The e-paper shows a short answer of at most 100 characters.
6. Text-to-speech generates the spoken answer and Bisc8 plays it.
7. If email is enabled and a relay is configured, Bisc8 sends the transcript, full answer, and audio when feasible.

Offline fallback fortunes remain available when Wi-Fi or OpenAI settings are missing.

Audio is not stored in NVS. The firmware reserves a raw flash `spool` partition for temporary WAV payloads so 15 second questions do not have to fit in RAM. Voice recording writes a 16 kHz mono WAV payload at `spool://question.wav` in one-second chunks.

Current implementation status: recording, UI states, NVS OpenAI settings, and the response contract are in place. The actual OpenAI speech-to-text, Responses API, text-to-speech, generated-audio playback, and email relay transport are not implemented yet. `VoiceOracleService::AskFromRecordedAudio()` currently returns `ESP_ERR_NOT_FINISHED`.

## Sound Assets

Original candidate sounds live in:

```sh
assets/candidate
```

Generate firmware-ready preview WAV files and C arrays:

```sh
python3 tools/generate_sound_assets.py
```

The generated preview files are written to `assets/sounds/firmware`, and the firmware arrays are written to `firmware/bisc8_fortune/main/generated/sound_assets.*`. Current cues are `boot`, `oracle_button`, `voice_submit`, and `shutdown`. Runtime playback uses 16 kHz, 16-bit, stereo PCM so the firmware does not need an OGG or MP3 decoder.

## Email Relay

The local setup page asks for the recipient email first. Advanced fields allow a relay URL and bearer token to be entered when the firmware build does not already provide them.

The planned relay request is an HTTPS POST from Bisc8 to the configured endpoint with the recipient, detected language, transcript, full oracle answer, short screen answer, and generated audio when available. The relay owns provider credentials and sends the message through its configured mail provider. This keeps the device UI friendly and avoids asking end users for SMTP host, port, username, password, TLS mode, and sender policy details.

Firmware defaults can be set at build time with:

```sh
idf.py menuconfig
```

Then set `Bisc8 -> Default Bisc8 email relay URL` and, only for private/provisioned builds, `Bisc8 -> Default Bisc8 email relay token`. Do not commit production relay tokens.

## Logo Asset

Keep the boot logo source at `assets/logo/logo_min.png`. For new source artwork, a square `1024x1024` PNG is preferred, but the checked-in source may be any clean square image that scales well. Use pure black on white or transparency, no text, no gradients, and a strong silhouette readable at `64x64`. Regenerate the firmware image with:

```sh
python3 tools/generate_logo_assets.py
```

The tool scales the source to `64x64`, thresholds it to a clean 1-bit black/white image, and emits the LVGL firmware wrapper in `firmware/bisc8_fortune/main/generated/logo_assets.*` for the 200x200 e-paper boot screen.

## Display Fonts

Bisc8 display fonts are generated from Montserrat Medium with full Latin-1 coverage (`0x20-0xFF`) so localized Italian, Spanish, French, Portuguese, and similar European-language text keeps required accents. Do not strip accents from display strings to fit ASCII.

Regenerate the LVGL font sources with:

```sh
python3 tools/generate_display_fonts.py
```

## Firmware

The ESP-IDF project lives in:

```sh
firmware/bisc8_fortune
```

Build and flash:

```sh
source tools/idf_env.sh
python "$IDF_PATH/tools/idf_tools.py" install --targets esp32c6
python "$IDF_PATH/tools/idf_tools.py" install-python-env
idf.py -C firmware/bisc8_fortune build
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodemXXXX flash
```

`tools/idf_env.sh` chooses an architecture-specific ESP-IDF tools cache from `uname -m`, for example `~/.espressif-bisc8-x86_64` on Intel Macs and `~/.espressif-bisc8-arm64` on Apple Silicon. Keep toolchains out of the Dropbox-synced project folder, or use separate `.espressif-x86_64` and `.espressif-arm64` caches, because ESP-IDF extracts same-version tools to the same directory name for different host architectures.

The current firmware profile targets the 16 MB flash board and maps the full chip. The custom partition table keeps the app at `0x10000`, reserves a 6 MB app partition, a 5 MB raw `assets` partition for future bundled media/data, and a raw `spool` partition from `0xb10000` to the end of flash for recorded voice audio and temporary generated payloads. The `assets` partition is reserved but not mounted yet.

The local ESP-IDF checkout, toolchain cache, build products, and managed components are intentionally ignored by git.

## Public Web Flasher

The static browser flashing page lives in:

```sh
public/flash
```

After a successful build, copy the bootloader, partition table, and app binary into the public flasher folder:

```sh
python3 tools/prepare_web_flash.py --build-dir path/to/bisc8-fortune-build
```

The page uses ESP Web Tools with the ESP32-C6 offsets `0x0`, `0x8000`, and `0x10000`. Host it over HTTPS or localhost, then send users to the Bisc8 hotspot and `http://192.168.4.1` after flashing. Do not publish firmware images that contain private API keys, Wi-Fi credentials, or email relay tokens.

## Fortune Data

Source fortunes are stored in:

```sh
assets/grimorio.txt
assets/grimorio.en.txt
assets/grimorio.es.txt
```

The firmware embeds Italian, English, and Spanish grimoire arrays and picks the active one from the saved language setting. Each language file must keep the same line count, and each line must stay under 120 characters for the current 200x200 e-paper layout.

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
.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests
```

Latest local result:

```text
58 passed
```
