#include "email_service.h"

#include "app_config.h"
#include "debug_serial.h"
#include "voice_oracle_service.h"

namespace bisc8 {

esp_err_t EmailService::SendOracleEmail(const EmailSettings &settings, const OracleResponse &response, const char *audio_path) {
    if (!settings.enabled || settings.recipient.empty() || settings.relay_url.empty()) {
        DebugSerial::LogAlways("[EMAIL]", "relay skipped; settings incomplete");
        return ESP_ERR_INVALID_STATE;
    }

    DebugSerial::LogAlways("[EMAIL]",
                           "relay HTTPS POST pending url=%s recipient=%s token=%s transcript=%s answer=%s",
                           settings.relay_url.c_str(),
                           MaskSecret(settings.recipient).c_str(),
                           settings.relay_token.empty() ? "missing" : "set",
                           response.transcript == nullptr || response.transcript[0] == '\0' ? "missing" : "set",
                           response.oracle_answer_full == nullptr || response.oracle_answer_full[0] == '\0' ? "missing" : "set");
    if (audio_path != nullptr && audio_path[0] != '\0') {
        DebugSerial::LogAlways("[EMAIL]", "attachment upload pending; relay will fall back to text-only if needed");
    }
    return ESP_ERR_NOT_FINISHED;
}

}  // namespace bisc8
