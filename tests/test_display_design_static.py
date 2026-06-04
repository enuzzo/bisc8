from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DISPLAY_CPP = ROOT / "firmware/bisc8_fortune/main/display_service.cpp"
DISPLAY_H = ROOT / "firmware/bisc8_fortune/main/display_service.h"
APP_MAIN_CPP = ROOT / "firmware/bisc8_fortune/main/app_main.cpp"
BOARD_CPP = ROOT / "firmware/bisc8_fortune/main/board_support.cpp"
BOARD_H = ROOT / "firmware/bisc8_fortune/main/board_support.h"
KCONFIG = ROOT / "firmware/bisc8_fortune/main/Kconfig.projbuild"


def test_display_service_exposes_boot_brand_screen():
    header = DISPLAY_H.read_text(encoding="utf-8")
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    assert "void ShowBoot();" in header
    assert "by Netmilk Studio" in source
    assert "oracle loading" in source


def test_display_service_builds_occult_clean_frame():
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    assert "CreateOracleFrame" in source
    assert "CreateCookieIcon" in source
    assert "CreateLogoIcon" in source
    assert "SetDecorations" in source


def test_boot_screen_uses_background_jingle_instead_of_blocking_delay():
    source = APP_MAIN_CPP.read_text(encoding="utf-8")

    assert "display.ShowBoot();" in source
    assert "audio.PlayCueAsync(AudioCue::Boot)" in source
    assert "vTaskDelay(pdMS_TO_TICKS(3600))" not in source


def test_low_power_mode_is_configurable_but_safe_by_default():
    source = KCONFIG.read_text(encoding="utf-8")

    assert "config BISC8_AUTO_SLEEP_AFTER_FORTUNE" in source
    assert "default n" in source
    assert "config BISC8_AUTO_SLEEP_DELAY_MS" in source
    assert "default 2500" in source
    assert "config BISC8_MANUAL_DEEP_SLEEP_ENABLED" in source


def test_idle_deep_sleep_defaults_to_three_minutes():
    source = KCONFIG.read_text(encoding="utf-8")

    assert "config BISC8_IDLE_DEEP_SLEEP_ENABLED" in source
    assert "config BISC8_IDLE_SLEEP_DELAY_MS" in source
    assert "default 180000" in source
    assert "range 30000 900000" in source


def test_board_support_enters_real_deep_sleep_from_pwr_button():
    header = BOARD_H.read_text(encoding="utf-8")
    source = BOARD_CPP.read_text(encoding="utf-8")

    assert "void EnterDeepSleep(const char *reason, uint64_t wake_pin_mask);" in header
    assert '#include <esp_sleep.h>' in source
    assert "esp_deep_sleep_enable_gpio_wakeup(wake_pin_mask, ESP_GPIO_WAKEUP_GPIO_LOW)" in source
    assert "ConfigureWakePin(PWR_BUTTON_PIN)" in source
    assert "ConfigureWakePin(BOOT_BUTTON_PIN)" in source
    assert "BoardPower_EPD_OFF();" in source
    assert "esp_deep_sleep_start();" in source


def test_deep_sleep_keeps_i2c_powered_devices_alive():
    source = BOARD_CPP.read_text(encoding="utf-8")
    sleep_body = source.split("void BoardSupport::PowerDownForSleep()", 1)[1].split("void BoardSupport::EnterDeepSleep", 1)[0]

    assert "BoardPower_EPD_OFF();" in sleep_body
    assert "BoardPower_Audio_OFF();" not in sleep_body
    assert "BoardPower_VBAT_OFF();" not in sleep_body
    assert "keeping I2C peripheral rails alive" in sleep_body


def test_board_support_recovers_i2c_bus_before_tca9554_init():
    source = BOARD_CPP.read_text(encoding="utf-8")
    init_body = source.split("esp_err_t BoardSupport::Initialize()", 1)[1].split("void BoardSupport::PowerDownForSleep", 1)[0]

    assert "RecoverI2cBus();" in source
    assert "RecoverI2cBus();" in init_body
    assert "ESP32_I2C_SCL_PIN" in source
    assert "ESP32_I2C_SDA_PIN" in source
    assert "gpio_deep_sleep_hold_dis();" in source
    assert "gpio_hold_dis(ESP32_I2C_SCL_PIN);" in source
    assert "gpio_hold_dis(ESP32_I2C_SDA_PIN);" in source
    assert "GPIO_MODE_INPUT_OUTPUT_OD" in source
    assert "LogI2cLevels" in source
    assert "I2C levels" in source
    assert "for (int i = 0; i < 32; ++i)" in source
    assert "esp_rom_delay_us" in source


def test_fortune_auto_sleep_keeps_the_fortune_visible():
    source = APP_MAIN_CPP.read_text(encoding="utf-8")

    assert "CONFIG_BISC8_AUTO_SLEEP_AFTER_FORTUNE" in source
    assert "CONFIG_BISC8_AUTO_SLEEP_DELAY_MS" in source
    fortune_case = source.split("case AppEvent::GenerateFortune:", 1)[1].split("case AppEvent::MicTest:", 1)[0]
    assert "display.ShowFortune" in fortune_case
    assert "audio.PlayCue(AudioCue::OracleButton)" in fortune_case
    assert "board.EnterDeepSleep(\"fortune\", kAnyButtonWakeMask)" in fortune_case
    assert "display.ShowSleep" not in fortune_case


def test_idle_timeout_enters_deep_sleep_wakeable_by_any_button():
    source = APP_MAIN_CPP.read_text(encoding="utf-8")

    assert "constexpr uint64_t kAnyButtonWakeMask = BIT64(BOOT_BUTTON_PIN) | BIT64(PWR_BUTTON_PIN);" in source
    assert "CONFIG_BISC8_IDLE_DEEP_SLEEP_ENABLED" in source
    assert "CONFIG_BISC8_IDLE_SLEEP_DELAY_MS" in source
    assert "xQueueReceive(g_event_queue, &event, pdMS_TO_TICKS(CONFIG_BISC8_IDLE_SLEEP_DELAY_MS))" in source
    assert "board.EnterDeepSleep(\"idle-timeout\", kAnyButtonWakeMask)" in source


def test_pwr_long_press_shows_power_off_prompt_and_wakes_only_from_pwr():
    source = APP_MAIN_CPP.read_text(encoding="utf-8")
    display_header = DISPLAY_H.read_text(encoding="utf-8")
    display_source = DISPLAY_CPP.read_text(encoding="utf-8")

    sleep_case = source.split("case AppEvent::Sleep:", 1)[1]
    assert "CONFIG_BISC8_MANUAL_DEEP_SLEEP_ENABLED" in sleep_case
    assert "display.ShowPowerOff(ParseLanguage(settings.language.c_str()))" in sleep_case
    assert "audio.PlayCue(AudioCue::Shutdown)" in sleep_case
    assert "board.EnterDeepSleep(\"power-off\", BIT64(PWR_BUTTON_PIN))" in sleep_case
    assert "void ShowPowerOff(Language language);" in display_header
    assert "strings.sleep_footer" in display_source
    assert "by Netmilk Studio" in display_source


def test_display_service_exposes_wifi_and_localized_voice_states():
    header = DISPLAY_H.read_text(encoding="utf-8")
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    for method in (
        "ShowWifiConnecting",
        "ShowWifiSetup",
            "ShowVoiceListening",
            "ShowVoiceCooking",
            "ShowVoiceThinking",
            "ShowVoiceSpeaking",
    ):
        assert method in header
        assert method in source
    assert "Setup Wi-Fi" in source
    assert "Join Bisc8-XXXX" in source
    assert "Open 192.168.4.1" in source
    assert "strings.listening_body" in source
    assert "strings.cooking_title" in source
