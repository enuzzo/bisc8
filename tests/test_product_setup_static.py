import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
README = ROOT / "README.md"
PARTITIONS = ROOT / "firmware/bisc8_fortune/partitions.csv"
SDKCONFIG = ROOT / "firmware/bisc8_fortune/sdkconfig"
SDKCONFIG_DEFAULTS = ROOT / "firmware/bisc8_fortune/sdkconfig.defaults"
FLASH_PAGE = ROOT / "public/flash/index.html"
FLASH_MANIFEST = ROOT / "public/flash/manifest.json"
FLASH_PREP = ROOT / "tools/prepare_web_flash.py"
SOUND_ASSETS = MAIN / "generated/sound_assets.h"
SOUND_ASSETS_SOURCE = MAIN / "generated/sound_assets.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def test_config_schema_declares_product_limits_and_secret_masking():
    header = read(MAIN / "app_config.h")
    source = read(MAIN / "app_config.cpp")

    assert "constexpr size_t kMaxWifiCredentials = 8" in header
    assert "constexpr size_t kMaxScreenAnswerChars = 100" in header
    assert "constexpr uint32_t kWifiAttemptTimeoutMs = 5000" in header
    assert "constexpr uint32_t kVoiceRecordLimitMs = 15000" in header
    assert "MaskSecret" in header
    assert 'return "***";' in source
    assert "OpenAiSettings" in header
    assert "EmailSettings" in header
    assert "DefaultEmailSettings" in header


def test_config_store_uses_nvs_namespace_keys_and_schema_version():
    header = read(MAIN / "app_config.h")
    source = read(MAIN / "app_config.cpp")

    assert "constexpr uint32_t kConfigSchemaVersion = 2" in header
    assert "class ConfigStore" in header
    for method in ("Init", "Load", "Save", "Reset"):
        assert f"esp_err_t {method}" in header
    for token in (
        '"bisc8"',
        '"schema"',
        '"language"',
        '"wifi_count"',
        '"openai_key"',
        '"email_relay_url"',
        '"email_relay_token"',
        '"email_recipient"',
        '"smtp_recipient"',
    ):
        assert token in source
    assert "nvs_flash_init" in source
    assert "nvs_open" in source
    assert "nvs_set_str" in source
    assert "nvs_get_str" in source
    assert "nvs_erase_all" in source
    assert "kMaxWifiCredentials" in source


def test_partition_table_reserves_audio_spool_storage():
    partitions = read(PARTITIONS)
    sdkconfig = read(SDKCONFIG)
    sdkconfig_defaults = read(SDKCONFIG_DEFAULTS)

    assert "spool" in partitions
    assert "assets" in partitions
    assert "0x40" in partitions
    assert "0x41" in partitions
    assert "0x600000" in partitions
    assert "0x610000" in partitions
    assert "0x500000" in partitions
    assert "0xb10000" in partitions
    assert "0x4f0000" in partitions
    assert 'CONFIG_ESPTOOLPY_FLASHSIZE="16MB"' in sdkconfig
    assert "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y" in sdkconfig_defaults


def test_localization_exposes_required_languages_and_display_states():
    header = read(MAIN / "localization.h")
    source = read(MAIN / "localization.cpp")

    for lang in ("en", "es", "it"):
        assert f'"{lang}"' in source
    for key in (
        "boot_status",
        "wifi_setup_title",
        "wifi_setup_body",
        "listening_title",
        "thinking_title",
        "cooking_title",
        "cooking_body",
        "speaking_title",
        "offline_title",
        "sleep_footer",
    ):
        assert key in header
        assert key in source
    assert 'DefaultLanguage()' in header
    assert 'return "en";' in source


def test_portal_declares_required_routes_and_masks_secrets():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")

    for route in (
        '"/"',
        '"/api/status"',
        '"/api/wifi/scan"',
        '"/api/wifi/credentials"',
        '"/api/settings"',
        '"/api/openai"',
        '"/api/email"',
        '"/api/smtp"',
        '"/api/reset"',
    ):
        assert route in source
    assert "secrets are stored on this device" in source
    assert "MaskSecret" in source
    assert "192.168.4.1" in source
    assert "CaptivePortalProbePaths" in header


def test_portal_runs_http_server_and_redirects_captive_probes():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")
    cmake = read(MAIN / "CMakeLists.txt")

    for token in (
        "httpd_start",
        "httpd_register_uri_handler",
        "max_uri_handlers",
        "httpd_resp_send",
        "httpd_resp_set_status(req, \"302 Found\")",
        "httpd_resp_set_hdr(req, \"Location\", \"/\")",
        "httpd_stop",
        "WifiStatus",
        "DeviceSettings",
        "BindStatus",
    ):
        assert token in source or token in header
    assert "esp_http_server" in cmake


def test_captive_dns_redirects_setup_clients_to_local_portal():
    header = read(MAIN / "captive_dns_service.h")
    source = read(MAIN / "captive_dns_service.cpp")
    connectivity = read(MAIN / "connectivity_service.cpp")
    cmake = read(MAIN / "CMakeLists.txt")

    for token in (
        "CaptiveDnsService",
        "socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)",
        "htons(kDnsPort)",
        "BuildResponse",
        "0x8180",
        "0xc00c",
        "kAnswerTtlSeconds",
        "sendto",
        "closesocket",
    ):
        assert token in header or token in source
    assert "kSetupIpHostOrder = 0xC0A80401" in connectivity
    assert "dns_.Start(kSetupIpHostOrder)" in connectivity
    assert "dns_.Stop()" in connectivity
    assert "captive_dns_service.cpp" in cmake
    assert "lwip" in cmake


def test_portal_has_real_forms_post_handlers_and_config_save():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")

    for token in (
        "BindConfig",
        "ConfigStore",
        "HTTP_POST",
        "HandleWifiCredentials",
        "HandleSettings",
        "HandleOpenAi",
        "HandleEmail",
        "HandleReset",
        "ReadRequestBody",
        "UrlDecode",
        "ParseForm",
        "config_store_->Save",
        "config_store_->Reset",
        "esp_wifi_scan_start",
        "esp_wifi_scan_get_ap_records",
        "WifiBandLabel",
        '\\"channel\\"',
        '\\"band\\"',
    ):
        assert token in source or token in header
    for html in (
        "data-bind=\"setup_ssid\"",
        "name=\"ssid\"",
        "name=\"language\"",
        "name=\"api_key\"",
        "name=\"recipient\"",
        "name=\"relay_url\"",
        "name=\"relay_token\"",
        "api('/api/status')",
        "data-api=\"/api/wifi/credentials\"",
        "data-api=\"/api/openai\"",
        "data-api=\"/api/email\"",
        "api('/api/reset'",
        "n.band",
    ):
        assert html in source


def test_connectivity_service_documents_multi_wifi_attempt_policy():
    header = read(MAIN / "connectivity_service.h")
    source = read(MAIN / "connectivity_service.cpp")

    assert "kMaxWifiCredentials" in source
    assert "kWifiAttemptTimeoutMs" in source
    assert "Bisc8-XXXX" in source
    assert "StartSetupPortal" in header
    assert "TryKnownNetworks" in header


def test_connectivity_service_uses_real_wifi_scan_sta_and_softap_fallback():
    header = read(MAIN / "connectivity_service.h")
    source = read(MAIN / "connectivity_service.cpp")
    cmake = read(MAIN / "CMakeLists.txt")

    for token in (
        "esp_netif_init",
        "esp_event_loop_create_default",
        "esp_wifi_init",
        "esp_wifi_scan_start",
        "esp_wifi_scan_get_ap_records",
        "wifi_config_t sta_config",
        "esp_wifi_connect",
        "xEventGroupWaitBits",
        "WIFI_CONNECTED_BIT",
        "Bisc8-",
        "esp_netif_create_default_wifi_ap",
        "esp_wifi_set_mode(WIFI_MODE_APSTA)",
        "esp_wifi_set_config(WIFI_IF_AP",
        "192.168.4.1",
        "captive DNS",
    ):
        assert token in source
    assert "DeviceSettings" in header
    assert "WifiStatus" in header
    assert "ssid_attempt" in header
    assert "setup_ssid" in header
    assert "esp_wifi" in cmake
    assert "esp_netif" in cmake
    assert "esp_event" in cmake


def test_voice_oracle_contract_and_audio_limits_are_explicit():
    header = read(MAIN / "voice_oracle_service.h")
    source = read(MAIN / "voice_oracle_service.cpp")
    audio_header = read(MAIN / "audio_service.h")

    for field in (
        "detected_language",
        "transcript",
        "oracle_answer_full",
        "oracle_answer_screen",
        "tts_text",
    ):
        assert field in source
    assert "audio/transcriptions" in source
    assert "responses" in source
    assert "audio/speech" in source
    assert "kMaxScreenAnswerChars" in source
    assert "StartVoiceRecording" in audio_header
    assert "FinishVoiceRecording" in audio_header


def test_voice_recording_spools_mono_wav_to_flash():
    header = read(MAIN / "audio_service.h")
    source = read(MAIN / "audio_service.cpp")
    cmake = read(MAIN / "CMakeLists.txt")

    for token in (
        "PrepareSpool",
        "VoiceRecordTask",
        "esp_partition_find_first",
        "esp_partition_erase_range",
        "esp_partition_write",
        '"spool"',
        '"spool://question.wav"',
        "BuildWavHeader",
        '"RIFF"',
        '"WAVE"',
        "kVoiceChannels = 1",
        "kVoiceMaxChunks",
        "Codec_RecordData",
        "xTaskCreate",
    ):
        assert token in header or token in source
    assert "fatfs" not in cmake
    assert "wear_levelling" not in cmake


def test_audio_initializes_before_wifi_and_setup_portal():
    app_main = read(MAIN / "app_main.cpp")

    audio_init = app_main.index("err = audio.Initialize();")
    wifi_try = app_main.index("connectivity.TryKnownNetworks")
    setup_start = app_main.index("connectivity.StartSetupPortal")
    assert audio_init < wifi_try
    assert audio_init < setup_start


def test_audio_feedback_uses_short_generated_chime():
    source = read(MAIN / "audio_service.cpp")
    header = read(MAIN / "audio_service.h")
    cmake = read(MAIN / "CMakeLists.txt")
    sound_header = read(SOUND_ASSETS)
    sound_source = read(SOUND_ASSETS_SOURCE)

    assert "PrepareChime" in source
    assert "kChimeMillis" in source
    assert "sinf" in source
    assert "decay" in source
    assert "Codec_PlaybackData" in source
    for token in (
        "enum class AudioCue",
        "PlayCueAsync",
        "StopPlayback",
        "kSoundBoot",
        "kSoundOracleButton",
        "kSoundVoiceSubmit",
        "kSoundShutdown",
        "PlaybackTask",
    ):
        assert token in header or token in source or token in sound_header or token in sound_source
    assert "generated/sound_assets.cpp" in cmake


def test_long_audio_cues_survive_epaper_screen_refreshes():
    audio_source = read(MAIN / "audio_service.cpp")
    audio_header = read(MAIN / "audio_service.h")
    app_main = read(MAIN / "app_main.cpp")
    lvgl_source = read(ROOT / "firmware/bisc8_fortune/components/port_bsp/port_lvgl.cpp")

    playback_priority = re.search(r"kPlaybackTaskPriority\s*=\s*(\d+)", audio_source)
    lvgl_priority = re.search(r'xTaskCreatePinnedToCore\(Lvgl_port_task,\s*"LVGL",\s*8 \* 1024,\s*NULL,\s*(\d+)', lvgl_source)
    assert playback_priority is not None
    assert lvgl_priority is not None
    assert int(playback_priority.group(1)) > int(lvgl_priority.group(1))
    assert "WaitForPlayback" in audio_header
    assert "WaitForPlayback" in audio_source
    play_cue = audio_source[audio_source.index("void AudioService::PlayCue(AudioCue cue)"):audio_source.index("void AudioService::PlayCueAsync(AudioCue cue)")]
    assert "PlayCueAsync(cue);" in play_cue
    assert "WaitForPlayback(" in play_cue
    assert "Codec_PlaybackData" not in play_cue

    sleep_case = app_main[app_main.index("case AppEvent::Sleep:"):app_main.index("#if CONFIG_BISC8_MANUAL_DEEP_SLEEP_ENABLED")]
    assert "display.ShowPowerOff(ParseLanguage(settings.language.c_str()))" in sleep_case
    assert "audio.PlayCueAsync(AudioCue::Shutdown);" in sleep_case
    assert "audio.WaitForPlayback(" in sleep_case
    assert sleep_case.index("audio.PlayCueAsync(AudioCue::Shutdown);") < sleep_case.index("audio.WaitForPlayback(")


def test_sound_asset_pipeline_uses_candidate_sources_and_firmware_previews():
    script = read(ROOT / "tools/generate_sound_assets.py")

    for token in (
        "assets/candidate/full-reboot.mp3",
        "assets/candidate/oracle-button.ogg",
        "assets/candidate/oracle-audio-before.ogg",
        "assets/candidate/shutdown.ogg",
        "assets/sounds/firmware",
        "ffmpeg",
        "ffprobe",
        "-f",
        "s16le",
        "SAMPLE_RATE = 16000",
        "CHANNELS = 2",
    ):
        assert token in script


def test_email_service_uses_relay_contract_and_degrades_to_text_only():
    header = read(MAIN / "email_service.h")
    source = read(MAIN / "email_service.cpp")

    assert "SendOracleEmail" in header
    assert "EmailSettings" in header
    assert "relay HTTPS POST" in source
    assert "text-only" in source
    assert "attachment upload pending" in source


def test_button_events_cover_voice_and_setup_recovery():
    events = read(MAIN / "app_events.h")
    buttons = read(MAIN / "button_controller.cpp")
    app_main = read(MAIN / "app_main.cpp")

    assert "StartVoiceRecording" in events
    assert "FinishVoiceRecording" in events
    assert "ForceWifiSetup" in events
    assert "FullConfigReset" in events
    assert "boot_button_->OnPressDown" in buttons
    assert "boot_button_->OnPressUp" in buttons
    assert "BOOT press down" in buttons
    assert "BOOT hold start" in buttons
    assert "BOOT release" in buttons
    assert "BOOT+PWR" in buttons
    assert "VOICE START" in app_main
    assert "VOICE STOP" in app_main
    assert "ShowVoiceCooking" in app_main
    assert "AudioCue::VoiceSubmit" in app_main
    assert "gpio_get_level(BOOT_BUTTON_PIN) == 0" in app_main
    assert "ForceWifiSetup" in app_main
    assert app_main.count("connectivity.StartSetupPortal(display, portal)") >= 4


def test_readme_documents_product_setup_and_logo_requirements():
    readme = read(README)

    for phrase in (
        "First boot defaults to English",
        "Bisc8-XXXX",
        "http://192.168.4.1",
        "OpenAI API key",
        "email relay",
        "Hold BOOT to ask",
        "BOOT + PWR",
        "1024x1024",
        "64x64",
        "secrets are stored on the device",
    ):
        assert phrase in readme


def test_public_flash_page_uses_web_serial_manifest_without_secrets():
    page = read(FLASH_PAGE)
    manifest = read(FLASH_MANIFEST)
    prep = read(FLASH_PREP)
    readme = read(README)

    assert "esp-web-tools@10.1.0" in page
    assert '<esp-web-install-button manifest="./manifest.json">' in page
    assert "Web Serial" in page
    assert "Bisc8-XXXX" in page
    assert "192.168.4.1" in page
    assert '"chipFamily": "ESP32-C6"' in manifest
    assert '"offset": 0' in manifest
    assert '"offset": 32768' in manifest
    assert '"offset": 65536' in manifest
    assert "bootloader.bin" in manifest
    assert "partition-table.bin" in manifest
    assert "bisc8_fortune.bin" in manifest
    assert "copy_artifact" in prep
    assert "--build-dir" in prep
    for forbidden in ("sk-", "smtp_password", "relay_token"):
        assert forbidden not in page
        assert forbidden not in manifest
    assert "Public Web Flasher" in readme
