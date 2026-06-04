# Bisc8

Bisc8 is an ESP-IDF firmware for the Waveshare ESP32-C6-ePaper-1.54 black-and-white board. It turns the device into a small e-paper fortune teller with BOOT/PWR controls, audio feedback, serial debugging, and framebuffer snapshots.

## Current Behavior

- First boot defaults to English.
- BOOT click: pick a random non-repeating fortune from the generated grimoire data and play a short digital chime.
- Hold BOOT to ask a voice question. Release BOOT to send it to the voice-oracle flow when OpenAI settings are configured.
- BOOT long press forces Wi-Fi setup. BOOT + PWR long press performs a full configuration reset.
- PWR click: run the microphone record/playback test.
- PWR long press: show the Bisc8 power-off prompt, then enter deep sleep wakeable by PWR.
- Idle timeout: after 3 minutes with no button or serial events, enter deep sleep wakeable by BOOT or PWR.
- Serial commands: `DEBUG 0`, `DEBUG 1`, `STATUS`, `SNAP`, `FORTUNE`, `MIC`, `VOICE START`, `VOICE STOP`, `WIFI SETUP`, `WIFI RESET`, `CONFIG RESET`, `HELP`.
- Configuration is stored in NVS: language, up to 8 Wi-Fi credentials, OpenAI settings, email recipient, and optional email relay settings.
- On boot, Bisc8 scans for saved SSIDs, tries visible known networks for 5 seconds each, and starts setup mode when none connects.
- Setup mode starts a `Bisc8-XXXX` SoftAP and an HTTP setup portal at `http://192.168.4.1`.
- Captive probe HTTP routes redirect to `/`, and setup mode runs a small DNS responder that points queries to `192.168.4.1`. The e-paper fallback still shows manual connection instructions because captive detection can be unreliable.

## Product Setup Roadmap

Bisc8 is moving toward a no-recompile product setup flow:

- The local web UI serves responsive forms for Wi-Fi, language, OpenAI API key, email, status, and reset.
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

## Email Relay

The local setup page asks for the recipient email first. Advanced fields allow a relay URL and bearer token to be entered when the firmware build does not already provide them.

The planned relay request is an HTTPS POST from Bisc8 to the configured endpoint with the recipient, detected language, transcript, full oracle answer, short screen answer, and generated audio when available. The relay owns provider credentials and sends the message through its configured mail provider. This keeps the device UI friendly and avoids asking end users for SMTP host, port, username, password, TLS mode, and sender policy details.

Firmware defaults can be set at build time with:

```sh
idf.py menuconfig
```

Then set `Bisc8 -> Default Bisc8 email relay URL` and, only for private/provisioned builds, `Bisc8 -> Default Bisc8 email relay token`. Do not commit production relay tokens.

## Logo Asset

Create the source logo as a square `1024x1024` PNG. Use pure black on white or transparency, no text, no gradients, and a strong silhouette readable at `64x64`. The firmware pipeline will convert it into a clean `64x64` 1-bit bitmap for the 200x200 e-paper boot screen.

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

## Public Web Flasher

The static browser flashing page lives in:

```sh
public/flash
```

After a successful build, copy the bootloader, partition table, and app binary into the public flasher folder:

```sh
python3 tools/prepare_web_flash.py --build-dir /private/tmp/bisc8-fortune-build12
```

The page uses ESP Web Tools with the ESP32-C6 offsets `0x0`, `0x8000`, and `0x10000`. Host it over HTTPS or localhost, then send users to the Bisc8 hotspot and `http://192.168.4.1` after flashing. Do not publish firmware images that contain private API keys, Wi-Fi credentials, or email relay tokens.

## Fortune Data

Source fortunes are stored in:

```sh
assets/grimorio.txt
```

Localized grimoire assets will live beside it as `assets/grimorio.en.txt` and `assets/grimorio.es.txt`. Each line must stay under 120 characters for the current 200x200 e-paper layout.

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
39 passed
```
