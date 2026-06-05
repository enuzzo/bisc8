#ifndef PORT_CODEC_H
#define PORT_CODEC_H

#include "esp_err.h"


#ifdef __cplusplus
extern "C" {
#endif

esp_err_t Codec_Init(const char *strName);
esp_err_t Codec_StartInit();

esp_err_t Codec_PlaybackData(uint8_t *buffer,size_t bytes);
esp_err_t Codec_RecordData(uint8_t *buffer,size_t bytes);

// Reopen the playback path at a different sample rate / channel count (e.g. to
// play 24 kHz TTS audio), then call again to restore 16 kHz stereo afterwards.
esp_err_t Codec_PlaybackOpen(int sample_rate, int channels);

#ifdef __cplusplus
}
#endif



#endif
