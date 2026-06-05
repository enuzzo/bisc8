import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
PORT = ROOT / "firmware/bisc8_fortune/components/port_bsp"

ORACLE_CPP = MAIN / "voice_oracle_service.cpp"
ORACLE_H = MAIN / "voice_oracle_service.h"
APP_MAIN = MAIN / "app_main.cpp"
APP_CONFIG_H = MAIN / "app_config.h"
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


def test_voice_oracle_signature_and_response_contract():
    header = read(ORACLE_H)
    src = read(ORACLE_CPP)
    # Settings are passed in (key + models + voice live in ConfigStore).
    assert "AskFromRecordedAudio(const char *wav_path, const OpenAiSettings &openai, OracleResponse *response)" in header
    assert "HasAnswerAudio" in header
    # The Brain JSON maps onto the repo's OracleResponse field names.
    for field in ("oracle_answer_screen", "oracle_answer_full", "tts_text", "detected_language", "voice_direction"):
        assert field in src


def test_screen_answer_is_utf8_truncated_to_the_limit():
    src = read(ORACLE_CPP)
    assert "Utf8Truncate" in src
    # Truncation is applied to the screen answer at the configured limit.
    assert "Utf8Truncate(answer_screen_, kMaxScreenAnswerChars)" in src
    # And it respects multibyte boundaries rather than cutting raw bytes.
    assert "0xC0" in src and "0xE0" in src and "0xF0" in src


def test_stt_streams_the_wav_from_spool_as_multipart():
    src = read(ORACLE_CPP)
    assert "multipart/form-data" in src
    assert "boundary" in src
    assert "filename=" in src
    # Streamed from flash, not held in RAM.
    assert "esp_partition_read" in src
    assert "esp_http_client_write" in src


def test_tts_requests_wav_and_streams_to_the_answer_spool():
    src = read(ORACLE_CPP)
    config = read(APP_CONFIG_H)
    assert '"response_format"' in src and '"wav"' in src
    assert "kVoiceAnswerSpoolOffset" in src
    assert "esp_partition_write" in src  # streamed to flash
    # The answer region is reserved away from the question region.
    assert "kVoiceAnswerSpoolOffset" in config
    assert "kVoiceAnswerSpoolMaxBytes" in config


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
    # The flow is dispatched to a dedicated worker (see the stack test below),
    # which is the one that calls AskFromRecordedAudio.
    assert "RunOracleOnWorker(oracle, wav_path, settings.openai, &response)" in app
    assert "AskFromRecordedAudio(" in app
    assert "oracle.HasAnswerAudio()" in app
    # Playback is fed the real byte count, not the WAV header's size field.
    assert "audio.PlayAnswerAudio(oracle.AnswerAudioBytes())" in app
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


def test_tts_pins_a_mystical_spoken_style():
    # The oracle's spoken delivery is pinned in code (a warm mystical seer) so it
    # stays consistent; the model only adds per-answer emotional colour.
    src = read(ORACLE_CPP)
    assert "mystical seer" in src
    assert "quiet wonder" in src


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
    m = re.search(r'xTaskCreate\(\s*OracleTaskEntry\s*,\s*"[^"]*"\s*,\s*(\d+)', app)
    assert m is not None, "oracle flow must run on a dedicated xTaskCreate worker"
    assert int(m.group(1)) >= 12288
