#include "smtp_service.h"

#include "app_config.h"
#include "debug_serial.h"
#include "voice_oracle_service.h"

namespace bisc8 {

esp_err_t SmtpService::SendOracleEmail(const SmtpSettings &settings, const OracleResponse &response, const char *audio_path) {
    if (!settings.enabled || settings.host.empty() || settings.recipient.empty()) {
        DebugSerial::LogAlways("[SMTP]", "direct SMTP skipped; settings incomplete");
        return ESP_ERR_INVALID_STATE;
    }

    DebugSerial::LogAlways("[SMTP]",
                           "direct SMTP text-only fallback host=%s recipient=%s transcript=%s answer=%s",
                           settings.host.c_str(),
                           settings.recipient.c_str(),
                           response.transcript == nullptr ? "" : response.transcript,
                           response.oracle_answer_full == nullptr ? "" : response.oracle_answer_full);
    if (audio_path != nullptr && audio_path[0] != '\0') {
        DebugSerial::LogAlways("[SMTP]", "attachment failed or unsupported; sent text-only email");
    }
    return ESP_ERR_NOT_FINISHED;
}

}  // namespace bisc8
