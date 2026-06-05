#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "localization.h"

namespace bisc8 {

class DisplayService;

enum class AudioCue {
    Boot,
    OracleButton,
    VoiceSubmit,
    Shutdown,
};

class AudioService {
public:
    // Fired when audio playback or microphone capture starts (active=true) and
    // stops (active=false). The display layer rides the playback hook to animate
    // the "speaking" glyph; the recording hook is the clean seam for a future
    // "ti ascolto" listening animation. ctx is passed back verbatim.
    using AudioStateHook = void (*)(void *ctx, bool active);

    esp_err_t Initialize();
    bool Available() const;
    void PlayChime();
    void PlayCue(AudioCue cue);
    void PlayCueAsync(AudioCue cue);
    bool WaitForPlayback(uint32_t timeout_ms);
    void StopPlayback();
    void StartVoiceRecording();
    const char *FinishVoiceRecording();
    void RunMicTest(DisplayService &display, Language language);

    void SetPlaybackObserver(AudioStateHook hook, void *ctx);
    void SetRecordingObserver(AudioStateHook hook, void *ctx);

private:
    void NotifyPlayback(bool active);
    void NotifyRecording(bool active);

    struct QueuedSound;

    static void PlaybackTaskEntry(void *arg);
    static void VoiceRecordTaskEntry(void *arg);
    const QueuedSound *SoundFor(AudioCue cue) const;
    void PlaybackTask();
    void VoiceRecordTask();
    esp_err_t EnsureRecordBuffer();
    void ReleaseRecordBuffer();
    esp_err_t PrepareSpool();
    void PrepareChime();

    AudioStateHook playback_hook_ = nullptr;
    void *playback_hook_ctx_ = nullptr;
    AudioStateHook recording_hook_ = nullptr;
    void *recording_hook_ctx_ = nullptr;

    bool available_ = false;
    uint8_t *record_buffer_ = nullptr;
    size_t record_bytes_ = 0;
    uint8_t *feedback_buffer_ = nullptr;
    size_t feedback_bytes_ = 0;
    TaskHandle_t playback_task_ = nullptr;
    volatile bool playback_stop_requested_ = false;
    const QueuedSound *queued_sound_ = nullptr;
    TaskHandle_t voice_task_ = nullptr;
    const esp_partition_t *spool_partition_ = nullptr;
    volatile bool voice_recording_ = false;
    volatile bool voice_stop_requested_ = false;
    bool voice_file_ready_ = false;
    esp_err_t voice_err_ = ESP_OK;
    size_t voice_pcm_bytes_ = 0;
};

}  // namespace bisc8
