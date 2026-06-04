#include "battery.h"

#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>

#include "debug_serial.h"

namespace bisc8 {

namespace {

constexpr float kVoltFull = 4.12f;
constexpr float kVoltEmpty = 3.0f;

adc_oneshot_unit_handle_t s_adc = nullptr;
adc_cali_handle_t s_cali = nullptr;
bool s_ready = false;

}  // namespace

void BatteryInit() {
    adc_oneshot_unit_init_cfg_t init = {};
    init.unit_id = ADC_UNIT_1;
    if (adc_oneshot_new_unit(&init, &s_adc) != ESP_OK) {
        DebugSerial::LogAlways("[BATT]", "ADC init failed");
        return;
    }
    adc_oneshot_chan_cfg_t chan = {};
    chan.bitwidth = ADC_BITWIDTH_12;
    chan.atten = ADC_ATTEN_DB_12;
    if (adc_oneshot_config_channel(s_adc, ADC_CHANNEL_0, &chan) != ESP_OK) {
        DebugSerial::LogAlways("[BATT]", "ADC channel config failed");
        return;
    }
    adc_cali_curve_fitting_config_t cali = {};
    cali.unit_id = ADC_UNIT_1;
    cali.atten = ADC_ATTEN_DB_12;
    cali.bitwidth = ADC_BITWIDTH_12;
    if (adc_cali_create_scheme_curve_fitting(&cali, &s_cali) != ESP_OK) {
        DebugSerial::LogAlways("[BATT]", "ADC calibration unavailable");
        return;
    }
    s_ready = true;
}

float BatteryVoltage() {
    if (!s_ready) {
        return 0.0f;
    }
    int accum_mv = 0;
    int samples = 0;
    for (int i = 0; i < 16; ++i) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_0, &raw) != ESP_OK) {
            continue;
        }
        int mv = 0;
        if (adc_cali_raw_to_voltage(s_cali, raw, &mv) != ESP_OK) {
            continue;
        }
        accum_mv += mv;
        ++samples;
    }
    if (samples == 0) {
        return 0.0f;
    }
    return 0.001f * (static_cast<float>(accum_mv) / samples) * 2.0f;  // 1:2 divider
}

uint8_t BatteryLevel() {
    if (!s_ready) {
        return 255;
    }
    float v = BatteryVoltage();
    if (v <= 0.1f) {
        return 255;
    }
    if (v <= kVoltEmpty) {
        return 0;
    }
    if (v >= kVoltFull) {
        return 100;
    }
    return static_cast<uint8_t>((v - kVoltEmpty) / (kVoltFull - kVoltEmpty) * 100.0f);
}

}  // namespace bisc8
