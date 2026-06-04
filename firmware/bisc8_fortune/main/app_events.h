#pragma once

#include <stdint.h>

namespace bisc8 {

enum class AppEvent : uint8_t {
    GenerateFortune,
    MicTest,
    StartVoiceRecording,
    FinishVoiceRecording,
    ForceWifiSetup,
    FullConfigReset,
    ShowStatus,
    Sleep,
};

}  // namespace bisc8
