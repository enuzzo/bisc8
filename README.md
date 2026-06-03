# Bisc8

Bisc8 is an ESP-IDF firmware for the Waveshare ESP32-C6-ePaper-1.54 black-and-white board. It turns the device into a small e-paper fortune teller with BOOT/PWR controls, audio feedback, serial debugging, and framebuffer snapshots.

## Current Behavior

- First boot defaults to English.
- BOOT click: pick a random non-repeating fortune from the generated grimoire data and play a beep.
- Hold BOOT to ask a voice question. Release BOOT to send it to the voice-oracle flow when OpenAI settings are configured.
- BOOT long press forces Wi-Fi setup. BOOT + PWR long press performs a full configuration reset.
- PWR click: run the microphone record/playback test.
- PWR long press: show the Bisc8 power-off prompt, then enter deep sleep wakeable by PWR.
- Idle timeout: after 3 minutes with no button or serial events, enter deep sleep wakeable by BOOT or PWR.
- Serial commands: `DEBUG 0`, `DEBUG 1`, `STATUS`, `SNAP`, `FORTUNE`, `MIC`, `WIFI SETUP`, `WIFI RESET`, `CONFIG RESET`, `HELP`.
- Configuration is stored in NVS: language, up to 8 Wi-Fi credentials, OpenAI settings, SMTP settings, and recipient email.
- On boot, Bisc8 scans for saved SSIDs, tries visible known networks for 5 seconds each, and starts setup mode when none connects.
- Setup mode starts a `Bisc8-XXXX` SoftAP and an HTTP setup portal at `http://192.168.4.1`.
- Captive probe HTTP routes redirect to `/`; the e-paper fallback still shows manual connection instructions because captive detection can be unreliable.

## Product Setup Roadmap

Bisc8 is moving toward a no-recompile product setup flow:

- The local web UI serves responsive forms for Wi-Fi, language, OpenAI API key, SMTP, status, and reset.
- API responses mask stored secrets; blank secret fields keep the currently stored value.
- The next web UI milestone will add richer validation and hardware QA for captive portal behavior across phones and laptops.
- OpenAI, Wi-Fi, and SMTP secrets are stored on the device. Enable flash encryption before production use.
- SMTP is direct from the device with user-provided SMTP host, port, security, username, password, sender, and recipient.

## Voice Oracle Flow

The intended online flow is:

1. Hold BOOT and speak a question.
2. Release BOOT, or hit the 15 second recording limit.
3. Bisc8 uploads the recorded audio for speech-to-text.
4. The model detects the question language and writes a contextual oracle answer in that language.
5. The e-paper shows a short answer of at most 100 characters.
6. Text-to-speech generates the spoken answer and Bisc8 plays it.
7. If SMTP is configured, Bisc8 sends the transcript, full answer, and audio when feasible.

Offline fallback fortunes remain available when Wi-Fi or OpenAI settings are missing.

Audio is not stored in NVS. The firmware reserves a flash `spool` partition for temporary WAV/PCM payloads so 15 second questions do not have to fit in RAM.

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
33 passed
```
