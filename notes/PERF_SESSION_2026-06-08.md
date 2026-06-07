# Performance session — voice oracle latency (2026-06-08, night)

User report: "after the voice question, a very long time passes before the answer,
the device seems frozen". Goal: find exactly where the time goes (GPT vs our
processing vs network), then make the experience fluid / near real-time, and try
the instant gpt voice model as the default.

## What I measured (BEFORE — on hardware, firmware with tts-1-hd)

Triggered ambient queries over serial (`tools/measure_oracle.py`) and read the
device's own `time=` numbers per stage. Each OpenAI stage is a separate blocking
HTTPS call with its own TLS handshake.

| Stage | Device time (measured) | What it is | Bound by |
|-------|------------------------|------------|----------|
| STT (whisper-1)      | 3 s … 24 s   | upload the question WAV (8–88 KB) + transcribe | **network upload** |
| brain (gpt-5.4-mini) | 3 s … 4 s    | GPT generates the answer JSON | GPT (fast) |
| TTS (tts-1-hd)       | 8 s … **112 s** | download the **~330–410 KB uncompressed** answer WAV | **network download** |
| our processing       | milliseconds | JSON parse, flash read/write | negligible |

Representative runs (same evening, variable home Wi-Fi):
- stt=3113ms brain≈4005ms tts=8349ms  → total ≈ 15 s
- stt=3064ms brain≈4298ms tts=24315ms → total ≈ 32 s
- stt=24296ms ………… tts=112033ms      → total ≈ 130 s (very slow network moment)

### Conclusion
The latency is **almost entirely network audio-transfer**, not GPT and not our
code. The brain (the actual "AI thinking") is only ~3–4 s. The two big, highly
variable costs are uploading the question recording (STT) and **downloading the
~350 KB uncompressed answer WAV (TTS)**. On a weak network the TTS download alone
can be 1–2 minutes.

Two structural problems made it FEEL frozen on top of being slow:
1. `app_main` blocked through STT+brain+TTS and only painted the answer AFTER the
   slow TTS — so you stared at "thinking" for the whole time even though the text
   answer existed after ~brain (~6–10 s).
2. The classic `tts-1-hd` model **silently ignores** the `instructions` field (the
   expressive per-answer voice direction) — that is a `gpt-4o-mini-tts` feature.

## What I changed (committed 5572c70)

1. **Two-phase flow — show the answer before the TTS download.**
   `VoiceOracleService` split into `AskTextAnswer` (STT+brain) and `SpeakAnswer`
   (TTS), run on separate workers. `app_main` paints `ShowVoiceSpeaking(answer)`
   the moment the brain returns, THEN fetches + plays the audio. Perceived
   latency for the *visible* answer drops from `STT+brain+TTS` to `STT+brain`.
2. **Instant voice model as default.** `speech_model` default → `gpt-4o-mini-tts`
   (the instant gpt voice): honours `instructions` (expressive delivery) and
   speaks `coral` natively. Migration heals a stale `tts-1*` model UP to it.
   Deliberate exception to the "no gpt-4o" rule, for the VOICE only — STT/brain
   stay non-4o (whisper-1 / gpt-5.4-mini). See the tension note below.
3. **Diagnostics.** brain step logs `time=` and `free_heap`; flow start logs
   `free_heap`. `tools/measure_oracle.py` added (the harness for these numbers).

Static tests updated + green (96/96). Firmware builds clean (57% free).

## AFTER — PENDING HARDWARE (port vanished before I could flash)

The USB-JTAG port (`/dev/cu.usbmodem14201`) re-enumerated and disappeared after a
reset during testing (known issue: needs a physical replug). I could not flash
the optimized firmware or take AFTER numbers. The device is NOT bricked — the
flash failed at connect, so it still runs the previous firmware.

**First thing on next connect:**
```
# flash the optimized firmware
export BISC8_IDF_TOOLS_PATH="$PWD/.espressif"; . tools/idf_env.sh
python "$IDF_PATH/tools/idf.py" -C firmware/bisc8_fortune -B "$HOME/bisc8-build" -p /dev/cu.usbmodem14201 flash
# confirm migration: boot log "[CONFIG] healed ... (voice coral)" and speech now gpt-4o-mini-tts
# measure AFTER:
python3 tools/measure_oracle.py 3.0 4 after-instant
```
Expected AFTER:
- The **text answer appears in ~STT+brain (~6–28 s)** instead of after TTS — the
  freeze is gone; you read the answer while the voice is fetched.
- Total audio time is still network-bound (~same total bytes). Confirm whether
  `gpt-4o-mini-tts` generation latency differs from `tts-1-hd` (expectation:
  similar download, but expressive voice restored). If it is SLOWER to start,
  reconsider.

## The real remaining lever (the big one — next session)

The TTS **download size** is the dominant, most variable cost (~350 KB
uncompressed PCM/WAV). Options, in order of value/effort:
1. **Compressed TTS format** (`response_format: opus`/`mp3`/`aac`): ~10× smaller
   download → seconds instead of minutes on a weak network. Needs an on-device
   decoder feeding I2S (opus is the natural fit). Biggest real win; moderate work.
2. **Stream the audio and play as it arrives** (start playback on the first
   chunk). Caveat: raw PCM streaming **stutters** if the network is slower than
   playback (24 kHz × 2 B = 48 KB/s) — exactly our bad-network case. Combine with
   (1) (compressed) or a pre-roll buffer to avoid I2S starvation.
3. **OpenAI Realtime API** (`gpt-realtime`, WebSocket, audio in/out): true
   low-latency conversational audio. Largest rearchitecture; the ultimate "instant".
4. Cheap wins to also try: keep-alive / reuse ONE TLS connection across the 3
   OpenAI calls (saves ~1–2 s per handshake ×3); bigger RX buffer for the TTS
   download; cap `tts_text` length a touch.

## Tension to confirm with the user
Earlier ask: "deprecate gpt-4o everywhere." Tonight's ask: "try the instant gpt
voice model as default." These conflict for TTS — `coral` + expressive
`instructions` + low latency are `gpt-4o-mini-tts` features (a gpt-4o model). I
followed the latest instruction: TTS = `gpt-4o-mini-tts`; STT/brain remain non-4o.
If you want a strictly gpt-4o-free build, the cost is the classic tts-1-hd voice
(no `instructions`, fewer voices). Flag for confirmation.
