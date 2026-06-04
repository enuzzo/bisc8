#pragma once

#include <stdint.h>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace bisc8 {

class CaptiveDnsService {
public:
    esp_err_t Start(uint32_t captive_ip);
    void Stop();
    bool Running() const;

private:
    static void TaskEntry(void *arg);
    void Run();
    int BuildResponse(const uint8_t *query, int query_len, uint8_t *response, int response_len) const;

    TaskHandle_t task_ = nullptr;
    int socket_ = -1;
    uint32_t captive_ip_ = 0;
    volatile bool running_ = false;
};

}  // namespace bisc8
