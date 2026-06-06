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
    // Force a specific device screen (serial "SCREEN ..." command) so every
    // state can be reached and snapshot-validated on the bench.
    PreviewNoWifi,
    PreviewLowBattery,
    PreviewFirstRun,
    PreviewSpeaking,
    PreviewLowPower,
    PreviewConnectFailed,
    PreviewError,
    PreviewListening,
    PreviewThinking,
    PreviewWifiSetup,
};

}  // namespace bisc8
