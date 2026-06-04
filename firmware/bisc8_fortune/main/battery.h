#pragma once

#include <stdint.h>

namespace bisc8 {

// Battery sensing for the Waveshare ESP32-C6-ePaper-1.54: VBAT is on ADC1
// channel 0 (GPIO0) through a 1:2 divider, gated by the VBAT_PWR rail (EXIO5,
// enabled by BoardSupport). Pin/divider per the board's official ADC example.
void BatteryInit();

// Battery voltage in volts (0 if the ADC is unavailable).
float BatteryVoltage();

// Battery charge 0-100, or 255 if unknown / ADC unavailable (so the UI can
// hide the indicator instead of showing a fake value).
uint8_t BatteryLevel();

}  // namespace bisc8
