#include "voice_oracle_service.h"

#include "app_config.h"
#include "debug_serial.h"

namespace bisc8 {

namespace {

constexpr const char *kTranscriptionPath = "audio/transcriptions";
constexpr const char *kResponsesPath = "responses";
constexpr const char *kSpeechPath = "audio/speech";

constexpr const char *kOracleJsonContract =
    "{"
    "\"detected_language\":\"<BCP-47 language>\","
    "\"transcript\":\"<user question>\","
    "\"oracle_answer_full\":\"<contextual oracle answer>\","
    "\"oracle_answer_screen\":\"<=100 characters\","
    "\"tts_text\":\"<spoken answer>\""
    "}";

}  // namespace

esp_err_t VoiceOracleService::AskFromRecordedAudio(const char *wav_path, OracleResponse *response) {
    DebugSerial::LogAlways("[ORACLE]",
                           "voice flow wav=%s stt=%s text=%s tts=%s contract=%s max_screen=%u",
                           wav_path == nullptr ? "(null)" : wav_path,
                           kTranscriptionPath,
                           kResponsesPath,
                           kSpeechPath,
                           kOracleJsonContract,
                           static_cast<unsigned>(kMaxScreenAnswerChars));
    if (response != nullptr) {
        response->detected_language = "en";
        response->transcript = "";
        response->oracle_answer_full = "";
        response->oracle_answer_screen = "";
        response->tts_text = "";
    }
    return ESP_ERR_NOT_FINISHED;
}

}  // namespace bisc8
