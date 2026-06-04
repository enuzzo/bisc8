#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bisc8 {

struct SoundAsset {
    const uint8_t *data;
    size_t bytes;
    const char *name;
};

extern const SoundAsset kSoundBoot;
extern const SoundAsset kSoundOracleButton;
extern const SoundAsset kSoundVoiceSubmit;
extern const SoundAsset kSoundShutdown;

}  // namespace bisc8
