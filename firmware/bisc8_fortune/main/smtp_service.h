#pragma once

#include <esp_err.h>

namespace bisc8 {

struct SmtpSettings;
struct OracleResponse;

class SmtpService {
public:
    esp_err_t SendOracleEmail(const SmtpSettings &settings, const OracleResponse &response, const char *audio_path);
};

}  // namespace bisc8
