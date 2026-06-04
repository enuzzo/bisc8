#include "audio_service.h"

#include <math.h>
#include <string.h>

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "display_service.h"
#include "app_config.h"
#include "port_codec.h"

namespace bisc8 {

namespace {
constexpr int kSampleRate = 16000;
constexpr int kChannels = 2;
constexpr int kBytesPerSample = 2;
constexpr int kRecordMillis = 1000;
constexpr int kChimeMillis = 120;
constexpr float kChimeAttackRatio = 0.16f;
constexpr float kChimeAmplitude = 7800.0f;
constexpr float kPi = 3.14159265f;
}  // namespace

esp_err_t AudioService::Initialize() {
    DebugSerial::LogAlways("[AUDIO]", "initializing ES8311 codec");
    esp_err_t err = Codec_StartInit();
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[AUDIO]", "codec init failed: %s", esp_err_to_name(err));
        available_ = false;
        return err;
    }

    record_bytes_ = (kSampleRate * kRecordMillis / 1000) * kChannels * kBytesPerSample;
    record_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(record_bytes_, MALLOC_CAP_DEFAULT));
    if (record_buffer_ == nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "record buffer allocation failed: %u", static_cast<unsigned>(record_bytes_));
        available_ = false;
        return ESP_ERR_NO_MEM;
    }

    PrepareChime();
    available_ = feedback_buffer_ != nullptr;
    DebugSerial::LogAlways("[AUDIO]", "ready record_bytes=%u feedback_bytes=%u", static_cast<unsigned>(record_bytes_), static_cast<unsigned>(feedback_bytes_));
    return available_ ? ESP_OK : ESP_ERR_NO_MEM;
}

void AudioService::PrepareChime() {
    const int samples = kSampleRate * kChimeMillis / 1000;
    feedback_bytes_ = samples * kChannels * kBytesPerSample;
    feedback_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(feedback_bytes_, MALLOC_CAP_DEFAULT));
    if (feedback_buffer_ == nullptr) {
        return;
    }

    int16_t *pcm = reinterpret_cast<int16_t *>(feedback_buffer_);
    for (int i = 0; i < samples; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float progress = static_cast<float>(i) / static_cast<float>(samples);
        const float attack = progress < kChimeAttackRatio ? progress / kChimeAttackRatio : 1.0f;
        const float decay = 1.0f - progress;
        const float envelope = attack * decay * decay;
        const float fundamental = sinf(2.0f * kPi * 1174.66f * t);
        const float sparkle = sinf(2.0f * kPi * 1760.00f * t) * 0.42f;
        const int16_t sample = static_cast<int16_t>((fundamental + sparkle) * kChimeAmplitude * envelope);
        pcm[i * kChannels] = sample;
        pcm[i * kChannels + 1] = sample;
    }
}

bool AudioService::Available() const {
    return available_;
}

void AudioService::PlayBeep() {
    if (!available_ || feedback_buffer_ == nullptr) {
        DebugSerial::Log("[AUDIO]", "beep skipped; audio unavailable");
        return;
    }
    esp_err_t err = Codec_PlaybackData(feedback_buffer_, feedback_bytes_);
    DebugSerial::Log("[AUDIO]", "chime bytes=%u result=%s", static_cast<unsigned>(feedback_bytes_), esp_err_to_name(err));
}

void AudioService::StartVoiceRecording() {
    voice_recording_ = true;
    DebugSerial::LogAlways("[AUDIO]", "voice recording started limit_ms=%u mono_wav_spool=pending",
                           static_cast<unsigned>(kVoiceRecordLimitMs));
}

const char *AudioService::FinishVoiceRecording() {
    if (!voice_recording_) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording finish ignored; no active recording");
        return nullptr;
    }
    voice_recording_ = false;
    DebugSerial::LogAlways("[AUDIO]", "voice recording finished; wav spool path pending");
    return "/spool/question.wav";
}

void AudioService::RunMicTest(DisplayService &display) {
    if (!available_ || record_buffer_ == nullptr) {
        display.ShowAudioUnavailable();
        DebugSerial::LogAlways("[AUDIO]", "mic test skipped; audio unavailable");
        return;
    }

    memset(record_buffer_, 0, record_bytes_);
    display.ShowMicRecording();
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_err_t err = Codec_RecordData(record_buffer_, record_bytes_);
    DebugSerial::LogAlways("[AUDIO]", "record bytes=%u result=%s", static_cast<unsigned>(record_bytes_), esp_err_to_name(err));
    if (err != ESP_OK) {
        display.ShowError("Registrazione fallita.");
        return;
    }

    display.ShowMicPlayback();
    vTaskDelay(pdMS_TO_TICKS(250));
    err = Codec_PlaybackData(record_buffer_, record_bytes_);
    DebugSerial::LogAlways("[AUDIO]", "playback bytes=%u result=%s", static_cast<unsigned>(record_bytes_), esp_err_to_name(err));
    display.ShowMicDone();
}

}  // namespace bisc8
