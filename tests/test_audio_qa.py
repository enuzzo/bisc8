"""Unit tests for tools/audio_qa.py — the bench twin of the firmware audio path.

The email_downsample() function must stay byte-for-byte equivalent to the
email_service.cpp encoder (same ratio math, same ceil byte count, same caps),
because we use it to preview email audio quality without a device.
"""

import array
import math

import pytest

from tools.analyze_question_wav import analyze_wav
from tools.audio_qa import (
    EMAIL_QUESTION_CAP_S,
    email_downsample,
    grade,
    high_band_ratio,
    write_mono16,
)


def sine(freq_hz, rate, seconds, amplitude):
    n = int(rate * seconds)
    return array.array("h", [int(amplitude * math.sin(2 * math.pi * freq_hz * i / rate)) for i in range(n)])


def rms(samples):
    return math.sqrt(sum(float(s) * s for s in samples) / max(1, len(samples)))


def test_box_downsample_matches_firmware_byte_math():
    # 5 samples at ratio 2: two full groups averaged + the trailing partial
    # group flushed as one sample (DownsampledPcmBytes ceil division).
    out, out_rate = email_downsample(array.array("h", [100, 200, 300, 400, 500]),
                                     16000, cap_seconds=8, mode="box")
    assert list(out) == [150, 350, 500]
    assert out_rate == 8000


def test_naive_downsample_reproduces_old_decimator():
    out, out_rate = email_downsample(array.array("h", [100, 200, 300, 400, 500]),
                                     16000, cap_seconds=8, mode="naive")
    assert list(out) == [100, 300, 500]
    assert out_rate == 8000


def test_output_rate_is_true_source_over_ratio():
    # A 22.05 kHz source downsamples by integer ratio 2 -> the file must say
    # 11025 Hz, not the nominal 8000, or playback pitch-shifts.
    out, out_rate = email_downsample(sine(440, 22050, 0.1, 8000), 22050, cap_seconds=8, mode="box")
    assert out_rate == 11025
    assert len(out) == math.ceil(int(22050 * 0.1) / 2)


def test_duration_cap_limits_output_like_firmware():
    long_input = array.array("h", [0] * (16000 * 20))  # 20 s of source audio
    out, _ = email_downsample(long_input, 16000, cap_seconds=EMAIL_QUESTION_CAP_S, mode="box")
    assert len(out) == 8000 * EMAIL_QUESTION_CAP_S  # capped, not ceil'd past the cap


def test_box_filter_attenuates_aliases_far_more_than_naive():
    # A 9 kHz tone in a 24 kHz source sits above the 4 kHz output Nyquist:
    # legal output content zero; whatever survives is pure aliasing. The old
    # naive decimator folds it down at full level, the box filter must not.
    tone = sine(9000, 24000, 0.25, 12000)
    naive, _ = email_downsample(tone, 24000, cap_seconds=8, mode="naive")
    box, _ = email_downsample(tone, 24000, cap_seconds=8, mode="box")
    assert rms(naive) > 6000          # the regression: alias at near-full level
    assert rms(box) < 0.4 * rms(naive)  # the fix: box-of-3 knocks it well down


def test_box_filter_preserves_passband_speech_levels():
    tone = sine(500, 16000, 0.25, 12000)
    box, out_rate = email_downsample(tone, 16000, cap_seconds=8, mode="box")
    assert out_rate == 8000
    assert abs(rms(box) - rms(tone)) / rms(tone) < 0.1  # 500 Hz sails through


def test_high_band_ratio_separates_bright_from_muffled():
    muffled = sine(300, 16000, 0.25, 12000)
    bright = array.array("h", [(-1) ** i * 8000 for i in range(4000)])  # ~Nyquist content
    assert high_band_ratio(muffled, 16000) < 0.02
    # One-pole HP gain tops out ~0.67 at Nyquist, so "bright" lands near 0.45.
    assert high_band_ratio(bright, 16000) > 0.3


def test_grade_fails_on_clipped_capture(tmp_path):
    clipped = array.array("h", ([32767] * 80 + [-32768] * 80) * 100)  # 100 Hz square at the rails
    path = tmp_path / "clipped.wav"
    write_mono16(path, clipped, 16000)
    info = analyze_wav(str(path))
    level, reasons = grade(info, high_band_ratio(clipped, 16000))
    assert level == "FAIL"
    assert any(reason.startswith("CLIPPING") for reason in reasons)


def test_grade_passes_clean_speechlike_capture(tmp_path):
    # Voiced fundamental + consonant-band overtone at sane levels.
    base = sine(220, 16000, 0.5, 9000)
    hiss = sine(3500, 16000, 0.5, 2500)
    clean = array.array("h", [base[i] + hiss[i] for i in range(len(base))])
    path = tmp_path / "clean.wav"
    write_mono16(path, clean, 16000)
    info = analyze_wav(str(path))
    level, _ = grade(info, high_band_ratio(clean, 16000))
    assert level == "PASS"


def test_unknown_mode_rejected():
    with pytest.raises(ValueError):
        email_downsample(array.array("h", [0, 0]), 16000, cap_seconds=8, mode="linear")
