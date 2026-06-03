#pragma once

#include <stdint.h>

namespace bisc8 {

enum class AppEvent : uint8_t {
    GenerateFortune,
    MicTest,
    Sleep,
};

}  // namespace bisc8
