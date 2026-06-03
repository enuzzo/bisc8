#include "board_support.h"

#include <stdio.h>

#include <driver/gpio.h>
#include <esp_bit_defs.h>
#include <esp_rom_sys.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "epaper_config.h"
#include "esp_io_expander_gpio_wrapper.h"
#include "esp_io_expander_tca9554.h"
#include "port_power.h"

namespace bisc8 {

namespace {
esp_io_expander_handle_t g_io_expander = nullptr;

void LogI2cLevels(const char *phase) {
    DebugSerial::LogAlways("[BOOT]",
                           "I2C levels %s scl=%d sda=%d",
                           phase,
                           gpio_get_level(ESP32_I2C_SCL_PIN),
                           gpio_get_level(ESP32_I2C_SDA_PIN));
}

void RecoverI2cBus() {
    DebugSerial::LogAlways("[BOOT]", "recovering I2C bus before device init");

#if !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
    gpio_deep_sleep_hold_dis();
#endif
    gpio_hold_dis(ESP32_I2C_SCL_PIN);
    gpio_hold_dis(ESP32_I2C_SDA_PIN);

    gpio_reset_pin(ESP32_I2C_SCL_PIN);
    gpio_reset_pin(ESP32_I2C_SDA_PIN);
    gpio_set_direction(ESP32_I2C_SCL_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(ESP32_I2C_SDA_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(ESP32_I2C_SCL_PIN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ESP32_I2C_SDA_PIN, GPIO_PULLUP_ONLY);

    gpio_set_level(ESP32_I2C_SDA_PIN, 1);
    gpio_set_level(ESP32_I2C_SCL_PIN, 1);
    esp_rom_delay_us(5000);
    LogI2cLevels("released");

    for (int i = 0; i < 32; ++i) {
        gpio_set_level(ESP32_I2C_SCL_PIN, 0);
        esp_rom_delay_us(100);
        gpio_set_level(ESP32_I2C_SCL_PIN, 1);
        esp_rom_delay_us(100);
        if (gpio_get_level(ESP32_I2C_SDA_PIN) == 1) {
            break;
        }
    }
    LogI2cLevels("after-clock");

    gpio_set_level(ESP32_I2C_SDA_PIN, 0);
    esp_rom_delay_us(100);
    gpio_set_level(ESP32_I2C_SCL_PIN, 1);
    esp_rom_delay_us(100);
    gpio_set_level(ESP32_I2C_SDA_PIN, 1);
    esp_rom_delay_us(100);
    LogI2cLevels("after-stop");

    gpio_reset_pin(ESP32_I2C_SCL_PIN);
    gpio_reset_pin(ESP32_I2C_SDA_PIN);
}

esp_err_t ConfigureWakePin(gpio_num_t pin) {
    const gpio_config_t wake_config = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&wake_config);
}
}

esp_err_t BoardSupport::Initialize() {
    DebugSerial::LogAlways("[BOOT]", "initializing I2C and IO expander");
    RecoverI2cBus();
    i2c_bus_ = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
    if (i2c_bus_ == nullptr) {
        DebugSerial::LogAlways("[BOOT]", "I2C bus init failed");
        return ESP_FAIL;
    }

    esp_err_t err = esp_io_expander_new_i2c_tca9554(
        i2c_bus_->Get_I2cBusHandle(),
        ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
        &g_io_expander);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[BOOT]", "TCA9554 init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_io_expander_gpio_wrapper_append_handler(g_io_expander, GPIO_NUM_MAX);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[BOOT]", "EXIO wrapper init failed: %s", esp_err_to_name(err));
        return err;
    }

    DebugSerial::Log("[BOOT]", "enabling EPD, audio, and battery rails");
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_Audio_ON();
    BoardPower_VBAT_ON();

    ESP_ERROR_CHECK(gpio_set_direction(EXIO_NUM_3, GPIO_MODE_OUTPUT));
    gpio_set_level(EXIO_NUM_3, 1);
    return ESP_OK;
}

void BoardSupport::PowerDownForSleep() {
    DebugSerial::LogAlways("[POWER]", "powering down e-paper; keeping I2C peripheral rails alive");
    BoardPower_EPD_OFF();
}

void BoardSupport::EnterDeepSleep(const char *reason, uint64_t wake_pin_mask) {
    DebugSerial::LogAlways("[POWER]", "entering deep sleep reason=%s wake_mask=0x%llx low",
                           reason == nullptr ? "unknown" : reason,
                           static_cast<unsigned long long>(wake_pin_mask));

    if (wake_pin_mask & BIT64(PWR_BUTTON_PIN)) {
        esp_err_t err = ConfigureWakePin(PWR_BUTTON_PIN);
        if (err != ESP_OK) {
            DebugSerial::LogAlways("[POWER]", "PWR wake GPIO config failed: %s", esp_err_to_name(err));
            return;
        }
    }

    if (wake_pin_mask & BIT64(BOOT_BUTTON_PIN)) {
        esp_err_t err = ConfigureWakePin(BOOT_BUTTON_PIN);
        if (err != ESP_OK) {
            DebugSerial::LogAlways("[POWER]", "BOOT wake GPIO config failed: %s", esp_err_to_name(err));
            return;
        }
    }

    if ((wake_pin_mask & (BIT64(PWR_BUTTON_PIN) | BIT64(BOOT_BUTTON_PIN))) == 0) {
        DebugSerial::LogAlways("[POWER]", "deep sleep wake mask is empty");
        return;
    }

    esp_err_t err = esp_deep_sleep_enable_gpio_wakeup(wake_pin_mask, ESP_GPIO_WAKEUP_GPIO_LOW);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[POWER]", "deep sleep wake config failed: %s", esp_err_to_name(err));
        return;
    }

    PowerDownForSleep();
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_deep_sleep_start();
}

I2cMasterBus *BoardSupport::i2c_bus() const {
    return i2c_bus_;
}

}  // namespace bisc8
