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
    void PlayBeep();
    void RunMicTest(DisplayService &display);

private:
    void PrepareBeep();

    bool available_ = false;
    uint8_t *record_buffer_ = nullptr;
    size_t record_bytes_ = 0;
    uint8_t *beep_buffer_ = nullptr;
    size_t beep_bytes_ = 0;
};

}  // namespace bisc8
