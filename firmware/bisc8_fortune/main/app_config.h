#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>

#include <string>

namespace bisc8 {

constexpr size_t kMaxWifiCredentials = 8;
constexpr size_t kMaxScreenAnswerChars = 55;  // tiny 200x200 e-paper; keep it short
constexpr uint32_t kWifiAttemptTimeoutMs = 10000;
constexpr uint32_t kVoiceRecordLimitMs = 15000;
constexpr uint32_t kConfigSchemaVersion = 2;

// Generated answer audio lives high in the 0x4f0000-byte spool partition, well
// past the question region (first ~512 KB), so STT input and TTS output never
// collide. gpt-4o-mini-tts PCM can exceed 1 MB for normal oracle answers, so
// reserve from 0x200000 through the end of the partition and wrap it in a local
// WAV header after download.
constexpr uint32_t kVoiceAnswerSpoolOffset = 0x200000;        // 2 MB into spool
constexpr uint32_t kVoiceAnswerSpoolMaxBytes = 0x2f0000;      // up to ~3 MB of WAV

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
    std::string reasoning_effort;  // minimal|low|medium|high; empty = omit (non-reasoning models)
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
