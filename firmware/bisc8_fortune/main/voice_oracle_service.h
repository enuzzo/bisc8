#pragma once

#include <esp_err.h>

#include <cstdint>
#include <string>

namespace bisc8 {

struct OpenAiSettings;  // app_config.h

struct OracleResponse {
    const char *detected_language;
    const char *transcript;
    const char *oracle_answer_full;
    const char *oracle_answer_screen;
    const char *tts_text;
    const char *stt_model;    // engine used per phase (for the email "made with" line)
    const char *brain_model;
    const char *tts_model;
    const char *voice;
};

class VoiceOracleService {
public:
    // Why the last ask failed, so the caller can show a friendly message plus a
    // precise error code the user can report back. Maps: NoKey=E02,
    // Transcribe=E03, NoSpeech=E04, Brain=E05 (E01 = recording, handled by the
    // caller before the oracle is even invoked).
    enum class OracleFailure { None, NoKey, Transcribe, NoSpeech, Brain };

    // Online voice flow: speech-to-text -> oracle answer -> text-to-speech,
    // leaving a WAV the caller can play back. Returns ESP_OK with `response`
    // filled and the spoken audio saved to the answer spool; an error otherwise
    // (the caller falls back to a local fortune / error screen). The const char*
    // fields in `response` point at internal storage valid until the next call.
    esp_err_t AskFromRecordedAudio(const char *wav_path, const OpenAiSettings &openai, OracleResponse *response);

    // Phase 1 of the flow: speech-to-text + the oracle's text answer. Fills the
    // text fields of `response` (and the engine/voice fields for the email) so the
    // caller can SHOW the answer immediately, before the slow TTS audio download.
    // The const char* fields point at internal storage valid until the next call.
    esp_err_t AskTextAnswer(const char *wav_path, const OpenAiSettings &openai, OracleResponse *response);

    // Phase 2: synthesize the spoken answer for the text produced by AskTextAnswer,
    // leaving a WAV in the answer spool. Returns ESP_OK iff fresh audio is ready
    // (a TTS failure is non-fatal: the caller still shows the screen answer).
    esp_err_t SpeakAnswer(const OpenAiSettings &openai);

    // Spool URI of the generated answer audio (valid after a successful ask).
    static const char *AnswerAudioPath();

    // True if the last ask produced fresh spoken audio in the answer spool.
    bool HasAnswerAudio() const { return answer_audio_ready_; }

    // Bytes actually written to the answer spool (the whole WAV, header
    // included). OpenAI streams the WAV with an "unknown length" placeholder in
    // its header, so playback must trust this count, not the header's size field.
    uint32_t AnswerAudioBytes() const { return answer_audio_bytes_; }

    // Reason the last ask failed (valid after AskFromRecordedAudio returns != OK).
    OracleFailure LastFailure() const { return last_failure_; }

private:
    esp_err_t Transcribe(const OpenAiSettings &openai);                                  // -> transcript_, detected_language_
    esp_err_t GenerateAnswer(const OpenAiSettings &openai, const std::string &device_language);  // -> answer_*/tts_/voice_
    esp_err_t Synthesize(const OpenAiSettings &openai);                                  // tts_text_ -> answer spool
    void FillResponse(OracleResponse *response);
    void Reset();

    std::string detected_language_;
    std::string transcript_;
    std::string answer_full_;
    std::string answer_screen_;
    std::string tts_text_;
    std::string voice_direction_;
    bool answer_audio_ready_ = false;
    uint32_t answer_audio_bytes_ = 0;
    OracleFailure last_failure_ = OracleFailure::None;
};

}  // namespace bisc8
