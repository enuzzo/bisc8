#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>

#include <string>

namespace bisc8 {

constexpr size_t kMaxWifiCredentials = 8;
constexpr size_t kMaxScreenAnswerChars = 100;
constexpr uint32_t kWifiAttemptTimeoutMs = 8000;
constexpr uint32_t kVoiceRecordLimitMs = 15000;
constexpr uint32_t kConfigSchemaVersion = 2;

struct WifiCredential {
    std::string ssid;
    std::string password;
};

struct OpenAiSettings {
    std::string api_key;
    std::string transcription_model;
    std::string response_model;
    std::string speech_model;
    std::string voice;
};

struct EmailSettings {
    bool enabled = false;
    std::string recipient;
    std::string relay_url;
    std::string relay_token;
};

struct DeviceSettings {
    std::string language;
    OpenAiSettings openai;
    EmailSettings email;
    WifiCredential wifi[kMaxWifiCredentials];
    size_t wifi_count = 0;
};

OpenAiSettings DefaultOpenAiSettings();
EmailSettings DefaultEmailSettings();
std::string MaskSecret(const std::string &secret);

class ConfigStore {
public:
    esp_err_t Init();
    esp_err_t Load(DeviceSettings *settings);
    esp_err_t Save(const DeviceSettings &settings);
    esp_err_t Reset();
};

}  // namespace bisc8
