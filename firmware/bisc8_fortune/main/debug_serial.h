#pragma once

#include <stdarg.h>

namespace bisc8 {

using StatusPrinter = void (*)();
using CommandHandler = bool (*)(const char *line);

class DebugSerial {
public:
    static void Start(StatusPrinter status_printer, CommandHandler command_handler = nullptr);
    static void SetVerbose(bool enabled);
    static bool Verbose();
    static void Log(const char *tag, const char *fmt, ...);
    static void LogAlways(const char *tag, const char *fmt, ...);

private:
    static void LogVa(bool force, const char *tag, const char *fmt, va_list args);
};

}  // namespace bisc8
