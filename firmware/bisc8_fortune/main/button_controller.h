#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "button.h"

namespace bisc8 {

class ButtonController {
public:
    void Initialize(QueueHandle_t queue);

private:
    Button *boot_button_ = nullptr;
    Button *power_button_ = nullptr;
};

}  // namespace bisc8
