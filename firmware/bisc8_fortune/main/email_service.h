#pragma once

#include <cstdint>

#include <esp_err.h>

namespace bisc8 {

struct EmailSettings;
struct OracleResponse;

class EmailService {
public:
    // Relays transcript + answer text to the user, attaching the original
    // question recording (question.wav, read from spool offset 0) and, when
    // answer_audio_bytes > 0, the generated answer audio (answer.wav, read from
    // the answer spool region). answer_audio_bytes is the true byte count of the
    // answer WAV, so the relayed file gets a repaired, playable length header.
    esp_err_t SendOracleEmail(const EmailSettings &settings, const OracleResponse &response,
                              const char *audio_path, uint32_t answer_audio_bytes = 0);
};

}  // namespace bisc8
