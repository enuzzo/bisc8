#pragma once

namespace bisc8 {

enum class Language {
    English,
    Spanish,
    Italian,
};

struct LocalizedStrings {
    const char *code;
    const char *boot_status;
    const char *idle_body;
    const char *idle_footer;
    const char *wifi_connecting_title;
    const char *wifi_setup_title;
    const char *wifi_setup_body;
    const char *wifi_setup_footer;
    const char *listening_title;
    const char *listening_body;
    const char *thinking_title;
    const char *thinking_body;
    const char *cooking_title;
    const char *cooking_body;
    const char *speaking_title;
    const char *speaking_body;
    const char *offline_title;
    const char *offline_body;
    const char *sleep_footer;
    const char *error_title;
    const char *intro_title;
    const char *intro_body;
    const char *intro_footer;
    const char *status_title;
    const char *status_online_body;
    const char *status_setup_body;
    const char *status_offline_body;
    const char *status_footer;
    const char *wifi_connecting_body;
    const char *wifi_connecting_footer;
    const char *voice_footer;
    const char *speaking_footer;
    const char *audio_unavailable_body;
    const char *recording_failed_body;
    const char *voice_oracle_unconfigured_body;
    const char *error_footer;
    const char *no_wifi_body;
    const char *low_battery_title;
    const char *low_battery_body;
    const char *first_run_title;
    const char *first_run_body;
    const char *low_power_title;
    const char *low_power_body;
    const char *wifi_setup_hint;
    const char *wifi_connect_failed_body;
    const char *intro_press;
    const char *voice_no_speech_body;
    const char *voice_error_generic_body;
    const char *status_connected_title;     // Wi-Fi status screen title when online
    const char *status_disconnected_title;  // ... and when not connected
};

const char *DefaultLanguage();
Language ParseLanguage(const char *code);
const LocalizedStrings &StringsFor(Language language);
const char *LocalizedValue(Language language, const char *key);

}  // namespace bisc8
