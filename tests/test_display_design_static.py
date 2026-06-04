from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DISPLAY_CPP = ROOT / "firmware/bisc8_fortune/main/display_service.cpp"
DISPLAY_H = ROOT / "firmware/bisc8_fortune/main/display_service.h"
FACTEST_C = ROOT / "firmware/bisc8_fortune/components/externlib/ui/FacTest_ui.c"
FACTEST_H = ROOT / "firmware/bisc8_fortune/components/externlib/ui/FacTest_ui.h"
APP_MAIN_CPP = ROOT / "firmware/bisc8_fortune/main/app_main.cpp"
BOARD_CPP = ROOT / "firmware/bisc8_fortune/main/board_support.cpp"
BOARD_H = ROOT / "firmware/bisc8_fortune/main/board_support.h"
KCONFIG = ROOT / "firmware/bisc8_fortune/main/Kconfig.projbuild"
LOCALIZATION_CPP = ROOT / "firmware/bisc8_fortune/main/localization.cpp"
UI_RES = ROOT / "firmware/bisc8_fortune/components/externlib/ui_res"
FONT_GEN = ROOT / "tools/generate_display_fonts.py"
LOGO_GEN = ROOT / "tools/generate_logo_assets.py"
LOGO_SRC = ROOT / "assets/logo/logo_min.png"
LOGO_H = ROOT / "firmware/bisc8_fortune/main/generated/logo_assets.h"
LOGO_CPP = ROOT / "firmware/bisc8_fortune/main/generated/logo_assets.cpp"
CMAKE = ROOT / "firmware/bisc8_fortune/main/CMakeLists.txt"
README = ROOT / "README.md"


def test_display_service_exposes_boot_brand_screen():
    header = DISPLAY_H.read_text(encoding="utf-8")
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    assert "void ShowBoot();" in header
    assert "by Netmilk Studio" in source
    assert "oracle loading" in source


def test_display_service_builds_occult_clean_frame():
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    assert "CreateOracleFrame" in source
    assert "CreateLogoIcon" in source
    assert "lv_image_set_src" in source
    assert "kBisc8BootLogo" in source
    assert "SetDecorations" in source


def test_boot_logo_uses_generated_min_png_bitmap_asset():
    display = DISPLAY_CPP.read_text(encoding="utf-8")
    cmake = CMAKE.read_text(encoding="utf-8")
    readme = README.read_text(encoding="utf-8")

    assert LOGO_SRC.exists()
    assert LOGO_GEN.exists()
    generator = LOGO_GEN.read_text(encoding="utf-8")
    assert "assets/logo/logo_min.png" in generator
    assert "64" in generator
    assert "ffmpeg" in generator
    assert "threshold" in generator

    assert LOGO_H.exists()
    assert LOGO_CPP.exists()
    header = LOGO_H.read_text(encoding="utf-8")
    source = LOGO_CPP.read_text(encoding="utf-8")
    assert "extern const lv_image_dsc_t kBisc8BootLogo;" in header
    assert "LV_COLOR_FORMAT_RGB565" in source
    assert "LV_IMAGE_HEADER_MAGIC" in source
    assert "{LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_RGB565, 0, 64, 64, 128, 0}" in source
    assert "generated/logo_assets.h" in display
    assert "generated/logo_assets.cpp" in cmake
    assert "tools/generate_logo_assets.py" in readme
    assert "assets/logo/logo_min.png" in readme


def test_display_fonts_cover_latin_1_accents():
    display = DISPLAY_CPP.read_text(encoding="utf-8")
    generator = FONT_GEN.read_text(encoding="utf-8")
    body = (UI_RES / "bisc8_font_body_16.c").read_text(encoding="utf-8")
    ui = (UI_RES / "bisc8_font_ui_14.c").read_text(encoding="utf-8")
    title = (UI_RES / "bisc8_font_title_25.c").read_text(encoding="utf-8")
    factest_c = FACTEST_C.read_text(encoding="utf-8")
    factest_h = FACTEST_H.read_text(encoding="utf-8")

    assert "LV_FONT_DECLARE(bisc8_font_title_25)" in display
    assert "LV_FONT_DECLARE(bisc8_font_ui_14)" in display
    assert "lv_font_montserrat" not in display
    assert 'LATIN_1_RANGE = "0x20-0xFF"' in generator
    assert "--lv-include" in generator
    for font in (body, ui, title):
        assert "0x20-0xFF" in font
        assert 'U+00E8 "è"' in font
    assert "bisc8_font_title_25" in factest_c
    assert "bisc8_font_title_25" in factest_h
    assert "lv_font_montserratMedium" not in factest_c
    assert "lv_font_montserratMedium" not in factest_h


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
    assert "audio.PlayCueAsync(AudioCue::Shutdown)" in sleep_case
    assert "audio.WaitForPlayback(" in sleep_case
    assert sleep_case.index("audio.PlayCueAsync(AudioCue::Shutdown)") < sleep_case.index("audio.WaitForPlayback(")
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
    assert "strings.wifi_setup_title" in source
    assert "strings.wifi_setup_body" in source
    assert "strings.wifi_setup_footer" in source
    assert "strings.listening_body" in source
    assert "strings.cooking_title" in source


def test_setup_connection_info_keeps_device_name_and_ip_on_one_line():
    display = DISPLAY_CPP.read_text(encoding="utf-8")
    localization = LOCALIZATION_CPP.read_text(encoding="utf-8")

    assert "SetupConnectionInfoLineLayout" in display
    assert "SetupDisplayAddress" in display
    assert "http://" in display
    assert "Bisc8-XXXX | 192.168.4.1" in localization
    assert '"%s | %s"' in localization
    assert "Bisc8-XXXX\\nOpen 192.168.4.1" not in localization
    assert "Bisc8-XXXX\\nAbre 192.168.4.1" not in localization
    assert "Bisc8-XXXX\\nApri 192.168.4.1" not in localization


def test_localized_display_strings_keep_required_accents_and_english_cooking():
    source = LOCALIZATION_CPP.read_text(encoding="utf-8")

    for phrase in (
        "oráculo cargando",
        "Mantén BOOT",
        "PWR energía",
        "Bisc8-XXXX | 192.168.4.1",
        "El oráculo lee tu pregunta.",
        "La pregunta está al fuego.",
    ):
        assert phrase in source
    assert '"Cooking"' in source
    assert "Bisc8 sta cucinando" not in source
    assert "Bisc8 esta cocinando" not in source
    assert "La domanda è sul fuoco." in source
