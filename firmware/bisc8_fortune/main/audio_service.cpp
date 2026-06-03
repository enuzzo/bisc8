#include "audio_service.h"

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
constexpr int kBeepMillis = 90;
constexpr int kBeepFrequencyHz = 880;
constexpr int16_t kBeepAmplitude = 9000;
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

    PrepareBeep();
    available_ = beep_buffer_ != nullptr;
    DebugSerial::LogAlways("[AUDIO]", "ready record_bytes=%u beep_bytes=%u", static_cast<unsigned>(record_bytes_), static_cast<unsigned>(beep_bytes_));
    return available_ ? ESP_OK : ESP_ERR_NO_MEM;
}

void AudioService::PrepareBeep() {
    const int samples = kSampleRate * kBeepMillis / 1000;
    beep_bytes_ = samples * kChannels * kBytesPerSample;
    beep_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(beep_bytes_, MALLOC_CAP_DEFAULT));
    if (beep_buffer_ == nullptr) {
        return;
    }

    int16_t *pcm = reinterpret_cast<int16_t *>(beep_buffer_);
    const int half_period = kSampleRate / (kBeepFrequencyHz * 2);
    for (int i = 0; i < samples; i++) {
        const int16_t sample = ((i / half_period) % 2 == 0) ? kBeepAmplitude : -kBeepAmplitude;
        pcm[i * 2] = sample;
        pcm[i * 2 + 1] = sample;
    }
}

bool AudioService::Available() const {
    return available_;
}

void AudioService::PlayBeep() {
    if (!available_ || beep_buffer_ == nullptr) {
        DebugSerial::Log("[AUDIO]", "beep skipped; audio unavailable");
        return;
    }
    esp_err_t err = Codec_PlaybackData(beep_buffer_, beep_bytes_);
    DebugSerial::Log("[AUDIO]", "beep bytes=%u result=%s", static_cast<unsigned>(beep_bytes_), esp_err_to_name(err));
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
