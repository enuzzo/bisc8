# Bisc8 OpenAI Voice Oracle

Updated: 2026-06-24

This is the authoritative in-repo contract for Bisc8's online oracle:

`recorded question -> STT -> oracle brain -> screen answer -> spoken answer -> optional email relay`

The matching implementation lives in:

- `firmware/bisc8_fortune/main/app_config.cpp`
- `firmware/bisc8_fortune/main/app_main.cpp`
- `firmware/bisc8_fortune/main/voice_oracle_service.cpp`
- `firmware/bisc8_fortune/main/email_service.cpp`
- `server/bisc8-email.php`
- `tests/test_voice_oracle_static.py`
- `tests/test_email_service_static.py`

## Current model map

| Phase | Current default | Endpoint / transport | Why this choice |
|---|---|---|---|
| Speech to text | `whisper-1` | `POST /v1/audio/transcriptions` | Stable request-based transcription for a bounded 16 kHz WAV question. |
| Oracle brain | `gpt-5.4-mini` | `POST /v1/chat/completions` | Text-only, compact enough for the ESP32-C6, returns a strict JSON object for screen, speech, and email. |
| Spoken answer | `gpt-realtime-2` | `wss://api.openai.com/v1/realtime?model=gpt-realtime-2` | Expressive 24 kHz PCM audio streamed over WebSocket into the answer spool. |
| Classic TTS fallback | configurable non-`gpt-realtime*` speech model, usually `gpt-4o-mini-tts` | `POST /v1/audio/speech` | Kept as an implementation fallback for request-based speech model strings. Not the default path. |
| Email relay | no OpenAI model | HTTPS multipart POST to `server/bisc8-email.php` | Device sends already-generated text plus compact audio review WAVs. |

Default settings are defined in `DefaultOpenAiSettings()`:

```cpp
settings.transcription_model = "whisper-1";
settings.response_model = "gpt-5.4-mini";
settings.speech_model = "gpt-realtime-2";
settings.voice = "echo";
settings.reasoning_effort = "";
```

Do not infer the live voice from defaults alone. A device may carry an older
saved voice in NVS; the serial log line is the source of truth for a run:

```text
[ORACLE] realtime tts model=gpt-realtime-2 voice=... wav=...B time=...ms
```

## Official OpenAI docs checked

Checked on 2026-06-24:

- Realtime overview: <https://developers.openai.com/api/docs/guides/realtime>
- Realtime WebSocket guide: <https://developers.openai.com/api/docs/guides/realtime-websocket>
- Realtime API reference: <https://developers.openai.com/api/reference/resources/realtime>
- GPT-Realtime-2 model page: <https://developers.openai.com/api/docs/models/gpt-realtime-2>
- Speech-to-text guide: <https://developers.openai.com/api/docs/guides/speech-to-text>
- Transcription API reference: <https://developers.openai.com/api/reference/resources/audio/subresources/transcriptions/methods/create>
- Text-to-speech guide: <https://developers.openai.com/api/docs/guides/text-to-speech>
- Speech API reference: <https://developers.openai.com/api/reference/resources/audio/subresources/speech/methods/create>
- GPT-5.4 mini model page: <https://developers.openai.com/api/docs/models/gpt-5.4-mini>

Important docs facts reflected in firmware:

- OpenAI recommends Realtime sessions for low-latency/live audio and request-based
  audio APIs for bounded file-style generation.
- `gpt-realtime-2` is the realtime voice model we use for the spoken answer.
- Realtime server audio events are `response.output_audio.delta` and
  `response.output_audio.done`; firmware also accepts the older
  `response.audio.*` names for compatibility.
- Realtime audio output can be requested as `audio/pcm` with `rate: 24000`.
- Realtime voice and output format must be pinned in `session.audio.output`.
- `session.update.session.type` must be `"realtime"`. Missing it caused the
  hardware failure on 2026-06-24.
- The Speech API still supports `gpt-4o-mini-tts`, voices such as `ash` and
  `echo`, `instructions`, and `response_format: "pcm"`/`"wav"`.
- The Transcriptions API supports `whisper-1`, WAV inputs, `prompt`, and
  `temperature`.

## Recording and spool contract

The question is recorded by `AudioService` as:

- 16 kHz
- mono
- PCM16 WAV
- max 15 seconds
- stored at offset `0` of the raw `spool` partition
- logical path `spool://question.wav`

The spoken answer is written separately:

- Realtime output target: 24 kHz mono PCM16
- local WAV header patched after all deltas arrive
- stored at `kVoiceAnswerSpoolOffset`
- full-quality answer WAV is only for local playback and portal download

Never try to hold full question or answer audio in RAM. Stream from flash to
OpenAI, to playback, and to the email relay.

## STT request shape

Implementation: `VoiceOracleService::Transcribe()`

Transport:

```http
POST https://api.openai.com/v1/audio/transcriptions
Authorization: Bearer <OPENAI_API_KEY>
Content-Type: multipart/form-data; boundary=<firmware boundary>
```

Form fields:

| Field | Value |
|---|---|
| `model` | `openai.transcription_model`, default `whisper-1` |
| `prompt` | `kTranscriptionPrompt` |
| `temperature` | `0` |
| `file` | `question.wav`, `audio/wav`, streamed from spool offset `0` |

Firmware retries STT with a fresh HTTP client. A successful run logs:

```text
[ORACLE] stt model=whisper-1 wav=...B status=200 time=...ms attempt=1
[ORACLE] stt ok transcript='...'
```

## Brain request shape

Implementation: `VoiceOracleService::GenerateAnswer()`

Transport:

```http
POST https://api.openai.com/v1/chat/completions
Authorization: Bearer <OPENAI_API_KEY>
Content-Type: application/json
```

Body shape:

```json
{
  "model": "gpt-5.4-mini",
  "messages": [
    { "role": "system", "content": "<kBrainSystemPrompt>" },
    { "role": "user", "content": "<language and transcript instruction>" }
  ],
  "response_format": { "type": "json_object" },
  "max_completion_tokens": 700,
  "temperature": 0.85
}
```

If `openai.reasoning_effort` is non-empty, firmware sends
`"reasoning_effort": "<value>"` and intentionally omits `temperature`, because
reasoning-capable models may reject custom temperature.

The model must return JSON text in the assistant message content. Required or
derived fields:

| Field | Use |
|---|---|
| `detected_language` | Email metadata and fallback language state. |
| `oracle_answer_full` | Full answer for email body and fallback TTS text. |
| `oracle_answer_screen` | Short e-paper answer, sanitized and capped to 55 chars. |
| `tts_text` | Exact text to speak. If missing, derived from full/screen answer. |
| `voice_direction` | Optional performance direction prepended to Realtime instructions. |

Successful run:

```text
[ORACLE] brain POST model=gpt-5.4-mini transcript_len=... free_heap=...
[ORACLE] brain ok lang=... time=...ms screen='...'
[ORACLE] brain voice_dir='...' tts='...'
```

## Realtime spoken-answer request shape

Implementation: `SynthesizeRealtime()`

Transport:

```text
wss://api.openai.com/v1/realtime?model=<openai.speech_model>
Authorization: Bearer <OPENAI_API_KEY>
User-Agent: bisc8/1.0
```

Current default:

```text
model = gpt-realtime-2
voice = echo, unless NVS/portal has a saved supported voice
output format = audio/pcm @ 24000 Hz
```

Event order:

1. Wait for server `session.created`.
2. Send `session.update`.
3. Send `conversation.item.create` containing the exact `tts_text`.
4. Send `response.create`.
5. Stream `response.output_audio.delta` into the answer spool.
6. On `response.output_audio.done` and `response.done`, patch WAV header and return.

The critical `session.update` shape:

```json
{
  "type": "session.update",
  "session": {
    "type": "realtime",
    "audio": {
      "output": {
        "format": { "type": "audio/pcm", "rate": 24000 },
        "voice": "echo"
      }
    }
  }
}
```

Do not remove `"session": { "type": "realtime" }`. Without it, OpenAI returns:

```text
Missing required parameter: 'session.type'.
```

The conversation item:

```json
{
  "type": "conversation.item.create",
  "item": {
    "type": "message",
    "role": "user",
    "content": [{ "type": "input_text", "text": "<tts_text>" }]
  }
}
```

The response request:

```json
{
  "type": "response.create",
  "response": {
    "output_modalities": ["audio"],
    "audio": {
      "output": {
        "format": { "type": "audio/pcm", "rate": 24000 },
        "voice": "echo"
      }
    },
    "instructions": "<voice_direction + exact reading instruction>"
  }
}
```

Successful hardware logs:

```text
[ORACLE] realtime session.created
[ORACLE] realtime first audio chunk=...B
[ORACLE] realtime audio.done chunks=... bytes=...
[ORACLE] realtime done status=completed audio=...
[ORACLE] realtime tts model=gpt-realtime-2 voice=... wav=...B time=...ms
[AUDIO] answer WAV rate=24000 ch=1 bytes=...
[AUDIO] answer playback done rate=24000 resample=1 ... result=ESP_OK
```

## Classic Speech API fallback

Implementation: `VoiceOracleService::Synthesize()` when the speech model does
not start with `gpt-realtime`.

Transport:

```http
POST https://api.openai.com/v1/audio/speech
Authorization: Bearer <OPENAI_API_KEY>
Content-Type: application/json
```

Body shape:

```json
{
  "model": "gpt-4o-mini-tts",
  "voice": "ash",
  "input": "<tts_text>",
  "instructions": "Parla come fossi un mago che recita una profezia misteriosa.",
  "response_format": "pcm"
}
```

Only send `instructions` for models that support them. The Speech API path writes
raw 24 kHz PCM to the same answer spool and patches a local WAV header after the
download.

## Email relay contract

Implementation:

- Firmware: `EmailService::SendOracleEmail()`
- Host relay: `server/bisc8-email.php`

The device POSTs `multipart/form-data` to `settings.email.relay_url`, with the
relay token both as `Authorization: Bearer <token>` and a form field named
`token` because some shared hosts strip the `Authorization` header.

Text fields:

| Field | Value |
|---|---|
| `token` | Shared relay token. |
| `to` | Stored recipient from device config. The host still pins delivery to server-side `mail_to`. |
| `transcript` | User question transcript. |
| `answer` | Full oracle answer text. |
| `lang` | Detected language. |
| `stt_model` | Model used by STT. |
| `brain_model` | Model used by brain step. |
| `tts_model` | Model used by speech step. |
| `voice` | Voice used by speech step. |

File fields:

| Field | Filename | Content |
|---|---|---|
| `audio` | `question.wav` | 8 kHz mono PCM16 review copy, capped to about 8 s. |
| `answer_audio` | `answer.wav` | 8 kHz mono PCM16 review copy, capped to about 12 s. |

Important invariant: the email attachments are review copies, not the local
playback WAVs. The full 24 kHz answer remains in flash for playback. Sending
full-size question/answer WAVs after Realtime fragments heap and can make TLS
fail before `esp_http_client_open()`.

Fallback order:

1. `compact8k`: question + answer review WAVs.
2. `question8k`: question review WAV only.
3. `text`: no audio attachments.

Successful hardware log from the 2026-06-24 fix:

```text
[EMAIL] relay payload mode=compact8k length=297418 question=104044B answer=192044B
[EMAIL] relay POST status=202 question=104044B answer=192044B mode=compact8k attempt=1
```

`202 Accepted` means the relay accepted and read the payload. It does not prove
the final mailbox delivery; check `bisc8-email.log.php` on the host for
`mail_result` if delivery is in doubt.

## 2026-06-24 regression and fix

User-visible failure:

- Screen showed only the short answer.
- No spoken answer.
- No email.

Root causes from serial logs:

1. Realtime `session.update` omitted `session.type`.
2. Email path tried oversized/full payloads after the Realtime phase, leaving
   too little contiguous heap for TLS.

Device evidence before the fix:

```text
[ORACLE] realtime error code=missing_required_parameter msg=Missing required parameter: 'session.type'.
[ORACLE] realtime failed done=0 audio=0 sink_error=0
[ORACLE] tts failed; showing screen answer without audio
[EMAIL] relay payload mode=legacy length=217222 question=216044B answer=0B
mbedtls_ssl_setup returned -0x7F00
[EMAIL] relay open failed: ESP_ERR_HTTP_CONNECT mode=legacy attempt=1
```

Fixes landed:

- Add `BuildRealtimeSessionUpdate()` with `session.type = "realtime"`.
- Send `session.update` after `session.created` and before the conversation item.
- Pin Realtime `audio.output.format` to `audio/pcm` at 24 kHz.
- Pin Realtime `audio.output.voice` at the session level to avoid voice drift.
- Rework relay attachments into 8 kHz review WAVs streamed from flash.
- Restore the relay fallback ladder: `compact8k -> question8k -> text`.
- Update tests so the bad request shape and oversized email path are caught.

Verification after the fix:

```text
python3 -m pytest tests -q
# 125 passed

idf.py -C firmware/bisc8_fortune build
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodem83201 flash
```

Hardware proof:

```text
[ORACLE] realtime first audio chunk=19200B
[ORACLE] realtime audio.done chunks=34 bytes=652800
[ORACLE] realtime done status=completed audio=652800 detail=
[ORACLE] realtime tts model=gpt-realtime-2 voice=ash wav=652844B time=8173ms
[AUDIO] answer playback done rate=24000 resample=1 gain=260% raw_peak=14053 max_safe_gain=233% result=ESP_OK
[EMAIL] relay POST status=202 question=104044B answer=192044B mode=compact8k attempt=1
```

## Before changing this system

Run these checks before claiming completion:

```sh
python3 -m pytest tests -q
git diff --check
```

For firmware changes, also build and flash:

```sh
export BISC8_IDF_TOOLS_PATH="$PWD/.espressif"
export BISC8_IDF_PYTHON_ENV_PATH="$PWD/.espressif/python_env/idf5.5_py3.14_env"
source tools/idf_env.sh
idf.py -C firmware/bisc8_fortune build
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodem83201 flash
```

Then run a real voice cycle and confirm all four legs:

- STT log has `status=200`.
- Brain log has `brain ok`.
- Realtime log has audio delta, audio done, response done, and local playback
  `result=ESP_OK`.
- Email log has `relay POST status=202` in one of the expected fallback modes.

## Future migration notes

- If replacing `gpt-5.4-mini`, migrate the brain request deliberately. The
  current firmware uses Chat Completions because it is implemented and hardware
  verified; do not silently switch to Responses without retesting JSON parsing,
  heap, and latency on the device.
- If replacing `gpt-realtime-2`, first verify the new model accepts the same
  Realtime session shape, `audio/pcm` output at 24 kHz, and the chosen built-in
  voice.
- If moving back to request-based Speech API as the default, keep the Realtime
  tests and docs explicit so future agents understand which path is live.
- If changing email attachment quality, use `tools/audio_qa.py --simulate-email`
  and then verify on hardware. Review audio must remain small enough for TLS
  reliability on the ESP32-C6.
