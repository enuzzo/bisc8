#include "audio_service.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "display_service.h"
#include "app_config.h"
#include "generated/sound_assets.h"
#include "port_codec.h"

namespace bisc8 {

namespace {
constexpr int kSampleRate = 16000;
constexpr int kChannels = 2;
constexpr int kVoiceChannels = 1;
constexpr int kBytesPerSample = 2;
constexpr int kRecordMillis = 1000;
constexpr int kChimeMillis = 120;
constexpr int kVoiceMaxChunks = kVoiceRecordLimitMs / kRecordMillis;
constexpr size_t kWavHeaderBytes = 44;
constexpr size_t kVoiceMaxPcmBytes = (kSampleRate * kVoiceRecordLimitMs / 1000) * kVoiceChannels * kBytesPerSample;
constexpr size_t kVoiceSpoolEraseBytes = 512 * 1024;
constexpr size_t kPlaybackChunkBytes = 4096;
constexpr uint32_t kVoiceTaskStackBytes = 4096;
constexpr uint32_t kPlaybackTaskStackBytes = 3072;
constexpr UBaseType_t kVoiceTaskPriority = 4;
constexpr UBaseType_t kPlaybackTaskPriority = 3;
constexpr float kChimeAttackRatio = 0.16f;
constexpr float kChimeAmplitude = 7800.0f;
constexpr float kPi = 3.14159265f;
constexpr const char *kSpoolPartition = "spool";
constexpr const char *kVoiceQuestionPath = "spool://question.wav";

void WriteLe16(uint8_t *dst, uint16_t value) {
    dst[0] = static_cast<uint8_t>(value & 0xff);
    dst[1] = static_cast<uint8_t>(value >> 8);
}

void WriteLe32(uint8_t *dst, uint32_t value) {
    dst[0] = static_cast<uint8_t>(value & 0xff);
    dst[1] = static_cast<uint8_t>((value >> 8) & 0xff);
    dst[2] = static_cast<uint8_t>((value >> 16) & 0xff);
    dst[3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

void BuildWavHeader(uint8_t *header, uint32_t pcm_bytes) {
    memcpy(header + 0, "RIFF", 4);
    WriteLe32(header + 4, 36 + pcm_bytes);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    WriteLe32(header + 16, 16);
    WriteLe16(header + 20, 1);
    WriteLe16(header + 22, kVoiceChannels);
    WriteLe32(header + 24, kSampleRate);
    WriteLe32(header + 28, kSampleRate * kVoiceChannels * kBytesPerSample);
    WriteLe16(header + 32, kVoiceChannels * kBytesPerSample);
    WriteLe16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    WriteLe32(header + 40, pcm_bytes);
}
}  // namespace

struct AudioService::QueuedSound {
    const SoundAsset *asset;
};

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

void AudioService::PlayChime() {
    StopPlayback();
    if (!available_ || feedback_buffer_ == nullptr) {
        DebugSerial::Log("[AUDIO]", "chime skipped; audio unavailable");
        return;
    }
    esp_err_t err = Codec_PlaybackData(feedback_buffer_, feedback_bytes_);
    DebugSerial::Log("[AUDIO]", "chime bytes=%u result=%s", static_cast<unsigned>(feedback_bytes_), esp_err_to_name(err));
}

const AudioService::QueuedSound *AudioService::SoundFor(AudioCue cue) const {
    static const QueuedSound boot = {&kSoundBoot};
    static const QueuedSound oracle_button = {&kSoundOracleButton};
    static const QueuedSound voice_submit = {&kSoundVoiceSubmit};
    static const QueuedSound shutdown = {&kSoundShutdown};

    switch (cue) {
        case AudioCue::Boot:
            return &boot;
        case AudioCue::OracleButton:
            return &oracle_button;
        case AudioCue::VoiceSubmit:
            return &voice_submit;
        case AudioCue::Shutdown:
            return &shutdown;
        default:
            return nullptr;
    }
}

void AudioService::PlayCue(AudioCue cue) {
    StopPlayback();
    const QueuedSound *sound = SoundFor(cue);
    if (!available_ || sound == nullptr || sound->asset == nullptr || sound->asset->data == nullptr || sound->asset->bytes == 0) {
        DebugSerial::Log("[AUDIO]", "cue skipped; audio unavailable");
        return;
    }

    esp_err_t err = Codec_PlaybackData(const_cast<uint8_t *>(sound->asset->data), sound->asset->bytes);
    DebugSerial::Log("[AUDIO]", "cue=%s bytes=%u result=%s",
                     sound->asset->name,
                     static_cast<unsigned>(sound->asset->bytes),
                     esp_err_to_name(err));
}

void AudioService::PlayCueAsync(AudioCue cue) {
    StopPlayback();
    const QueuedSound *sound = SoundFor(cue);
    if (!available_ || sound == nullptr || sound->asset == nullptr || sound->asset->data == nullptr || sound->asset->bytes == 0) {
        DebugSerial::Log("[AUDIO]", "async cue skipped; audio unavailable");
        return;
    }

    queued_sound_ = sound;
    playback_stop_requested_ = false;
    if (xTaskCreate(&AudioService::PlaybackTaskEntry, "bisc8_sound", kPlaybackTaskStackBytes, this, kPlaybackTaskPriority, &playback_task_) != pdPASS) {
        playback_task_ = nullptr;
        queued_sound_ = nullptr;
        DebugSerial::LogAlways("[AUDIO]", "async cue task create failed");
        return;
    }
    DebugSerial::Log("[AUDIO]", "async cue=%s bytes=%u",
                     sound->asset->name,
                     static_cast<unsigned>(sound->asset->bytes));
}

void AudioService::StopPlayback() {
    if (playback_task_ == nullptr) {
        return;
    }
    playback_stop_requested_ = true;
    const TickType_t start = xTaskGetTickCount();
    while (playback_task_ != nullptr && xTaskGetTickCount() - start < pdMS_TO_TICKS(600)) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (playback_task_ != nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "playback stop timed out");
    }
}

void AudioService::PlaybackTaskEntry(void *arg) {
    static_cast<AudioService *>(arg)->PlaybackTask();
    vTaskDelete(nullptr);
}

void AudioService::PlaybackTask() {
    const QueuedSound *sound = queued_sound_;
    if (sound == nullptr || sound->asset == nullptr) {
        playback_task_ = nullptr;
        return;
    }

    const SoundAsset *asset = sound->asset;
    size_t offset = 0;
    esp_err_t err = ESP_OK;
    while (!playback_stop_requested_ && offset < asset->bytes) {
        const size_t remaining = asset->bytes - offset;
        const size_t chunk_bytes = std::min(kPlaybackChunkBytes, remaining);
        err = Codec_PlaybackData(const_cast<uint8_t *>(asset->data + offset), chunk_bytes);
        if (err != ESP_OK) {
            break;
        }
        offset += chunk_bytes;
    }
    DebugSerial::Log("[AUDIO]", "async cue=%s played=%u/%u result=%s",
                     asset->name,
                     static_cast<unsigned>(offset),
                     static_cast<unsigned>(asset->bytes),
                     esp_err_to_name(err));
    queued_sound_ = nullptr;
    playback_stop_requested_ = false;
    playback_task_ = nullptr;
}

void AudioService::StartVoiceRecording() {
    StopPlayback();
    if (!available_ || record_buffer_ == nullptr) {
        voice_err_ = ESP_ERR_INVALID_STATE;
        DebugSerial::LogAlways("[AUDIO]", "voice recording skipped; audio unavailable");
        return;
    }
    if (voice_task_ != nullptr || voice_recording_) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording already active");
        return;
    }

    esp_err_t err = PrepareSpool();
    if (err != ESP_OK) {
        voice_err_ = err;
        DebugSerial::LogAlways("[AUDIO]", "voice spool prepare failed: %s", esp_err_to_name(err));
        return;
    }

    voice_stop_requested_ = false;
    voice_recording_ = true;
    voice_file_ready_ = false;
    voice_pcm_bytes_ = 0;
    voice_err_ = ESP_OK;
    if (xTaskCreate(&AudioService::VoiceRecordTaskEntry, "bisc8_voice", kVoiceTaskStackBytes, this, kVoiceTaskPriority, &voice_task_) != pdPASS) {
        voice_task_ = nullptr;
        voice_recording_ = false;
        voice_err_ = ESP_ERR_NO_MEM;
        DebugSerial::LogAlways("[AUDIO]", "voice recording task create failed");
        return;
    }

    DebugSerial::LogAlways("[AUDIO]", "voice recording started limit_ms=%u path=%s",
                           static_cast<unsigned>(kVoiceRecordLimitMs),
                           kVoiceQuestionPath);
}

const char *AudioService::FinishVoiceRecording() {
    if (!voice_recording_ && voice_task_ == nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording finish ignored; no active recording");
        return nullptr;
    }

    voice_stop_requested_ = true;
    const TickType_t start = xTaskGetTickCount();
    while (voice_task_ != nullptr && xTaskGetTickCount() - start < pdMS_TO_TICKS(1500)) {
        vTaskDelay(pdMS_TO_TICKS(25));
    }
    if (voice_task_ != nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording stop timed out");
        return nullptr;
    }
    if (voice_err_ != ESP_OK || !voice_file_ready_) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording failed: %s", esp_err_to_name(voice_err_));
        return nullptr;
    }

    DebugSerial::LogAlways("[AUDIO]", "voice recording finished path=%s pcm_bytes=%u",
                           kVoiceQuestionPath,
                           static_cast<unsigned>(voice_pcm_bytes_));
    return kVoiceQuestionPath;
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
        display.ShowError("Recording failed.");
        return;
    }

    display.ShowMicPlayback();
    vTaskDelay(pdMS_TO_TICKS(250));
    err = Codec_PlaybackData(record_buffer_, record_bytes_);
    DebugSerial::LogAlways("[AUDIO]", "playback bytes=%u result=%s", static_cast<unsigned>(record_bytes_), esp_err_to_name(err));
    display.ShowMicDone();
}

esp_err_t AudioService::PrepareSpool() {
    if (spool_partition_ == nullptr) {
        spool_partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);
    }
    if (spool_partition_ == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }
    if (spool_partition_->size < kWavHeaderBytes + kVoiceMaxPcmBytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t erase_bytes = std::min(kVoiceSpoolEraseBytes, static_cast<size_t>(spool_partition_->size));
    esp_err_t err = esp_partition_erase_range(spool_partition_, 0, erase_bytes);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t header[kWavHeaderBytes] = {};
    BuildWavHeader(header, 0);
    err = esp_partition_write(spool_partition_, 0, header, sizeof(header));
    if (err == ESP_OK) {
        DebugSerial::LogAlways("[AUDIO]", "voice raw spool prepared partition=%s erase_bytes=%u",
                               kSpoolPartition,
                               static_cast<unsigned>(erase_bytes));
    }
    return err;
}

void AudioService::VoiceRecordTaskEntry(void *arg) {
    static_cast<AudioService *>(arg)->VoiceRecordTask();
    vTaskDelete(nullptr);
}

void AudioService::VoiceRecordTask() {
    int chunks = 0;
    while (!voice_stop_requested_ && chunks < kVoiceMaxChunks) {
        memset(record_buffer_, 0, record_bytes_);
        esp_err_t err = Codec_RecordData(record_buffer_, record_bytes_);
        if (err != ESP_OK) {
            voice_err_ = err;
            break;
        }

        int16_t *samples = reinterpret_cast<int16_t *>(record_buffer_);
        const size_t frames = record_bytes_ / (kChannels * kBytesPerSample);
        for (size_t i = 0; i < frames; ++i) {
            const int32_t left = samples[i * kChannels];
            const int32_t right = samples[i * kChannels + 1];
            samples[i] = static_cast<int16_t>((left + right) / 2);
        }

        const size_t mono_bytes = frames * kVoiceChannels * kBytesPerSample;
        if (voice_pcm_bytes_ + mono_bytes > kVoiceMaxPcmBytes) {
            voice_err_ = ESP_ERR_INVALID_SIZE;
            break;
        }
        err = esp_partition_write(spool_partition_, kWavHeaderBytes + voice_pcm_bytes_, record_buffer_, mono_bytes);
        if (err != ESP_OK) {
            voice_err_ = err;
            break;
        }
        voice_pcm_bytes_ += mono_bytes;
        ++chunks;
    }

    if (voice_err_ == ESP_OK) {
        uint8_t header[kWavHeaderBytes] = {};
        BuildWavHeader(header, static_cast<uint32_t>(voice_pcm_bytes_));
        voice_err_ = esp_partition_write(spool_partition_, 0, header, sizeof(header));
        voice_file_ready_ = voice_pcm_bytes_ > 0;
    }
    voice_recording_ = false;
    voice_task_ = nullptr;
}

}  // namespace bisc8
