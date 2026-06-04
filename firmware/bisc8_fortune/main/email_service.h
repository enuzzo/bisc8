#pragma once

#include <esp_err.h>

namespace bisc8 {

struct EmailSettings;
struct OracleResponse;

class EmailService {
public:
    esp_err_t SendOracleEmail(const EmailSettings &settings, const OracleResponse &response, const char *audio_path);
};

}  // namespace bisc8
