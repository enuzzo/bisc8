#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>

namespace bisc8 {

class DisplayService;

class AudioService {
public:
    esp_err_t Initialize();
    bool Available() const;
    void PlayChime();
    void StartVoiceRecording();
    const char *FinishVoiceRecording();
    void RunMicTest(DisplayService &display);

private:
    void PrepareChime();

    bool available_ = false;
    uint8_t *record_buffer_ = nullptr;
    size_t record_bytes_ = 0;
    uint8_t *feedback_buffer_ = nullptr;
    size_t feedback_bytes_ = 0;
    bool voice_recording_ = false;
};

}  // namespace bisc8
