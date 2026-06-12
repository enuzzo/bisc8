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
    VoiceStart,
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
    // Pre-erase the question spool during idle so the next recording starts
    // instantly. Call at boot and after each completed query.
    void RearmQuestionSpool();
    // Copy the finished question WAV to the archive slot (kQuestionArchiveSpoolOffset)
    // so the portal can serve the last mic capture even after the rearm wipes
    // the live region. Call right before RearmQuestionSpool().
    void ArchiveQuestionRecording();
    // Play the generated oracle answer WAV sitting in the spool answer region
    // (parses its header, reopens the codec at its sample rate, mono->stereo as
    // needed, restores 16 kHz after). Drives the playback observer so the
    // speaking glyph animates. Blocks until done. `wav_total_bytes` is the byte
    // count actually written to the spool. The oracle may synthesize the WAV
    // header locally around streamed PCM, so playback trusts this count.
    esp_err_t PlayAnswerAudio(uint32_t wav_total_bytes);
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
    esp_err_t EnsureRecordBuffer(size_t bytes);
    void ReleaseRecordBuffer();
    esp_err_t PrepareSpool();
    void PrepareChime();

    AudioStateHook playback_hook_ = nullptr;
    void *playback_hook_ctx_ = nullptr;
    AudioStateHook recording_hook_ = nullptr;
    void *recording_hook_ctx_ = nullptr;

    bool available_ = false;
    uint8_t *record_buffer_ = nullptr;
    size_t record_bytes_ = 0;       // mic-test loopback buffer size
    size_t record_buf_size_ = 0;    // size record_buffer_ is currently allocated at
    bool question_spool_clean_ = false;  // true when the question spool is pre-erased
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
