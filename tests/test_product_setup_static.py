import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
README = ROOT / "README.md"
AI_HANDOFF = ROOT / "notes/AI_HANDOFF.md"
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
    assert "constexpr size_t kMaxScreenAnswerChars = 55" in header
    assert "constexpr uint32_t kWifiAttemptTimeoutMs = 10000" in header
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
        '"email_relaytok"',
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


def test_all_nvs_keys_fit_esp_idf_key_length_limit():
    # ESP-IDF NVS keys (and namespaces) must be <= 15 characters:
    # NVS_KEY_NAME_MAX_SIZE is 16 *including* the null terminator. A longer key
    # makes nvs_set_str/nvs_get_str return ESP_ERR_NVS_KEY_TOO_LONG, which aborts
    # the whole ConfigStore::Save() transaction (so nothing, including Wi-Fi, is
    # committed).
    NVS_KEY_MAX_LEN = 15

    header = read(MAIN / "app_config.h")
    source = read(MAIN / "app_config.cpp")

    # Keys/namespaces declared as string-literal constants.
    literals = re.findall(r'constexpr const char \*\w+\s*=\s*"([^"]*)"', source)
    assert literals, "expected to find NVS key constants in app_config.cpp"
    too_long = [k for k in literals if len(k) > NVS_KEY_MAX_LEN]

    # Keys assembled at runtime with a numeric index suffix (e.g. wifi_ssid_<i>).
    max_index = int(re.search(r"kMaxWifiCredentials\s*=\s*(\d+)", header).group(1)) - 1
    suffix_len = len(str(max_index))
    for prefix in sorted(set(re.findall(r'IndexedKey\("([^"]*)"', source))):
        longest = len(prefix) + suffix_len
        if longest > NVS_KEY_MAX_LEN:
            too_long.append(f"{prefix}<index> ({longest} chars)")

    assert not too_long, (
        f"NVS keys exceeding the ESP-IDF {NVS_KEY_MAX_LEN}-char limit "
        f"(would raise ESP_ERR_NVS_KEY_TOO_LONG): {too_long}"
    )


def test_build_version_is_wired_into_portal_and_serial_log():
    cmake = read(MAIN / "CMakeLists.txt")
    gen = read(MAIN / "build_info.cmake")
    portal = read(MAIN / "web_portal.cpp")
    app_main = read(MAIN / "app_main.cpp")

    # Generator builds the version from git commit count + build timestamp,
    # and ignores untracked files when deciding the dirty (+) marker.
    assert "rev-list --count HEAD" in gen
    assert "BISC8_BUILD_VERSION" in gen
    assert "TIMESTAMP" in gen
    assert "--untracked-files=no" in gen

    # Build wires the generator and exposes its generated include dir.
    assert "build_info.cmake" in cmake
    assert "build_info" in cmake

    # Portal surfaces the version: JSON field + a data-bind badge in the header.
    assert '#include "build_info.h"' in portal
    assert "BISC8_BUILD_VERSION" in portal
    assert '"build_version"' in portal
    assert 'data-bind="build_version"' in portal

    # Serial log prints it at boot under a [BUILD] tag.
    assert "BISC8_BUILD_VERSION" in app_main
    assert "[BUILD]" in app_main


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
    assert "CONFIG_MBEDTLS_DYNAMIC_BUFFER=y" in sdkconfig
    assert "CONFIG_MBEDTLS_DYNAMIC_BUFFER=y" in sdkconfig_defaults


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
        "intro_title",
        "intro_body",
        "intro_footer",
        "status_title",
        "status_online_body",
        "status_setup_body",
        "status_footer",
        "voice_footer",
        "recording_failed_body",
        "voice_oracle_unconfigured_body",
        "error_footer",
    ):
        assert key in header
        assert key in source
    assert 'DefaultLanguage()' in header
    assert 'return "en";' in source
    assert "Pulsa PWR" in source
    assert "Premi PWR" in source


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
        '"/api/reboot"',
    ):
        assert route in source
    assert "secrets are stored on this device" in source
    assert "MaskSecret" in source
    assert "192.168.4.1" in source
    assert "CaptivePortalProbePaths" in header


def test_portal_localizes_visible_ui_in_supported_languages():
    source = read(MAIN / "web_portal.cpp")

    for token in (
        "const I18N",
        "applyLanguage",
        "data-i18n",
        "data-i18n-placeholder",
        "Guardar Wi-Fi",
        "Salva Wi-Fi",
        "Buscar redes",
        "Cerca reti",
        "Configuración borrada",
        "Configurazione cancellata",
    ):
        assert token in source


def test_portal_runs_http_server_and_redirects_captive_probes():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")
    cmake = read(MAIN / "CMakeLists.txt")

    for token in (
        "httpd_start",
        "httpd_register_uri_handler",
        "max_uri_handlers",
        "httpd_resp_send",
        'httpd_resp_set_hdr(req, "Cache-Control", "no-store, max-age=0")',
        'httpd_resp_set_hdr(req, "Pragma", "no-cache")',
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
        "data-bind=\"wifi_mode\"",
        "data-bind=\"device_address\"",
        "name=\"ssid\"",
        "data-lang=\"it\"",
        "name=\"api_key\"",
        "name=\"recipient\"",
        "name=\"relay_url\"",
        "name=\"relay_token\"",
        "api('/api/status')",
        "data-api=\"/api/wifi/credentials\"",
        "data-api=\"/api/openai\"",
        "data-api=\"/api/email\"",
        "api('/api/reset'",
        "api('/api/reboot'",
        "n.band",
    ):
        assert html in source


def test_wifi_save_tests_credentials_and_requires_reboot_to_apply():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")
    connectivity_header = read(MAIN / "connectivity_service.h")
    connectivity = read(MAIN / "connectivity_service.cpp")
    app_main = read(MAIN / "app_main.cpp")

    assert "BindRuntime" in header
    assert "portal.BindRuntime(&connectivity, &display)" in app_main
    assert "TestCredentials" in connectivity_header
    assert "ConnectToNetwork(ssid, password, display, language, false)" in connectivity
    assert "HandleReboot" in header
    assert "esp_restart()" in source
    assert "reboot_required" in source
    assert "reboot_required_ = true" in source

    wifi_body = source.split("esp_err_t WebPortal::HandleWifiCredentials", 1)[1].split("\nesp_err_t WebPortal::", 1)[0]
    assert "DeviceSettings candidate = *portal->settings_" in wifi_body
    assert "*portal->settings_ = candidate" in wifi_body
    # Save-then-restart: the handler saves and asks for a restart; it does NOT
    # live-test in AP mode (single-radio channel switch drops the portal).
    assert "reboot_required_ = true" in wifi_body
    assert "TestCredentials" not in wifi_body

    for token in (
        "id=\"rebar\"",
        "reboot_title",
        "reboot_now",
        "document.getElementById('reboot')",
    ):
        assert token in source


def test_setup_portal_does_not_require_pairing_pin_for_local_setup():
    header = read(MAIN / "web_portal.h")
    source = read(MAIN / "web_portal.cpp")
    connectivity = read(MAIN / "connectivity_service.cpp")

    for token in (
        "GeneratePairingPin",
        "PairingPin",
        "RequirePairingPin",
        "setup_pin_",
        "esp_random()",
        '"setup_pin"',
        '"403 Forbidden"',
        '"Setup PIN is required"',
    ):
        assert token not in source
        assert token not in header
    assert "portal.GeneratePairingPin();" not in connectivity

    for handler in (
        "HandleWifiCredentials",
        "HandleOpenAi",
        "HandleEmail",
        "HandleReset",
    ):
        body = source.split(f"esp_err_t WebPortal::{handler}", 1)[1].split("\nesp_err_t WebPortal::", 1)[0]
        assert "RequirePairingPin(req, form)" not in body


def test_setup_screen_and_status_do_not_expose_or_track_pairing_pin():
    connectivity_header = read(MAIN / "connectivity_service.h")
    connectivity = read(MAIN / "connectivity_service.cpp")
    display_header = read(MAIN / "display_service.h")
    display = read(MAIN / "display_service.cpp")
    localization_header = read(MAIN / "localization.h")
    localization = read(MAIN / "localization.cpp")
    web = read(MAIN / "web_portal.cpp")

    assert "std::string setup_pin" not in connectivity_header
    assert "status_.setup_pin" not in connectivity
    assert "portal.PairingPin()" not in connectivity
    assert "display.ShowWifiSetup(setup_ssid, kSetupUrl, language)" in connectivity
    assert "void ShowWifiSetup(const char *ssid, const char *url, Language language);" in display_header
    assert "strings.wifi_setup_pin_footer" not in display
    assert "wifi_setup_pin_footer" not in localization_header
    assert "PIN %s" not in localization
    assert '"collegati a\\n%s"' in localization

    status_json = web.split("esp_err_t WebPortal::SendStatusJson", 1)[1].split("esp_err_t WebPortal::SendWifiScanJson", 1)[0]
    assert '"setup_pin"' not in status_json
    assert "PairingPin()" not in status_json


def test_automatic_setup_portal_keeps_setup_screen_visible():
    app_main = read(MAIN / "app_main.cpp")

    automatic_setup = app_main.split(
        "connectivity.TryKnownNetworks(settings, display, startup_language, false) != ESP_OK",
        1,
    )[1].split("if (!setup_mode_active", 1)[0]
    assert "connectivity.StartSetupPortal(display, portal, startup_language, true)" in automatic_setup
    assert "display.ShowIntro(startup_language)" not in automatic_setup


def test_portal_ui_has_no_setup_pin_gate():
    source = read(MAIN / "web_portal.cpp")

    for token in (
        "setupPinTitle",
        "setupPinLabel",
        "setupPinHint",
        "setupPinRequired",
        'id="setup_pin"',
        "setupPinValue",
        "appendSetupPin",
        "data-pin=\"required\"",
        "Enter the PIN shown on Bisc8",
        "PIN mostrado en Bisc8",
        "PIN mostrato su Bisc8",
    ):
        assert token not in source


def test_portal_json_string_escapes_control_characters():
    source = read(MAIN / "web_portal.cpp")

    json_string = source[source.index("std::string JsonString"):source.index("const char *WifiBandLabel")]
    for token in (
        "'\\b'",
        '"\\\\b"',
        "'\\f'",
        '"\\\\f"',
        "'\\n'",
        '"\\\\n"',
        "'\\r'",
        '"\\\\r"',
        "'\\t'",
        '"\\\\t"',
        "ch < 0x20",
        '"\\\\u%04x"',
    ):
        assert token in json_string


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


def test_connected_wifi_status_exposes_sta_ip_to_web_and_intro_splash():
    header = read(MAIN / "connectivity_service.h")
    connectivity = read(MAIN / "connectivity_service.cpp")
    web = read(MAIN / "web_portal.cpp")
    display = read(MAIN / "display_service.cpp")
    localization = read(MAIN / "localization.cpp")
    app_main = read(MAIN / "app_main.cpp")

    assert "std::string connected_ip" in header
    assert "UpdateConnectedIp" in header
    assert "MarkDisconnected" in header
    assert "ip_event_got_ip_t" in connectivity
    assert "IP2STR" in connectivity
    assert "connected_ip" in web
    assert '"device_address"' in web
    assert 'data-bind="device_address"' in web
    assert "wifi_mode" in web
    assert "status.connected_ip" in display
    # Online status body now stacks SSID + IP on their own lines ("%s\n%s");
    # the connected/disconnected header lives in status_connected_title.
    assert '"%s\\n%s"' in localization
    assert "kOnlineStatusSplashMs" in app_main
    assert "display.ShowStatus(connectivity.Status(), startup_language)" in app_main


def test_setup_portal_uses_conservative_ap_and_http_resources():
    connectivity = read(MAIN / "connectivity_service.cpp")
    web = read(MAIN / "web_portal.cpp")

    assert "WIFI_EVENT_AP_STACONNECTED" in connectivity
    assert "WIFI_EVENT_AP_STADISCONNECTED" in connectivity
    assert "setup client leave" in connectivity
    assert "ap_config.ap.max_connection = 2;" in connectivity
    assert "config.max_open_sockets = 3;" in web
    assert "config.backlog_conn = 2;" in web
    assert "config.recv_wait_timeout = 16;" in web
    assert "config.send_wait_timeout = 16;" in web
    assert "GET /api/wifi/scan start" in web


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
    assert "chat/completions" in source  # Brain endpoint (chat completions, JSON mode)
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


def test_voice_recording_timeout_can_still_finalize_on_release():
    source = read(MAIN / "audio_service.cpp")
    finish_body = source.split("const char *AudioService::FinishVoiceRecording()", 1)[1].split(
        "esp_err_t AudioService::PlayAnswerAudio", 1
    )[0]

    assert "voice_file_ready_" in finish_body
    assert "voice recording auto-finished" in finish_body
    assert finish_body.index("voice_file_ready_") < finish_body.index("voice recording finish ignored")


def test_audio_record_buffer_is_lazy_to_keep_setup_portal_heap_available():
    header = read(MAIN / "audio_service.h")
    source = read(MAIN / "audio_service.cpp")
    init_body = source.split("esp_err_t AudioService::Initialize()", 1)[1].split("void AudioService::PrepareChime()", 1)[0]
    start_voice_body = source.split("void AudioService::StartVoiceRecording()", 1)[1].split("const char *AudioService::FinishVoiceRecording()", 1)[0]
    mic_body = source.split("void AudioService::RunMicTest", 1)[1].split("esp_err_t AudioService::PrepareSpool()", 1)[0]
    voice_task_body = source.split("void AudioService::VoiceRecordTask()", 1)[1]

    assert "esp_err_t EnsureRecordBuffer(size_t bytes);" in header
    assert "void ReleaseRecordBuffer();" in header
    assert "record_buffer=lazy" in source
    assert "heap_caps_malloc(record_bytes_" not in init_body
    # Voice recording uses a small dedicated chunk (reliable alloc + prompt stop);
    # the mic-test loopback keeps the larger record_bytes_ buffer. Both stay lazy.
    assert "EnsureRecordBuffer(kVoiceChunkBytes)" in start_voice_body
    assert "ReleaseRecordBuffer();" in start_voice_body
    assert "EnsureRecordBuffer(record_bytes_)" in mic_body
    assert "ReleaseRecordBuffer();" in mic_body
    assert "ReleaseRecordBuffer();" in voice_task_body


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
    # Real relay POST over TLS: multipart form-data, streaming the question WAV
    # from flash, Bearer-token auth, degrading to text-only when there is no WAV.
    assert "esp_http_client_open" in source
    assert "esp_crt_bundle_attach" in source
    assert "multipart/form-data" in source
    assert "esp_partition_read" in source
    assert '"Bearer "' in source
    assert "attach" in source


def test_button_events_cover_voice_and_setup_recovery():
    events = read(MAIN / "app_events.h")
    buttons = read(MAIN / "button_controller.cpp")
    app_main = read(MAIN / "app_main.cpp")

    assert "StartVoiceRecording" in events
    assert "FinishVoiceRecording" in events
    assert "ForceWifiSetup" in events
    assert "FullConfigReset" in events
    assert "ShowStatus" in events
    assert "boot_button_->OnPressDown" in buttons
    assert "boot_button_->OnPressUp" in buttons
    assert "BOOT press down" in buttons
    assert "BOOT hold start" in buttons
    assert "BOOT release" in buttons
    assert "BOOT+PWR" in buttons
    assert "power_button_->OnClick" in buttons
    assert "AppEvent::ShowStatus" in buttons
    assert "power_button_->OnMultipleClick" in buttons
    assert "PWR triple click: full config reset" in buttons
    assert "send_event(AppEvent::FullConfigReset)" in buttons
    assert "send_event(AppEvent::MicTest)" not in buttons
    assert "VOICE START" in app_main
    assert "VOICE STOP" in app_main
    assert "ShowVoiceCooking" in app_main
    assert "AudioCue::VoiceSubmit" in app_main
    assert "gpio_get_level(BOOT_BUTTON_PIN) == 0" in app_main
    assert "ForceWifiSetup" in app_main
    assert "display.ShowIntro" in app_main
    assert "display.ShowStatus" in app_main
    assert app_main.count("connectivity.StartSetupPortal(display, portal,") >= 4
    boot_section = app_main[app_main.index("display.ShowIntro"):app_main.index("buttons.Initialize")]
    assert "display.ShowIdle" not in boot_section
    assert "connectivity.TryKnownNetworks(settings, display, startup_language, false)" in boot_section
    assert "connectivity.StartSetupPortal(display, portal, startup_language, true)" in boot_section


def test_fortune_service_selects_grimoire_by_language():
    header = read(MAIN / "fortune_service.h")
    source = read(MAIN / "fortune_service.cpp")
    data = read(MAIN / "generated/fortune_data.h")
    app_main = read(MAIN / "app_main.cpp")

    for token in (
        "PickRandom(Language language)",
        "Count(Language language)",
        "FortuneTableFor",
        "kFortunes_en",
        "kFortunes_es",
        "kFortuneCount_en",
        "kFortuneCount_es",
        "Language::Spanish",
        "Language::Italian",
    ):
        assert token in header or token in source or token in data
    fortune_case = app_main[app_main.index("case AppEvent::GenerateFortune:"):app_main.index("case AppEvent::MicTest:")]
    assert "ParseLanguage(settings.language.c_str())" in fortune_case
    assert "fortunes.PickRandom(language)" in fortune_case


def test_readme_documents_product_setup_and_logo_requirements():
    # The README is now a public showcase (badges, flow diagram, hardware table)
    # rather than an internal setup doc — it must still document the core product,
    # the browser flashing flow, the setup hotspot, and the type/credits system.
    readme = read(README)

    for phrase in (
        "briciomanzia",
        "ESP32-C6",
        "e-paper",
        "Bisc8-XXXX",
        "192.168.4.1",
        "ESP Web Tools",
        "whisper-1",
        "OpenAI",
        "deep sleep",
        "Pixelify Sans",
    ):
        assert phrase in readme


def test_ai_handoff_doc_describes_runtime_boundaries_and_openai_status():
    # AI_HANDOFF now lives in notes/ (kept out of the published docs/ GitHub Pages site).
    # The public README is a showcase and intentionally does not link the internal handoff.
    handoff = read(AI_HANDOFF)

    for phrase in (
        "VoiceOracleService",
        "audio/transcriptions",
        "audio/speech",
        "spool://question.wav",
        "Bisc8-XXXX",
        "does not require a PIN",
        "reboot_required",
        "/api/reboot",
        "API responses mask stored secrets",
        "PWR triple click",
        "CONFIG_BISC8_EMAIL_RELAY_URL",
        "Run this before claiming completion",
    ):
        assert phrase in handoff
    # The online oracle is implemented and confirmed on hardware now; the handoff's
    # status banner must say so (it previously pinned the pre-oracle "not implemented
    # yet" / Responses-API era, which was stale and is no longer required here).
    assert "fully implemented" in handoff
    assert "chat/completions" in handoff


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
    # The README must prominently point at the public browser flasher (Pages site).
    assert "enuzzo.github.io/bisc8" in readme


def test_public_flash_prepare_rejects_symlink_escape_and_prints_hashes():
    prep = read(FLASH_PREP)

    for token in (
        "hashlib.sha256",
        "source.is_symlink()",
        "Refusing to copy symlinked artifact",
        "source.resolve()",
        "build_dir.resolve()",
        "Artifact escapes build directory",
        "sha256=",
    ):
        assert token in prep
