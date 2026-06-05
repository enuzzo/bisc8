"""Static checks for the oracle email relay (firmware side + the PHP relay).

The verification email must carry, per query: the transcript text, the answer
text, the original question recording (domanda.wav) AND the generated answer
audio (risposta.wav), so a remote reviewer has text + both voice samples in one
place. These greps pin that wiring without needing a device or a PHP runtime.
"""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
EMAIL_CPP = MAIN / "email_service.cpp"
EMAIL_H = MAIN / "email_service.h"
APP_MAIN = MAIN / "app_main.cpp"
RELAY_PHP = ROOT / "server/bisc8-email.php"


def read(path):
    return path.read_text(encoding="utf-8")


def test_email_signature_takes_the_answer_audio_byte_count():
    header = read(EMAIL_H)
    # The TTS answer WAV lives in the spool with a known real length (the WAV
    # header's size field is an OpenAI "unknown length" placeholder), so the
    # caller must hand the true byte count in.
    assert "answer_audio_bytes" in header
    assert "SendOracleEmail" in header


def test_firmware_attaches_both_question_and_answer_wavs():
    src = read(EMAIL_CPP)
    # Question part (existing) + new answer part, each a multipart file field.
    assert '"question.wav"' in src
    assert '"answer.wav"' in src
    assert '"audio"' in src  # question file field
    # The answer FILE field must not reuse the "answer" text field name.
    assert '"answer_audio"' in src
    # The answer audio is streamed from its own reserved spool region.
    assert "kVoiceAnswerSpoolOffset" in src


def test_firmware_repairs_the_answer_wav_length_header_for_players():
    src = read(EMAIL_CPP)
    # OpenAI streams the WAV with a 0xFFFFFFFF "unknown length" data size; the
    # attached file must carry real RIFF + data sizes or desktop players choke.
    assert "answer_audio_bytes" in src
    assert "RIFF" in src and "data" in src
    # A patched copy of the header is what gets sent (real sizes written in).
    assert "0xFFFFFFFF" in src or "ans_total" in src or "patch" in src.lower()


def test_app_main_passes_the_answer_byte_count_to_the_relay():
    app = read(APP_MAIN)
    # The same real byte count used for playback is forwarded to the email.
    assert "AnswerAudioBytes()" in app
    # The email worker now carries the answer byte count through to the relay.
    assert "answer_audio_bytes" in app


def test_relay_php_forwards_the_answer_attachment():
    php = read(RELAY_PHP)
    # The relay must accept the second upload field and attach it as risposta.wav.
    assert "answer_audio" in php
    assert "risposta.wav" in php
    # And keep the question attachment.
    assert "domanda.wav" in php
