# Handoff (next session)

## Latest session: capture audio quality fix + answer-audio email (2026-06-05 pm)

**Goal:** the user's recorded question reached speech-to-text as gibberish (the
emailed `domanda.wav` sounded terrible), while the early record->playback
loopback was excellent.

**Root cause (high confidence): mic clipping.** `Codec_StartInit` set the ES8311
`in_gain` to **45.0 dB**, but the chip's mic PGA tops out at **42 dB**
(`es8311_set_mic_gain` buckets anything >=42 to the max), so the mic ran at
MAXIMUM analog gain. On this near-field pocket device that rails the ADC on
ordinary speech -> hard clipping -> STT hears phonetically-rich gibberish. The
ear (and the speaker loopback) tolerate clipping that STT does not, which is why
the SAME mic sounded "ottimo" in the loopback yet transcribed as nonsense, and
why it was intermittent (level-dependent): the emailed transcripts alternate
clean ("Pioverà domani?") and gibberish ("Manitroona trimulindusperdu") minutes
apart -- not a uniform sample-rate error, and never empty (loud-distorted, not
weak/far).

**Fix:** `port_codec.cpp` `in_gain` 45.0 -> **24.0 dB** (strong but unclipped for
close talk, with headroom). One change, one variable -- to be confirmed by
measurement on-device (below).

**Also shipped this session:**
- **Capture diagnostics** (`audio_service.cpp`): one `[VOICEDIAG]` serial line per
  recording -- per-channel + mono peak, RMS dBFS, clip%, worst codec-read /
  flash-write stall (ms), and wall-clock vs frame-derived duration. This is the
  test harness: it names the defect instead of guessing.
- **`tools/analyze_question_wav.py`**: desktop twin of `[VOICEDIAG]` -- run it on
  any emailed `domanda.wav` to get clip%/dBFS/dropout/per-channel verdict
  (CLIPPING vs WEAK/FAR vs DROPOUTS vs levels-OK). Stdlib only.
- **Answer audio in the email** (`email_service.*`, `bisc8-email.php`,
  `app_main.cpp`): the verification email now carries transcript + answer text +
  `domanda.wav` (question) + `risposta.wav` (the TTS answer) -- text and BOTH
  voice samples per query. The TTS WAV's OpenAI "unknown length" (0xFFFFFFFF)
  header is repaired to a real length so desktop players open it.

**>>> NEXT SESSION: run the voice sampling test to verify the fix <<<**
1. Firmware is already built at `$HOME/bisc8-build`. Plug the device in, find the
   port (`ls /dev/cu.usbmodem*` -- it re-enumerates on replug), then flash +
   monitor: `idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" -p <PORT> flash monitor`
   (set the env first; see Build/flash below).
2. Hold BOOT, speak a clear phrase, release. Read the `[VOICEDIAG]` line:
   - `clip=` should now be ~0% and `peakMono` well under 32000 (was the bug if high).
   - `rms_dBFS` healthy ≈ -20..-12. If `< -35` it's too quiet -> raise gain
     (`port_codec.cpp` 24 -> 30). If `clip` still > 0.5% -> still hot -> lower (24 -> 18).
   - `peakL` vs `peakR`: if one is ~0, the codec gives mono in a single slot ->
     change the downmix to take that channel instead of `(L+R)/2`.
   - `wall` ≈ `dur` (and small `maxRead`/`maxWrite`) confirms NO stalls -> rules
     out dropouts. If `wall` >> `dur` or the stall times spike, revisit decoupling
     the I2S read from the flash write.
3. The email now has `domanda.wav` + `risposta.wav`. Run
   `python3 tools/analyze_question_wav.py <domanda.wav>` -> expect "levels look OK",
   clip ~0, and a duration matching how long you actually spoke (mismatch => rate).
4. Confirm the transcript matches what you said. Repeat 2-3 phrases at different
   volume/distance to dial in the best gain, then lock it in.

---

Last session: the **online GPT voice oracle**, end to end, plus a lot of polish.
Date: 2026-06-05.

## State: working on hardware

Hold BOOT, speak a question, release: Bisc8 records, transcribes, asks the LLM,
speaks the answer aloud (coral voice), shows it on screen, and (optionally) emails
the transcript + answer + the original recording. Confirmed working by the user.

Host tests: `82 passed` (`/opt/homebrew/bin/python3 -m pytest tests/` — any arm64
python with pytest; the in-tree `idf5.5_py3.14_env` lacks pytest).

## The voice oracle pipeline

`VoiceOracleService::AskFromRecordedAudio` (`main/voice_oracle_service.cpp`):
1. **STT** -> `POST /v1/audio/transcriptions`, multipart, the WAV streamed straight
   from the `spool` flash partition (offset 0), model `gpt-4o-mini-transcribe`.
2. **Brain** -> `POST /v1/chat/completions`, model from settings, returns JSON
   (`detected_language`, `oracle_answer_screen`, `oracle_answer_full`, `tts_text`,
   `voice_direction`). `response_format=json_object`, `max_completion_tokens`,
   `reasoning_effort` sent only when set (else `temperature`).
3. **TTS** -> `POST /v1/audio/speech` (wav), streamed to the answer spool region.
   `instructions` = a pinned "warm mystical seer" style + the per-answer
   `voice_direction`. Voice defaults to `coral`.
4. Playback: `AudioService::PlayAnswerAudio` reads the answer WAV from flash.

### Non-obvious things (read before touching audio/config)

- **The flow runs on a dedicated 16 KB-stack worker** (`RunOracleOnWorker` /
  `RunEmailOnWorker` in `app_main.cpp`). The main task has only 3584 B; the mbedTLS
  handshake overflows it (stack-protection panic). Any new TLS call must run on a
  worker too.
- **The TTS is 24 kHz but plays at 16 kHz.** The ES8311 is opened in TDM mode and
  reopening the codec to 24 kHz does NOT retune the I2S clock (it would play 1.5x
  slow + an octave low). `PlayAnswerAudio` therefore **resamples 24->16 kHz**
  (exact 3:2, integer) and keeps the codec native. Side effect: some HF loss
  ("muffled") because the 16 kHz Nyquist is 8 kHz. A real fix is making the codec
  actually run at 24 kHz.
- **OpenAI streams the answer WAV with a 0xFFFFFFFF "unknown length" data size.**
  Playback trusts `AnswerAudioBytes()` (bytes actually written), not the header.
- Quiet TTS is lifted by `kAnswerGainPercent` (currently 150, clipped). Tunable.
- **Question spool is pre-erased at idle** (`RearmQuestionSpool`, at boot and after
  each query) so recording starts instantly; the 512 KB erase otherwise cost ~1-2 s
  at press time. The voice record buffer is a small 16 KB chunk (`kVoiceChunkBytes`)
  so it allocates even on a fragmented heap (64 KB failed).

### Config gotcha that bit us (reasoning_effort -> 400)

`ConfigStore::Load` = `ApplyDefaults` then per-field NVS overlay. A NEW
`OpenAiSettings` field gets the ApplyDefaults value, while the model fields come
from older NVS. So defaulting `reasoning_effort="low"` sent it to a user whose
saved model was `gpt-4o-mini` (which rejects it) -> HTTP 400 -> error screen. Now
`reasoning_effort` defaults empty and is portal-configurable. Add any new
model-coupled param the same way (empty default + portal field).

## Error codes (error-screen footer, "cod. EXX")

E01 recording failed, E02 no API key, E03 STT/network, E04 nothing heard (empty
transcript), E05 Brain. Ask the user to read the code; STT/Brain error bodies are
now logged (`[ORACLE] brain http status=.. body=..`).

## Models / voice / language (portal -> OpenAI section)

- All models are portal fields (`transcription_model`, `response_model`,
  `speech_model`); voice is a dropdown; `reasoning_effort` is a dropdown.
  Defaults: `gpt-4o-mini` / `gpt-4o-mini-tts` (proven), voice `coral`, reasoning off.
- Placeholders SUGGEST `gpt-5.4-mini` / `gpt-realtime-1.5` (user's choice) but these
  are **unverified** by us. `gpt-realtime-1.5` may be a Realtime-API model that does
  NOT work via `/v1/audio/speech` -> would E05. If the user sets them and it fails,
  read the logged body and adjust the name/params or rearchitect TTS to the Realtime
  API. The user's NVS still holds `gpt-4o-mini`; they must TYPE a new model to switch.
- Reply language follows the **transcript** (the words actually spoken, read by the
  model), not the STT language tag (which mislabels short speech). UI/device language
  is just the menus. User confirmed: reply follows what they speak.

## Email (works)

`EmailService::SendOracleEmail` POSTs multipart (token + transcript + answer text
+ the question WAV `audio` field + the answer WAV `answer_audio` field) to a
user-set `relay_url`. The relay is a **standalone PHP file we own**:
`server/bisc8-email.php` (+ `.config.example.php`, README). No email credentials
on the device, only the relay token. The user deploys it to their own host
(currently `https://netmi.lk/nomenomen/api/bisc8-email.php`) and it sends via the
host's `mail()`. First mail from a new domain often lands in spam.

**REDEPLOY the relay** to get `risposta.wav`: the answer-audio attachment needs
the updated `server/bisc8-email.php` on the host (it now reads the `answer_audio`
upload and attaches it). The firmware sends it regardless; an old relay just drops
it. The answer file field is `answer_audio` (not `answer`, which is the answer
TEXT field).

## UI added this session

- Listening screen: animated **mic glyph** + "Parla ora" (`BuildMic`,
  `StartVoiceAnim(true)`). No full refresh (it looked like a reset).
- Thinking screen: **"Consulto le briciole.."** + animated dots (`BuildWaitDots`,
  `StartVoiceAnim(false)`).
- A start-recording cue (`AudioCue::VoiceStart`, `start-talking.ogg`) plays at the
  true capture start (after the pre-erase), so it is a clear "go".
- New serial previews: `SCREEN ERROR|LISTENING|THINKING`.

## Pending / follow-ups

- **Confirm the mic-gain fix on-device** (see the sampling test above) and lock in
  the best value from `[VOICEDIAG]`. If it needs frequent tuning, consider making
  `in_gain` a portal field (like the model fields) so it's adjustable without a
  reflash.
- **Redeploy `server/bisc8-email.php`** on the host so the `risposta.wav` answer
  attachment actually goes out (the firmware already sends it).
- `docs/AI_HANDOFF.md` is **stale** (still says the OpenAI transport is not
  implemented / returns `ESP_ERR_NOT_FINISHED`); update it + its test assertions.
- Verify `gpt-5.4-mini` / `gpt-realtime-1.5` exist for the user's key.
- Voice quality at 16 kHz is "muffled"; revisit the 24 kHz codec path if it matters.
  (This is the ANSWER playback path, separate from the capture clipping fix.)
- `screenshots/epaper/` is git-ignored (dev SNAPs).

## Build / flash (this machine)

arm64 py3.14 env, build OUTSIDE Dropbox. See memory `bisc8-build-flash-workflow`:
`. tools/idf_env.sh` (with `BISC8_IDF_PYTHON_ENV_PATH` -> `idf5.5_py3.14_env`), then
`idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" build flash -p <port>`.
SNAP: `tools/capture_epaper_snapshot.py --before-command "SCREEN ..."`.
