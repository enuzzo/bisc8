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
    const char *speaking_title;
    const char *speaking_body;
    const char *offline_title;
    const char *offline_body;
    const char *sleep_footer;
    const char *error_title;
};

const char *DefaultLanguage();
Language ParseLanguage(const char *code);
const LocalizedStrings &StringsFor(Language language);
const char *LocalizedValue(Language language, const char *key);

}  // namespace bisc8
