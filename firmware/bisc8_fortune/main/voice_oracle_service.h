#pragma once

#include <esp_err.h>

namespace bisc8 {

struct OracleResponse {
    const char *detected_language;
    const char *transcript;
    const char *oracle_answer_full;
    const char *oracle_answer_screen;
    const char *tts_text;
};

class VoiceOracleService {
public:
    esp_err_t AskFromRecordedAudio(const char *wav_path, OracleResponse *response);
};

}  // namespace bisc8
