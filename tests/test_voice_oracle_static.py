import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
PORT = ROOT / "firmware/bisc8_fortune/components/port_bsp"

ORACLE_CPP = MAIN / "voice_oracle_service.cpp"
ORACLE_H = MAIN / "voice_oracle_service.h"
APP_MAIN = MAIN / "app_main.cpp"
APP_CONFIG_H = MAIN / "app_config.h"
APP_CONFIG_CPP = MAIN / "app_config.cpp"
WEB_PORTAL_CPP = MAIN / "web_portal.cpp"
WEB_PORTAL_HTML = MAIN / "web_portal.html"
CMAKE = MAIN / "CMakeLists.txt"
PORT_CODEC_H = PORT / "port_codec.h"
PORT_CODEC_CPP = PORT / "port_codec.cpp"

EM_DASH = "—"
EN_DASH = "–"


def read(path):
    return path.read_text(encoding="utf-8")


def test_voice_oracle_uses_tls_http_client_and_endpoints():
    src = read(ORACLE_CPP)
    cmake = read(CMAKE)
    # HTTP + TLS via the cert bundle, pulled in as components.
    assert "esp_http_client" in cmake
    assert "mbedtls" in cmake
    assert "json" in cmake
    assert "esp_crt_bundle_attach" in src
    assert "esp_http_client_perform" in src or "esp_http_client_open" in src
    # The three OpenAI stages.
    assert "v1/chat/completions" in src
    assert "v1/audio/transcriptions" in src
    assert "v1/audio/speech" in src


def test_live_openai_defaults_and_portal_use_gpt4o_mini_tts():
    config = read(APP_CONFIG_CPP)
    portal = read(WEB_PORTAL_CPP)
    template = read(WEB_PORTAL_HTML)

    assert 'settings.transcription_model = "whisper-1"' in config
    assert 'settings.response_model = "gpt-5.4-mini"' in config
    assert 'settings.speech_model = "gpt-4o-mini-tts"' in config
    assert 'settings.voice = "ash"' in config

    for live_source in (config, portal, template):
        assert "gpt-4o-mini-tts" in live_source
        assert "gpt-5.4-mini" in live_source
    for web_source in (portal, template):
        assert 'placeholder="gpt-4o-mini-tts"' in web_source


def test_openai_voice_dropdown_uses_real_speech_voice_options():
    portal = read(WEB_PORTAL_CPP)
    template = read(WEB_PORTAL_HTML)
    expected = (
        "alloy",
        "ash",
        "ballad",
        "cedar",
        "coral",
        "echo",
        "fable",
        "marin",
        "nova",
        "onyx",
        "sage",
        "shimmer",
        "verse",
    )

    for source in (portal, template):
        assert 'select name="voice"' in source
        assert '<optgroup label="Voci maschili / profonde">' in source
        assert '<optgroup label="Voci femminili / chiare">' in source
        assert '<optgroup label="Voci neutre / narrative">' in source
        assert '<option value="ash">maschile - ash, profonda</option>' in source
        assert '<option value="nova">femminile - nova, brillante</option>' in source
        assert '<option value="fable">neutra - fable, narrativa</option>' in source
        for voice in expected:
            assert f'<option value="{voice}">' in source


def test_portal_status_exposes_current_openai_settings_for_prefill():
    portal = read(WEB_PORTAL_CPP)

    for key in (
        "openai_transcription_model",
        "openai_response_model",
        "openai_speech_model",
        "openai_voice",
        "openai_reasoning_effort",
    ):
        assert f'"{key}"' in portal
        assert f'data-fill="{key}"' in portal

    assert "document.querySelectorAll('[data-fill]')" in portal
    assert "el.tagName==='SELECT'" in portal


def test_config_migrates_realtime_tts_defaults_and_sanitizes_voices():
    config = read(APP_CONFIG_CPP)

    assert "MigrateOpenAiSettings" in config
    assert "IsRealtimeSpeechModel" in config
    assert "MigrateOpenAiSettings(settings, true)" in config
    assert "MigrateOpenAiSettings(&settings, false)" in config
    assert 'settings->openai.speech_model = defaults.speech_model' in config
    assert "openai.speech_model" in config
    assert "IsSupportedOpenAiVoice" in config
    assert 'settings->openai.voice = defaults.voice' in config


def test_tts1_speech_request_omits_expressive_instructions():
    src = read(ORACLE_CPP)

    assert "SpeechModelSupportsInstructions" in src
    assert "kSpeechInstructions" in src
    assert "Parla come fossi un mago che recita una profezia misteriosa." in src
    assert 'model.rfind("tts-1", 0) != 0' in src
    assert 'if (SpeechModelSupportsInstructions(openai.speech_model))' in src
    assert 'cJSON_AddStringToObject(root, "instructions", kSpeechInstructions)' in src


def test_realtime_speech_path_uses_websocket_audio_deltas():
    src = read(ORACLE_CPP)
    cmake = read(ROOT / "firmware/bisc8_fortune/main/CMakeLists.txt")

    assert "SynthesizeRealtime" in src
    assert "esp_transport_ws_init" in src
    assert "wss://api.openai.com/v1/realtime?model=" not in src
    assert '"/v1/realtime?model="' in src
    assert '"conversation.item.create"' in src
    assert '"response.create"' in src
    assert '"output_modalities"' in src
    assert 'cJSON_AddStringToObject(format, "type", "audio/pcm")' in src
    assert 'cJSON_AddNumberToObject(format, "rate", kRealtimeAudioRateHz)' in src
    assert '"response.audio.delta"' in src
    assert '"response.output_audio.delta"' in src
    assert "session_created" in src
    assert "session.created timed out" in src
    assert "WS_TRANSPORT_OPCODES_TEXT | WS_TRANSPORT_OPCODES_FIN" in src
    assert "ESP_TLS_DYN_BUF_RX_STATIC" in src
    assert "esp_transport_ssl_set_esp_tls_dyn_buf_strategy" in src
    assert "DecodeAndAppendRealtimeBase64" in src
    assert "StreamRealtimeAudioDeltaEvent" in src
    assert "ConsumeRealtimeAudioDeltaJson" in src
    assert "PendingRealtimeEventIsAudioDelta" in src
    assert "SkipOversizedRealtimeEvent" in src
    assert "RealtimeEventTypeNeedsFullJson" in src
    assert "realtime skipped oversized event" in src
    assert "event.streamed_audio_bytes" in src
    assert src.count("ReadRealtimeEvent(ws, &pending_event, buf, sizeof(buf), &event, &closed, &sink)") >= 2
    assert "constexpr size_t kRealtimeMaxJsonEventBytes = 12288" in src
    assert "constexpr size_t kRealtimeReadChunkBytes = 2048" in src
    assert 'JsonStr(j, "delta")' not in src
    assert '"session.update"' not in src
    assert '"session.updated"' not in src
    assert src.index("BuildRealtimeConversationItemCreate(tts_text)") < src.index(
        "BuildRealtimeResponseCreate(voice_direction, voice)"
    )
    assert "cJSON_ParseWithLengthOpts" in src
    assert "kRealtimeMaxJsonEventBytes" in src
    assert '"response.output_audio.done"' in src
    assert "mbedtls_base64_decode" in src
    assert "tcp_transport" in cmake


def test_voice_oracle_signature_and_response_contract():
    header = read(ORACLE_H)
    src = read(ORACLE_CPP)
    # Settings are passed in (key + models + voice live in ConfigStore).
    assert "AskFromRecordedAudio(const char *wav_path, const OpenAiSettings &openai, OracleResponse *response)" in header
    # Two-phase API: the text answer first (shown immediately), then the spoken audio.
    assert "AskTextAnswer(const char *wav_path, const OpenAiSettings &openai, OracleResponse *response)" in header
    assert "const std::string &device_language, OracleResponse *response" in header
    assert "SpeakAnswer(const OpenAiSettings &openai)" in header
    assert "HasAnswerAudio" in header
    # The Brain JSON maps onto the repo's OracleResponse field names.
    for field in ("oracle_answer_screen", "oracle_answer_full", "tts_text", "detected_language", "voice_direction"):
        assert field in src


def test_screen_answer_is_utf8_truncated_to_the_limit():
    src = read(ORACLE_CPP)
    assert "Utf8Truncate" in src
    assert "SanitizeScreenTextForDisplay" in src
    assert "ScreenFallbackForLanguage" in src
    assert "Latin-1 characters only" in src
    # Truncation is applied to the screen answer at the configured limit.
    assert "Utf8Truncate(answer_screen_, kMaxScreenAnswerChars)" in src
    assert src.index("answer_screen_ = SanitizeScreenTextForDisplay(answer_screen_)") < src.index(
        "Utf8Truncate(answer_screen_, kMaxScreenAnswerChars)"
    )
    # The e-paper fonts cover Latin-1; other scripts are display-only sanitized
    # so they cannot show up as tofu glyph boxes.
    assert "cp >= 0xA1 && cp <= 0xFF" in src
    assert "never use CJK, emoji, runes" in src
    # And it respects multibyte boundaries rather than cutting raw bytes.
    assert "0xC0" in src and "0xE0" in src and "0xF0" in src


def test_stt_streams_the_wav_from_spool_as_multipart():
    src = read(ORACLE_CPP)
    assert "multipart/form-data" in src
    assert "boundary" in src
    assert "filename=" in src
    assert 'name=\\"prompt\\"' in src
    assert 'name=\\"temperature\\"' in src
    assert "kTranscriptionPrompt" in src
    assert "do not invent subtitles" in src
    # Streamed from flash, not held in RAM.
    assert "esp_partition_read" in src
    assert "esp_http_client_write" in src


def test_oracle_uses_device_language_when_transcript_looks_ambiguous():
    app = read(APP_MAIN)
    src = read(ORACLE_CPP)

    assert "const std::string *device_language" in app
    assert "RunOracleTextOnWorker(oracle, wav_path, settings.openai, settings.language, &response)" in app
    assert "detected_language_ = fallback_language" in src
    assert "err = GenerateAnswer(openai, fallback_language)" in src
    assert "mostly non-Latin" in src
    assert "accidental captions" in src
    assert "err = GenerateAnswer(openai, detected_language_)" not in src


def test_tts_requests_wav_and_streams_to_the_answer_spool():
    src = read(ORACLE_CPP)
    config = read(APP_CONFIG_H)
    partitions = read(ROOT / "firmware/bisc8_fortune/partitions.csv")
    assert '"response_format"' in src and '"pcm"' in src
    assert "kVoiceAnswerSpoolOffset" in src
    assert "esp_partition_write" in src  # streamed to flash
    assert "WritePcm16WavHeader(spool, kVoiceAnswerSpoolOffset, 0)" in src
    assert "WritePcm16WavHeader(spool, kVoiceAnswerSpoolOffset, sink.written)" in src
    assert "FlashSink sink = {spool, kVoiceAnswerSpoolOffset + 44" in src
    assert "answer_audio_bytes_ = 44 + sink.written" in src
    # The answer region is reserved away from the question region.
    assert "kVoiceAnswerSpoolOffset" in config
    assert "kVoiceAnswerSpoolMaxBytes" in config
    assert "0x200000" in config
    assert "0x2f0000" in config
    assert "spool,    data, 0x40,    0xb10000,  0x4f0000" in partitions
    assert "tts response exceeded answer spool cap" in src


def test_generated_audio_playback_resamples_to_codec_rate():
    audio = read(MAIN / "audio_service.cpp")
    assert "PlayAnswerAudio" in audio
    assert '"RIFF"' in audio and '"data"' in audio
    # OpenAI returns 24 kHz; reopening the TDM codec to 24 kHz doesn't retune the
    # I2S clock (plays 1.5x slow + an octave low), so we resample 24->16 kHz and
    # keep the codec at its native rate instead of reopening it.
    assert "resample" in audio
    assert "rate == 24000" in audio
    assert "kAnswerGainPercent" in audio  # quiet TTS lifted with clipping


def test_voice_flow_is_wired_and_guards_on_audio_and_key():
    app = read(APP_MAIN)
    src = read(ORACLE_CPP)
    # Two-phase flow on dedicated workers (see the stack test below): phase 1
    # (STT + brain) yields the text answer; phase 2 (TTS) yields the audio.
    assert "RunOracleTextOnWorker(oracle, wav_path, settings.openai, settings.language, &response)" in app
    assert "AskTextAnswer(job->wav_path, *job->openai, *job->device_language, job->response)" in app
    assert "The device UI language is " in src
    assert "RunOracleSpeakOnWorker(oracle, settings.openai)" in app
    # Perceived-latency fix: the text answer is painted BEFORE the (slow) TTS phase
    # runs, so the screen never looks frozen while audio downloads.
    assert app.index("display.ShowVoiceSpeaking(response.oracle_answer_screen") < app.index("RunOracleSpeakOnWorker(oracle, settings.openai)")
    assert "oracle.HasAnswerAudio()" in app
    # Playback is fed the real byte count, not the WAV header's size field.
    assert "audio.PlayAnswerAudio(oracle.AnswerAudioBytes())" in app
    # The config portal is paused during the OpenAI/TLS-heavy section so Realtime
    # and email have the largest available heap block on the ESP32-C6.
    assert "pausing portal during voice flow to free heap" in app
    assert "portal.Stop()" in app
    assert "portal resume after voice flow" in app
    # No key -> clean error, no crash.
    assert "openai.api_key.empty()" in src


def test_api_key_is_never_logged():
    src = read(ORACLE_CPP)
    # The key is only ever used to build the Bearer header, never logged.
    assert '"Bearer "' in src
    for line in src.splitlines():
        if "DebugSerial" in line:
            assert "api_key" not in line


def test_oracle_failures_map_to_reportable_codes():
    # Empty transcript etc. must not all read "not configured": each failure maps
    # to a short message plus a code the user can read off-screen and report.
    app = read(APP_MAIN)
    src = read(ORACLE_CPP)
    header = read(ORACLE_H)
    loc_h = read(MAIN / "localization.h")
    assert "OracleFailure" in header and "LastFailure()" in header
    assert "OracleFailure::NoSpeech" in src  # empty transcript is its own case
    for code in ("E01", "E02", "E03", "E04", "E05"):
        assert code in app
    assert "voice_no_speech_body" in loc_h
    assert "voice_error_generic_body" in loc_h


def test_tts_pins_a_theatrical_spoken_style():
    # The oracle's spoken delivery is pinned in code (a theatrical seer in the
    # throes of a vision) so it stays consistent; the model only adds per-answer
    # emotional colour via voice_direction.
    src = read(ORACLE_CPP)
    assert "theatrical seer" in src
    assert "rises and falls" in src


def test_no_long_dashes_in_oracle_prompts():
    src = read(ORACLE_CPP)
    assert EM_DASH not in src
    assert EN_DASH not in src


def test_mic_gain_is_below_the_es8311_clipping_ceiling():
    # The ES8311 mic PGA tops out at 42 dB; a value >= 42 runs it flat out, and a
    # near-field pocket mic then rails the ADC -> clipped audio -> speech-to-text
    # hears phonetically-rich gibberish (while the ear-forgiving loopback sounds
    # fine). Keep the capture gain well under the ceiling, with headroom.
    src = read(PORT_CODEC_CPP)
    m = re.search(r"esp_codec_dev_set_in_gain\(record,\s*([0-9.]+)\)", src)
    assert m is not None, "the record path must set an explicit mic gain"
    gain = float(m.group(1))
    assert gain < 42.0, f"mic gain {gain} dB hits the ES8311 ceiling -> clipping"
    assert gain <= 30.0, f"mic gain {gain} dB is hot for a close-talk mic"


def test_voice_oracle_runs_on_a_dedicated_high_stack_task():
    # The OpenAI HTTPS calls (mbedTLS handshake) need far more stack than the
    # main task carries; running them inline panicked with a stack-protection
    # fault. They must run on a dedicated worker with a generous stack, so the
    # main task stays small and the 64 KB mic buffer still fits contiguous heap.
    app = read(APP_MAIN)
    m = re.search(r'xTaskCreate\(\s*OracleTextTaskEntry\s*,\s*"[^"]*"\s*,\s*(\d+)', app)
    assert m is not None, "oracle text phase must run on a dedicated xTaskCreate worker"
    assert int(m.group(1)) >= 12288
    m2 = re.search(r'xTaskCreate\(\s*OracleSpeakTaskEntry\s*,\s*"[^"]*"\s*,\s*(\d+)', app)
    assert m2 is not None, "oracle speak (TTS) phase must run on a dedicated xTaskCreate worker"
    # The Realtime TTS path streams audio to flash and uses a 2 KB read buffer;
    # keeping this stack compact leaves mbedTLS a >16 KB contiguous heap block
    # for TLS reads during long audio responses.
    assert 8192 <= int(m2.group(1)) <= 10240
