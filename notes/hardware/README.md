# Bisc8 hardware reference (Waveshare ESP32-C6-ePaper-1.54)

Local copies of the board documentation, saved so we don't depend on
Waveshare's site (the wiki blocks automated access).

## Files here
- `ESP32-C6-ePaper-1.54-Schematic.pdf` — official board schematic.
- `waveshare-adc-reference/` — Waveshare's official battery-ADC example
  (`01_ADC_Test`): `port_adc.cpp/.h`, `epaper_config.h`, `01_ADC_Test.ino`.
  This is the source of truth for the battery sensing we implemented in
  `firmware/.../main/battery.cpp`.

## Battery sensing (confirmed from the ADC example)
- **VBAT → ADC1 channel 0 = GPIO0**, through a **1:2 voltage divider**.
- Measurement rail enabled by **VBAT_PWR = EXIO5** (TCA9554 IO-expander), which
  `BoardSupport` already turns on at boot.
- Read: 12-bit, `ADC_ATTEN_DB_12`, curve-fitting calibration, average 16 samples,
  then `Vbat = mV * 2 / 1000`.
- Charge mapping: **full 4.12 V, empty 3.0 V**, linear 0-100%.

## Pin map (from Waveshare `epaper_config.h`)
| Function | Pin |
|---|---|
| EPD DC / CS / SCK / MOSI / RST / BUSY / MISO | GPIO15 / 7 / 6 / 5 / 11 / 10 / 4 |
| I2C SDA / SCL | GPIO18 / GPIO8 |
| Buttons BOOT / PWR | GPIO9 / GPIO2 |
| Battery sense | GPIO0 (ADC1_CH0) |
| SD MISO / MOSI / CLK / CS | GPIO4 / 5 / 6 / 3 |
| EXIO (TCA9554): EPD_PWR / Audio_PWR / LED / VBAT_PWR | EXIO0 / 1 / 4 / 5 |

On-board I2C peripherals also include a **PCF85063** RTC and an **SHTC3**
temperature/humidity sensor (not yet used by the firmware).

## Sources
- Repo: https://github.com/waveshareteam/ESP32-C6-ePaper-1.54
- Docs: https://docs.waveshare.com/ESP32-C6-ePaper-1.54
- Product: https://www.waveshare.com/esp32-c6-epaper-1.54.htm
- ESP32-C6 datasheet: https://documentation.espressif.com/esp32-c6_datasheet_en.pdf
