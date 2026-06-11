#include "app_config.h"

#include <algorithm>

#include <esp_check.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "sdkconfig.h"

namespace bisc8 {
namespace {

constexpr const char *kConfigNamespace = "bisc8";
constexpr const char *kSchemaKey = "schema";
constexpr const char *kLanguageKey = "language";
constexpr const char *kWifiCountKey = "wifi_count";
constexpr const char *kOpenAiKeyKey = "openai_key";
constexpr const char *kOpenAiTranscriptionModelKey = "openai_stt";
constexpr const char *kOpenAiResponseModelKey = "openai_resp";
constexpr const char *kOpenAiSpeechModelKey = "openai_tts";
constexpr const char *kOpenAiVoiceKey = "openai_voice";
constexpr const char *kOpenAiReasoningKey = "openai_reason";  // <=15 chars for NVS
constexpr const char *kEmailEnabledKey = "email_enabled";
constexpr const char *kEmailRecipientKey = "email_recipient";
constexpr const char *kEmailRelayUrlKey = "email_relay_url";
// NVS keys must stay <= 15 chars (NVS_KEY_NAME_MAX_SIZE - 1), otherwise
// nvs_set_str/nvs_get_str fail with ESP_ERR_NVS_KEY_TOO_LONG. "email_relay_token"
// was 17 chars, which aborted the whole Save()/Load() transaction.
constexpr const char *kEmailRelayTokenKey = "email_relaytok";
constexpr const char *kLegacyEmailRecipientKey = "smtp_recipient";

bool IsSupportedOpenAiVoice(const std::string &voice) {
    return voice == "alloy" || voice == "ash" || voice == "ballad" || voice == "coral" ||
           voice == "echo" || voice == "fable" || voice == "nova" || voice == "onyx" ||
           voice == "sage" || voice == "shimmer" || voice == "verse" || voice == "marin" ||
           voice == "cedar";
}

bool IsRealtimeSpeechModel(const std::string &model) {
    return model.rfind("gpt-realtime", 0) == 0;
}

void ApplyDefaults(DeviceSettings *settings) {
    settings->language = "en";
    settings->openai = DefaultOpenAiSettings();
    settings->email = DefaultEmailSettings();
    settings->wifi_count = 0;
    for (size_t i = 0; i < kMaxWifiCredentials; ++i) {
        settings->wifi[i] = WifiCredential{};
    }
}

std::string IndexedKey(const char *prefix, size_t index) {
    return std::string(prefix) + std::to_string(index);
}

esp_err_t GetString(nvs_handle_t handle, const char *key, std::string *value) {
    size_t length = 0;
    esp_err_t err = nvs_get_str(handle, key, nullptr, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    std::string buffer(length, '\0');
    err = nvs_get_str(handle, key, buffer.data(), &length);
    if (err != ESP_OK) {
        return err;
    }
    if (!buffer.empty() && buffer.back() == '\0') {
        buffer.pop_back();
    }
    *value = buffer;
    return ESP_OK;
}

esp_err_t SetString(nvs_handle_t handle, const char *key, const std::string &value) {
    return nvs_set_str(handle, key, value.c_str());
}

esp_err_t LoadWifi(nvs_handle_t handle, DeviceSettings *settings) {
    uint32_t stored_count = 0;
    esp_err_t err = nvs_get_u32(handle, kWifiCountKey, &stored_count);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    settings->wifi_count = std::min(static_cast<size_t>(stored_count), kMaxWifiCredentials);
    for (size_t i = 0; i < settings->wifi_count; ++i) {
        std::string ssid_key = IndexedKey("wifi_ssid_", i);
        std::string password_key = IndexedKey("wifi_pass_", i);
        err = GetString(handle, ssid_key.c_str(), &settings->wifi[i].ssid);
        if (err != ESP_OK) {
            return err;
        }
        err = GetString(handle, password_key.c_str(), &settings->wifi[i].password);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t SaveWifi(nvs_handle_t handle, const DeviceSettings &settings) {
    const size_t count = std::min(settings.wifi_count, kMaxWifiCredentials);
    esp_err_t err = nvs_set_u32(handle, kWifiCountKey, static_cast<uint32_t>(count));
    if (err != ESP_OK) {
        return err;
    }
    for (size_t i = 0; i < kMaxWifiCredentials; ++i) {
        std::string ssid_key = IndexedKey("wifi_ssid_", i);
        std::string password_key = IndexedKey("wifi_pass_", i);
        if (i < count) {
            err = SetString(handle, ssid_key.c_str(), settings.wifi[i].ssid);
            if (err != ESP_OK) {
                return err;
            }
            err = SetString(handle, password_key.c_str(), settings.wifi[i].password);
            if (err != ESP_OK) {
                return err;
            }
        } else {
            nvs_erase_key(handle, ssid_key.c_str());
            nvs_erase_key(handle, password_key.c_str());
        }
    }
    return ESP_OK;
}

}  // namespace

OpenAiSettings DefaultOpenAiSettings() {
    OpenAiSettings settings;
    settings.transcription_model = "whisper-1";       // request-based STT fallback path
    settings.response_model = "gpt-5.4-mini";          // simple oracle query model
    settings.speech_model = "gpt-4o-mini-tts";         // Speech API TTS with expressive instructions
    settings.voice = "ash";
    settings.reasoning_effort = "";  // off by default; set per reasoning-capable model in the portal
    return settings;
}

EmailSettings DefaultEmailSettings() {
    EmailSettings settings;
#ifdef CONFIG_BISC8_EMAIL_RELAY_URL
    settings.relay_url = CONFIG_BISC8_EMAIL_RELAY_URL;
#endif
#ifdef CONFIG_BISC8_EMAIL_RELAY_TOKEN
    settings.relay_token = CONFIG_BISC8_EMAIL_RELAY_TOKEN;
#endif
    return settings;
}

std::string MaskSecret(const std::string &secret) {
    if (secret.empty()) {
        return "";
    }
    if (secret.size() <= 6) {
        return "***";
    }
    return secret.substr(0, 3) + "***" + secret.substr(secret.size() - 3);
}

esp_err_t ConfigStore::Init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), "CONFIG", "erase NVS after init failure");
        err = nvs_flash_init();
    }
    return err;
}

namespace {
// One-time self-heal for saved voice settings. Runs on every Load; once a
// config is clean it's a no-op. The caller persists the result, so a device with
// stale Realtime defaults upgrades itself the first time it boots this firmware.
bool MigrateOpenAiSettings(DeviceSettings *settings, bool migrate_realtime_defaults) {
    const OpenAiSettings defaults = DefaultOpenAiSettings();
    bool changed = false;
    bool migrated_realtime_speech = false;

    if (settings->openai.speech_model.empty() ||
        (migrate_realtime_defaults && IsRealtimeSpeechModel(settings->openai.speech_model))) {
        settings->openai.speech_model = defaults.speech_model;
        migrated_realtime_speech = true;
        changed = true;
    }
    if (settings->openai.voice.empty() || !IsSupportedOpenAiVoice(settings->openai.voice) ||
        (migrated_realtime_speech && settings->openai.voice == "cedar")) {
        settings->openai.voice = defaults.voice;
        changed = true;
    }
    return changed;
}
}  // namespace

esp_err_t ConfigStore::Load(DeviceSettings *settings) {
    if (settings == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ApplyDefaults(settings);

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kConfigNamespace, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    uint32_t schema = 0;
    err = nvs_get_u32(handle, kSchemaKey, &schema);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    err = GetString(handle, kLanguageKey, &settings->language);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiKeyKey, &settings->openai.api_key);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiTranscriptionModelKey, &settings->openai.transcription_model);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiResponseModelKey, &settings->openai.response_model);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiSpeechModelKey, &settings->openai.speech_model);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiVoiceKey, &settings->openai.voice);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kOpenAiReasoningKey, &settings->openai.reasoning_effort);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    uint8_t email_enabled = 0;
    if (nvs_get_u8(handle, kEmailEnabledKey, &email_enabled) == ESP_OK) {
        settings->email.enabled = email_enabled != 0;
    }
    err = GetString(handle, kEmailRecipientKey, &settings->email.recipient);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    if (settings->email.recipient.empty()) {
        err = GetString(handle, kLegacyEmailRecipientKey, &settings->email.recipient);
        if (err != ESP_OK) {
            nvs_close(handle);
            return err;
        }
    }
    err = GetString(handle, kEmailRelayUrlKey, &settings->email.relay_url);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = GetString(handle, kEmailRelayTokenKey, &settings->email.relay_token);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    err = LoadWifi(handle, settings);
    nvs_close(handle);
    if (err == ESP_OK && MigrateOpenAiSettings(settings, true)) {
        printf("[CONFIG] healed OpenAI model(s)/voice -> %s / %s / %s (voice %s)\n",
               settings->openai.transcription_model.c_str(),
               settings->openai.response_model.c_str(),
               settings->openai.speech_model.c_str(),
               settings->openai.voice.c_str());
        Save(*settings);  // persist the upgrade so it only happens once
    }
    return err;
}

esp_err_t ConfigStore::Save(const DeviceSettings &settings_in) {
    // Sanitize before persisting so stale/unsupported voice settings cannot
    // reach NVS, no matter the source: portal POST, oracle flow, or boot self-heal.
    DeviceSettings settings = settings_in;
    if (MigrateOpenAiSettings(&settings, false)) {
        printf("[CONFIG] refused to persist unsupported model/voice; using %s / %s / %s (voice %s)\n",
               settings.openai.transcription_model.c_str(),
               settings.openai.response_model.c_str(),
               settings.openai.speech_model.c_str(),
               settings.openai.voice.c_str());
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kConfigNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u32(handle, kSchemaKey, kConfigSchemaVersion);
    if (err == ESP_OK) {
        err = SetString(handle, kLanguageKey, settings.language);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiKeyKey, settings.openai.api_key);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiTranscriptionModelKey, settings.openai.transcription_model);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiResponseModelKey, settings.openai.response_model);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiSpeechModelKey, settings.openai.speech_model);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiVoiceKey, settings.openai.voice);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kOpenAiReasoningKey, settings.openai.reasoning_effort);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, kEmailEnabledKey, settings.email.enabled ? 1 : 0);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kEmailRecipientKey, settings.email.recipient);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kEmailRelayUrlKey, settings.email.relay_url);
    }
    if (err == ESP_OK) {
        err = SetString(handle, kEmailRelayTokenKey, settings.email.relay_token);
    }
    if (err == ESP_OK) {
        err = SaveWifi(handle, settings);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t ConfigStore::Reset() {
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kConfigNamespace, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

}  // namespace bisc8
