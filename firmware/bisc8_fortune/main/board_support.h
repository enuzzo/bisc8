#pragma once

#include <stdint.h>

#include <esp_err.h>

#include "port_i2c.h"

namespace bisc8 {

class BoardSupport {
public:
    esp_err_t Initialize();
    void PowerDownForSleep();
    void EnterDeepSleep(const char *reason, uint64_t wake_pin_mask);
    I2cMasterBus *i2c_bus() const;

private:
    I2cMasterBus *i2c_bus_ = nullptr;
};

}  // namespace bisc8
