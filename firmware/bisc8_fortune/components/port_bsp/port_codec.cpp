#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_heap_caps.h>
#include "port_codec.h"
#include "codec_board.h"
#include "codec_init.h"
#include "esp_codec_dev.h"


static esp_codec_dev_handle_t playback = NULL;
static esp_codec_dev_handle_t record = NULL;
static bool i2s_initialized = false;

esp_err_t Codec_Init(const char *strName) {
  	if(i2s_initialized)
    return ESP_OK;
    set_codec_board_type(strName);
  	codec_init_cfg_t codec_cfg = {};
	codec_cfg.in_mode = CODEC_I2S_MODE_TDM;
	codec_cfg.out_mode = CODEC_I2S_MODE_TDM;
	codec_cfg.in_use_tdm = false;
	codec_cfg.reuse_dev = false;
  	int ret = init_codec(&codec_cfg);
    if (ret != 0) {
        return ESP_FAIL;
    }
    i2s_initialized = true;
	playback = get_playback_handle();
	record = get_record_handle();
    if (playback == NULL || record == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_codec_dev_handle_t Codec_SpeakerInit(void) {
    ESP_ERROR_CHECK(Codec_Init("S3_ePaper_1_54"));
  	return get_playback_handle();
}

esp_codec_dev_handle_t Codec_MicrophoneInit(void) {
    ESP_ERROR_CHECK(Codec_Init("S3_ePaper_1_54"));
    return get_record_handle();
}

esp_err_t Codec_StartInit() {
	esp_err_t err = Codec_Init("C6_ePaper_1_54");
    if (err != ESP_OK) {
        return err;
    }
	esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 16000;        
    fs.channel = 2;
    fs.bits_per_sample = 16;
    err = esp_codec_dev_open(playback, &fs);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_codec_dev_open(record, &fs);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_codec_dev_set_out_vol(playback, 100);
    if (err != ESP_OK) {
        return err;
    }
    // Mic capture gain. The ES8311 mic PGA tops out at 42 dB; the old 45.0 ran it
    // flat out, so this near-field pocket mic railed the ADC on normal speech ->
    // clipped audio -> speech-to-text heard gibberish (the ear-forgiving record/
    // playback loopback masked it). 24 dB keeps a strong but unclipped level for
    // close talk; [VOICEDIAG]/analyze_question_wav confirm clip% and dBFS, so this
    // can be nudged (18-30) once measured on-device.
    return esp_codec_dev_set_in_gain(record, 24.0);
}

esp_err_t Codec_PlaybackData(uint8_t *buffer,size_t bytes) {
	return esp_codec_dev_write(playback, buffer, bytes);
}

esp_err_t Codec_PlaybackOpen(int sample_rate, int channels) {
    if (playback == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_codec_dev_close(playback);
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = sample_rate;
    fs.channel = channels;
    fs.bits_per_sample = 16;
    esp_err_t err = esp_codec_dev_open(playback, &fs);
    if (err != ESP_OK) {
        return err;
    }
    return esp_codec_dev_set_out_vol(playback, 100);
}

esp_err_t Codec_RecordData(uint8_t *buffer,size_t bytes) {
	return esp_codec_dev_read(record, buffer, bytes);
}
