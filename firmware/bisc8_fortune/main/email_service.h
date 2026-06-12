#pragma once

#include <cstdint>

#include <esp_err.h>

namespace bisc8 {

struct EmailSettings;
struct OracleResponse;

class EmailService {
public:
    // Relays transcript + answer text to the user. Audio attachments are small
    // 4 kHz WAV review copies derived from the question spool and, when
    // answer_audio_bytes > 0, the answer spool. answer_audio_bytes is the true
    // byte count of the full local-playback WAV, used to bound the derived copy.
    esp_err_t SendOracleEmail(const EmailSettings &settings, const OracleResponse &response,
                              const char *audio_path, uint32_t answer_audio_bytes = 0);
};

}  // namespace bisc8
