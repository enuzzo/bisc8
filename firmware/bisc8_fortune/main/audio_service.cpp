#include "audio_service.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
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
constexpr int kRecordMillis = 1000;       // mic-test loopback chunk
constexpr int kChimeMillis = 120;
// Voice recording streams to flash in small chunks. A small working buffer
// allocates reliably even with a fragmented heap (a 64 KB contiguous block
// often does not at ~85 KB free), and a short chunk makes the BOOT-release stop
// prompt. This is decoupled from the larger mic-test buffer above.
constexpr int kVoiceChunkMillis = 250;
constexpr size_t kVoiceChunkBytes =
    static_cast<size_t>(kSampleRate) * kVoiceChunkMillis / 1000 * kChannels * kBytesPerSample;
constexpr int kVoiceMaxChunks = kVoiceRecordLimitMs / kVoiceChunkMillis;
constexpr size_t kWavHeaderBytes = 44;
constexpr size_t kVoiceMaxPcmBytes = (kSampleRate * kVoiceRecordLimitMs / 1000) * kVoiceChannels * kBytesPerSample;
constexpr size_t kVoiceSpoolEraseBytes = 512 * 1024;
constexpr size_t kPlaybackChunkBytes = 4096;
constexpr uint32_t kVoiceTaskStackBytes = 4096;
constexpr uint32_t kPlaybackTaskStackBytes = 3072;
constexpr uint32_t kCuePlaybackWaitMs = 20000;
constexpr UBaseType_t kVoiceTaskPriority = 4;
constexpr UBaseType_t kPlaybackTaskPriority = 6;
constexpr int kAnswerGainPercent = 180;  // lift the (quiet) TTS without over-clipping it
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
    PrepareChime();
    available_ = feedback_buffer_ != nullptr;
    DebugSerial::LogAlways("[AUDIO]",
                           "ready record_bytes=%u feedback_bytes=%u record_buffer=lazy",
                           static_cast<unsigned>(record_bytes_),
                           static_cast<unsigned>(feedback_bytes_));
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

void AudioService::SetPlaybackObserver(AudioStateHook hook, void *ctx) {
    playback_hook_ = hook;
    playback_hook_ctx_ = ctx;
}

void AudioService::SetRecordingObserver(AudioStateHook hook, void *ctx) {
    recording_hook_ = hook;
    recording_hook_ctx_ = ctx;
}

void AudioService::NotifyPlayback(bool active) {
    if (playback_hook_ != nullptr) {
        playback_hook_(playback_hook_ctx_, active);
    }
}

void AudioService::NotifyRecording(bool active) {
    if (recording_hook_ != nullptr) {
        recording_hook_(recording_hook_ctx_, active);
    }
}

void AudioService::PlayChime() {
    StopPlayback();
    if (!available_ || feedback_buffer_ == nullptr) {
        DebugSerial::Log("[AUDIO]", "chime skipped; audio unavailable");
        return;
    }
    NotifyPlayback(true);
    esp_err_t err = Codec_PlaybackData(feedback_buffer_, feedback_bytes_);
    NotifyPlayback(false);
    DebugSerial::Log("[AUDIO]", "chime bytes=%u result=%s", static_cast<unsigned>(feedback_bytes_), esp_err_to_name(err));
}

const AudioService::QueuedSound *AudioService::SoundFor(AudioCue cue) const {
    static const QueuedSound boot = {&kSoundBoot};
    static const QueuedSound oracle_button = {&kSoundOracleButton};
    static const QueuedSound voice_start = {&kSoundVoiceStart};
    static const QueuedSound voice_submit = {&kSoundVoiceSubmit};
    static const QueuedSound shutdown = {&kSoundShutdown};

    switch (cue) {
        case AudioCue::Boot:
            return &boot;
        case AudioCue::OracleButton:
            return &oracle_button;
        case AudioCue::VoiceStart:
            return &voice_start;
        case AudioCue::VoiceSubmit:
            return &voice_submit;
        case AudioCue::Shutdown:
            return &shutdown;
        default:
            return nullptr;
    }
}

void AudioService::PlayCue(AudioCue cue) {
    PlayCueAsync(cue);
    WaitForPlayback(kCuePlaybackWaitMs);
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
    NotifyPlayback(true);
    DebugSerial::Log("[AUDIO]", "async cue=%s bytes=%u",
                     sound->asset->name,
                     static_cast<unsigned>(sound->asset->bytes));
}

bool AudioService::WaitForPlayback(uint32_t timeout_ms) {
    if (playback_task_ == nullptr) {
        return true;
    }
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(timeout_ms);
    while (playback_task_ != nullptr && xTaskGetTickCount() - start < timeout) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (playback_task_ != nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "playback wait timed out");
        return false;
    }
    return true;
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

esp_err_t AudioService::EnsureRecordBuffer(size_t bytes) {
    if (bytes == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (record_buffer_ != nullptr && record_buf_size_ == bytes) {
        return ESP_OK;
    }
    if (record_buffer_ != nullptr) {
        heap_caps_free(record_buffer_);
        record_buffer_ = nullptr;
        record_buf_size_ = 0;
    }
    record_buffer_ = static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_DEFAULT));
    if (record_buffer_ == nullptr) {
        DebugSerial::LogAlways("[AUDIO]",
                               "record buffer allocation failed: bytes=%u free_heap=%u largest_block=%u",
                               static_cast<unsigned>(bytes),
                               static_cast<unsigned>(esp_get_free_heap_size()),
                               static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        return ESP_ERR_NO_MEM;
    }
    record_buf_size_ = bytes;
    DebugSerial::LogAlways("[AUDIO]",
                           "record buffer allocated bytes=%u free_heap=%u",
                           static_cast<unsigned>(bytes),
                           static_cast<unsigned>(esp_get_free_heap_size()));
    return ESP_OK;
}

void AudioService::ReleaseRecordBuffer() {
    if (record_buffer_ == nullptr) {
        return;
    }
    heap_caps_free(record_buffer_);
    record_buffer_ = nullptr;
    record_buf_size_ = 0;
    DebugSerial::LogAlways("[AUDIO]",
                           "record buffer released free_heap=%u",
                           static_cast<unsigned>(esp_get_free_heap_size()));
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
    NotifyPlayback(false);
}

void AudioService::StartVoiceRecording() {
    StopPlayback();
    if (!available_) {
        voice_err_ = ESP_ERR_INVALID_STATE;
        DebugSerial::LogAlways("[AUDIO]", "voice recording skipped; audio unavailable");
        return;
    }
    if (voice_task_ != nullptr || voice_recording_) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording already active");
        return;
    }

    esp_err_t err = EnsureRecordBuffer(kVoiceChunkBytes);
    if (err != ESP_OK) {
        voice_err_ = err;
        DebugSerial::LogAlways("[AUDIO]", "voice recording skipped; record buffer unavailable");
        return;
    }

    err = PrepareSpool();
    if (err != ESP_OK) {
        voice_err_ = err;
        ReleaseRecordBuffer();
        DebugSerial::LogAlways("[AUDIO]", "voice spool prepare failed: %s", esp_err_to_name(err));
        return;
    }

    // "Start talking" cue, played to completion BEFORE capture begins so it is a
    // clear go signal and is not itself recorded.
    PlayCue(AudioCue::VoiceStart);

    voice_stop_requested_ = false;
    voice_recording_ = true;
    voice_file_ready_ = false;
    voice_pcm_bytes_ = 0;
    voice_err_ = ESP_OK;
    if (xTaskCreate(&AudioService::VoiceRecordTaskEntry, "bisc8_voice", kVoiceTaskStackBytes, this, kVoiceTaskPriority, &voice_task_) != pdPASS) {
        voice_task_ = nullptr;
        voice_recording_ = false;
        voice_err_ = ESP_ERR_NO_MEM;
        ReleaseRecordBuffer();
        DebugSerial::LogAlways("[AUDIO]", "voice recording task create failed");
        return;
    }

    NotifyRecording(true);
    DebugSerial::LogAlways("[AUDIO]", "voice recording started limit_ms=%u path=%s",
                           static_cast<unsigned>(kVoiceRecordLimitMs),
                           kVoiceQuestionPath);
}

const char *AudioService::FinishVoiceRecording() {
    if (!voice_recording_ && voice_task_ == nullptr) {
        if (voice_err_ == ESP_OK && voice_file_ready_) {
            NotifyRecording(false);
            DebugSerial::LogAlways("[AUDIO]", "voice recording auto-finished path=%s pcm_bytes=%u",
                                   kVoiceQuestionPath,
                                   static_cast<unsigned>(voice_pcm_bytes_));
            return kVoiceQuestionPath;
        }
        DebugSerial::LogAlways("[AUDIO]", "voice recording finish ignored; no active recording");
        return nullptr;
    }

    voice_stop_requested_ = true;
    const TickType_t start = xTaskGetTickCount();
    while (voice_task_ != nullptr && xTaskGetTickCount() - start < pdMS_TO_TICKS(2500)) {
        vTaskDelay(pdMS_TO_TICKS(25));
    }
    if (voice_task_ != nullptr) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording stop timed out");
        return nullptr;
    }
    NotifyRecording(false);
    if (voice_err_ != ESP_OK || !voice_file_ready_) {
        DebugSerial::LogAlways("[AUDIO]", "voice recording failed: %s", esp_err_to_name(voice_err_));
        return nullptr;
    }

    DebugSerial::LogAlways("[AUDIO]", "voice recording finished path=%s pcm_bytes=%u",
                           kVoiceQuestionPath,
                           static_cast<unsigned>(voice_pcm_bytes_));
    return kVoiceQuestionPath;
}

esp_err_t AudioService::PlayAnswerAudio(uint32_t wav_total_bytes) {
    if (!available_) {
        DebugSerial::LogAlways("[AUDIO]", "answer playback skipped; audio unavailable");
        return ESP_ERR_INVALID_STATE;
    }
    StopPlayback();
    const esp_partition_t *spool =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);
    if (spool == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t hdr[128] = {};
    esp_err_t err = esp_partition_read(spool, kVoiceAnswerSpoolOffset, hdr, sizeof(hdr));
    if (err != ESP_OK) {
        return err;
    }
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        DebugSerial::LogAlways("[AUDIO]", "answer audio is not a WAV");
        return ESP_ERR_INVALID_STATE;
    }
    auto le16 = [](const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); };
    auto le32 = [](const uint8_t *p) {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    };
    uint16_t channels = 1;
    uint16_t bits = 16;
    uint32_t rate = 24000;
    uint32_t data_off = 0;
    uint32_t data_size = 0;
    size_t p = 12;
    while (p + 8 <= sizeof(hdr)) {
        const uint32_t csize = le32(hdr + p + 4);
        if (memcmp(hdr + p, "fmt ", 4) == 0) {
            channels = le16(hdr + p + 8 + 2);
            rate = le32(hdr + p + 8 + 4);
            bits = le16(hdr + p + 8 + 14);
        } else if (memcmp(hdr + p, "data", 4) == 0) {
            data_off = static_cast<uint32_t>(p + 8);
            data_size = csize;
            break;
        }
        p += 8 + csize + (csize & 1);
    }
    // OpenAI streams the answer WAV with an "unknown length" placeholder in the
    // data-chunk size (seen as 0xFFFFFFFF), so it cannot be trusted. Use the
    // bytes actually written to the spool, and guard every bound against
    // uint32 overflow (data_off + 0xFFFFFFFF would wrap and defeat the clamp).
    const uint32_t avail = (wav_total_bytes > data_off) ? (wav_total_bytes - data_off) : 0;
    if (data_size == 0 || data_size == 0xFFFFFFFFu || data_size > avail) {
        data_size = avail;
    }
    if (data_off < kVoiceAnswerSpoolMaxBytes &&
        data_size > kVoiceAnswerSpoolMaxBytes - data_off) {
        data_size = kVoiceAnswerSpoolMaxBytes - data_off;
    }
    if (data_off == 0 || data_size == 0 || bits != 16 || channels != 1) {
        DebugSerial::LogAlways("[AUDIO]", "answer WAV unsupported ch=%u bits=%u data=%u total=%u",
                               channels, bits, static_cast<unsigned>(data_size),
                               static_cast<unsigned>(wav_total_bytes));
        return ESP_ERR_NOT_SUPPORTED;
    }
    DebugSerial::LogAlways("[AUDIO]", "answer WAV rate=%u ch=%u bytes=%u",
                           static_cast<unsigned>(rate), channels, static_cast<unsigned>(data_size));

    // The codec runs at kSampleRate (16 kHz). OpenAI returns 24 kHz audio, and
    // reopening the codec to 24 kHz does NOT actually retune the TDM I2S clock,
    // so it plays 1.5x slow and an octave low. Resample 24->16 kHz here instead
    // (exact 3:2 ratio, integer math: 3 input samples -> 2 output) and leave the
    // codec at its native rate. The answer is mono; we duplicate to stereo.
    const bool resample = (rate == 24000 && kSampleRate == 16000);
    if (!resample && rate != static_cast<uint32_t>(kSampleRate)) {
        DebugSerial::LogAlways("[AUDIO]", "answer rate=%u unhandled; playing as-is",
                               static_cast<unsigned>(rate));
    }

    constexpr size_t kInBytes = 6144;  // 3072 mono samples; multiple of 6 for clean 3:2 groups
    int16_t *inbuf = static_cast<int16_t *>(heap_caps_malloc(kInBytes, MALLOC_CAP_DEFAULT));
    int16_t *outbuf = static_cast<int16_t *>(heap_caps_malloc(kInBytes * 2, MALLOC_CAP_DEFAULT));
    if (inbuf == nullptr || outbuf == nullptr) {
        heap_caps_free(inbuf);
        heap_caps_free(outbuf);
        return ESP_ERR_NO_MEM;
    }
    auto clip16 = [](int32_t v) -> int16_t {
        return v > 32767 ? 32767 : (v < -32768 ? -32768 : static_cast<int16_t>(v));
    };

    // The answer plays from the (low-priority) main task; raise its priority for
    // the duration so Wi-Fi and friends can't starve the codec feed (stutter).
    const UBaseType_t prev_prio = uxTaskPriorityGet(nullptr);
    vTaskPrioritySet(nullptr, kPlaybackTaskPriority);

    NotifyPlayback(true);
    uint32_t off = kVoiceAnswerSpoolOffset + data_off;
    uint32_t remaining = data_size & ~static_cast<uint32_t>(1);  // whole samples only
    err = ESP_OK;
    while (remaining >= 2) {
        size_t want = remaining < kInBytes ? remaining : kInBytes;
        want -= want % (resample ? 6 : 2);
        if (want == 0) {
            break;  // trailing 1-2 samples, inaudible
        }
        if (esp_partition_read(spool, off, inbuf, want) != ESP_OK) {
            err = ESP_FAIL;
            break;
        }
        const size_t in_samples = want / 2;
        size_t out_i = 0;
        if (resample) {
            for (size_t g = 0; g + 3 <= in_samples; g += 3) {
                const int16_t o0 = clip16(static_cast<int32_t>(inbuf[g]) * kAnswerGainPercent / 100);
                const int32_t mix = (static_cast<int32_t>(inbuf[g + 1]) + inbuf[g + 2]) / 2;
                const int16_t o1 = clip16(mix * kAnswerGainPercent / 100);
                outbuf[out_i++] = o0; outbuf[out_i++] = o0;
                outbuf[out_i++] = o1; outbuf[out_i++] = o1;
            }
        } else {
            for (size_t s = 0; s < in_samples; ++s) {
                const int16_t o = clip16(static_cast<int32_t>(inbuf[s]) * kAnswerGainPercent / 100);
                outbuf[out_i++] = o; outbuf[out_i++] = o;
            }
        }
        err = Codec_PlaybackData(reinterpret_cast<uint8_t *>(outbuf), out_i * sizeof(int16_t));
        if (err != ESP_OK) {
            break;
        }
        off += want;
        remaining -= want;
    }
    NotifyPlayback(false);
    vTaskPrioritySet(nullptr, prev_prio);
    heap_caps_free(inbuf);
    heap_caps_free(outbuf);
    DebugSerial::LogAlways("[AUDIO]", "answer playback done rate=%u resample=%d result=%s",
                           static_cast<unsigned>(rate), resample ? 1 : 0, esp_err_to_name(err));
    return err;
}

void AudioService::RunMicTest(DisplayService &display, Language language) {
    if (!available_) {
        display.ShowAudioUnavailable(language);
        DebugSerial::LogAlways("[AUDIO]", "mic test skipped; audio unavailable");
        return;
    }
    esp_err_t err = EnsureRecordBuffer(record_bytes_);
    if (err != ESP_OK) {
        display.ShowAudioUnavailable(language);
        DebugSerial::LogAlways("[AUDIO]", "mic test skipped; record buffer unavailable");
        return;
    }

    memset(record_buffer_, 0, record_bytes_);
    display.ShowMicRecording(language);
    vTaskDelay(pdMS_TO_TICKS(250));
    err = Codec_RecordData(record_buffer_, record_bytes_);
    DebugSerial::LogAlways("[AUDIO]", "record bytes=%u result=%s", static_cast<unsigned>(record_bytes_), esp_err_to_name(err));
    if (err != ESP_OK) {
        display.ShowError(StringsFor(language).recording_failed_body, "E01", language);
        ReleaseRecordBuffer();
        return;
    }

    display.ShowMicPlayback(language);
    vTaskDelay(pdMS_TO_TICKS(250));
    err = Codec_PlaybackData(record_buffer_, record_bytes_);
    DebugSerial::LogAlways("[AUDIO]", "playback bytes=%u result=%s", static_cast<unsigned>(record_bytes_), esp_err_to_name(err));
    ReleaseRecordBuffer();
    display.ShowMicDone(language);
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

    // The 512 KB erase costs ~1-2s. RearmQuestionSpool() pre-erases it during
    // idle (at boot and after each query), so a real recording starts instantly.
    if (question_spool_clean_) {
        question_spool_clean_ = false;  // this recording is about to dirty it
        DebugSerial::LogAlways("[AUDIO]", "voice spool pre-erased; instant start");
        return ESP_OK;
    }

    const size_t erase_bytes = std::min(kVoiceSpoolEraseBytes, static_cast<size_t>(spool_partition_->size));
    esp_err_t err = esp_partition_erase_range(spool_partition_, 0, erase_bytes);
    if (err != ESP_OK) {
        return err;
    }

    // Do NOT write a placeholder header here. The final WAV header is written
    // once, after recording, over still-erased flash (0xFF -> any value works).
    // A placeholder with size 0 could not later be raised to the real size,
    // since NOR flash can only clear bits without an erase, which left readers
    // (the speech-to-text upload) seeing a 0-length data chunk.
    DebugSerial::LogAlways("[AUDIO]", "voice raw spool prepared partition=%s erase_bytes=%u",
                           kSpoolPartition,
                           static_cast<unsigned>(erase_bytes));
    return ESP_OK;
}

void AudioService::RearmQuestionSpool() {
    // Pre-erase the question region during idle (boot, and after each query once
    // the WAV is no longer needed) so the next recording skips the slow erase.
    if (voice_recording_ || voice_task_ != nullptr) {
        return;  // never erase under an active recording
    }
    question_spool_clean_ = false;  // force PrepareSpool down the erase path
    if (PrepareSpool() == ESP_OK) {
        question_spool_clean_ = true;
    }
}

void AudioService::VoiceRecordTaskEntry(void *arg) {
    static_cast<AudioService *>(arg)->VoiceRecordTask();
    vTaskDelete(nullptr);
}

void AudioService::VoiceRecordTask() {
    int chunks = 0;
    // Capture diagnostics: one [VOICEDIAG] line per recording so an on-device
    // test can tell WHY the question audio is bad instead of us guessing.
    //   peakL/peakR/peakMono : ~32767 => clipping (gain too hot); tiny => too weak.
    //   rms_dBFS             : healthy speech ~ -20..-12; < -40 => far/too quiet.
    //   clip%                : fraction of near-full-scale samples (distortion).
    //   maxRead/maxWrite     : per-chunk codec-read / flash-write stalls (ms).
    //   wall vs dur          : wall >> dur => the loop stalled (dropouts); wall
    //                          far below dur => real capture rate != 16 kHz.
    // The hot loop stays integer-only; the dBFS/log math runs once at the end.
    int32_t peak_l = 0, peak_r = 0, peak_mono = 0;
    uint64_t sumsq_mono = 0;
    uint32_t clip_count = 0;
    uint32_t stat_frames = 0;
    int64_t max_read_us = 0, max_write_us = 0;
    const int64_t t_start = esp_timer_get_time();
    while (!voice_stop_requested_ && chunks < kVoiceMaxChunks) {
        memset(record_buffer_, 0, kVoiceChunkBytes);
        const int64_t r0 = esp_timer_get_time();
        esp_err_t err = Codec_RecordData(record_buffer_, kVoiceChunkBytes);
        const int64_t read_us = esp_timer_get_time() - r0;
        if (read_us > max_read_us) max_read_us = read_us;
        if (err != ESP_OK) {
            voice_err_ = err;
            break;
        }

        int16_t *samples = reinterpret_cast<int16_t *>(record_buffer_);
        const size_t frames = kVoiceChunkBytes / (kChannels * kBytesPerSample);
        for (size_t i = 0; i < frames; ++i) {
            const int32_t left = samples[i * kChannels];
            const int32_t right = samples[i * kChannels + 1];
            const int32_t mono = (left + right) / 2;
            samples[i] = static_cast<int16_t>(mono);
            const int32_t al = left < 0 ? -left : left;
            const int32_t ar = right < 0 ? -right : right;
            const int32_t am = mono < 0 ? -mono : mono;
            if (al > peak_l) peak_l = al;
            if (ar > peak_r) peak_r = ar;
            if (am > peak_mono) peak_mono = am;
            sumsq_mono += static_cast<uint64_t>(mono * mono);
            if (am >= 32000) ++clip_count;  // within ~0.2 dB of full scale
        }
        stat_frames += static_cast<uint32_t>(frames);

        const size_t mono_bytes = frames * kVoiceChannels * kBytesPerSample;
        if (voice_pcm_bytes_ + mono_bytes > kVoiceMaxPcmBytes) {
            voice_err_ = ESP_ERR_INVALID_SIZE;
            break;
        }
        const int64_t w0 = esp_timer_get_time();
        err = esp_partition_write(spool_partition_, kWavHeaderBytes + voice_pcm_bytes_, record_buffer_, mono_bytes);
        const int64_t write_us = esp_timer_get_time() - w0;
        if (write_us > max_write_us) max_write_us = write_us;
        if (err != ESP_OK) {
            voice_err_ = err;
            break;
        }
        voice_pcm_bytes_ += mono_bytes;
        ++chunks;
    }
    const uint32_t wall_ms = static_cast<uint32_t>((esp_timer_get_time() - t_start) / 1000);

    if (voice_err_ == ESP_OK) {
        uint8_t header[kWavHeaderBytes] = {};
        BuildWavHeader(header, static_cast<uint32_t>(voice_pcm_bytes_));
        voice_err_ = esp_partition_write(spool_partition_, 0, header, sizeof(header));
        voice_file_ready_ = voice_pcm_bytes_ > 0;
    }

    if (stat_frames > 0) {
        const double rms = sqrt(static_cast<double>(sumsq_mono) / stat_frames);
        const double dbfs = rms > 1.0 ? 20.0 * log10(rms / 32768.0) : -120.0;
        const double clip_pct = 100.0 * clip_count / stat_frames;
        DebugSerial::LogAlways("[VOICEDIAG]",
            "frames=%u dur=%ums wall=%ums peakL=%d peakR=%d peakMono=%d rms_dBFS=%.1f clip=%.2f%% maxRead=%ums maxWrite=%ums",
            static_cast<unsigned>(stat_frames),
            static_cast<unsigned>(stat_frames * 1000u / static_cast<unsigned>(kSampleRate)),
            static_cast<unsigned>(wall_ms),
            static_cast<int>(peak_l), static_cast<int>(peak_r), static_cast<int>(peak_mono),
            dbfs, clip_pct,
            static_cast<unsigned>(max_read_us / 1000), static_cast<unsigned>(max_write_us / 1000));
    }

    ReleaseRecordBuffer();
    voice_recording_ = false;
    voice_task_ = nullptr;
}

}  // namespace bisc8
