#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
    static void VoiceRecordTaskEntry(void *arg);
    void VoiceRecordTask();
    esp_err_t PrepareSpool();
    void PrepareChime();

    bool available_ = false;
    uint8_t *record_buffer_ = nullptr;
    size_t record_bytes_ = 0;
    uint8_t *feedback_buffer_ = nullptr;
    size_t feedback_bytes_ = 0;
    TaskHandle_t voice_task_ = nullptr;
    const esp_partition_t *spool_partition_ = nullptr;
    volatile bool voice_recording_ = false;
    volatile bool voice_stop_requested_ = false;
    bool voice_file_ready_ = false;
    esp_err_t voice_err_ = ESP_OK;
    size_t voice_pcm_bytes_ = 0;
};

}  // namespace bisc8
