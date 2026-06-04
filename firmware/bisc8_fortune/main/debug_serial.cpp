#include "debug_serial.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/usb_serial_jtag.h>
#include <driver/usb_serial_jtag_vfs.h>
#include <esp_err.h>

#include "epaper_config.h"
#include "port_display.h"

namespace bisc8 {

namespace {

bool g_verbose = CONFIG_BISC8_DEBUG_LOGS;
StatusPrinter g_status_printer = nullptr;
CommandHandler g_command_handler = nullptr;

void configure_usb_console() {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t err = usb_serial_jtag_driver_install(&config);
    if (err == ESP_OK) {
        usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CRLF);
        usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
        usb_serial_jtag_vfs_use_driver();
        printf("[SERIAL] USB-Serial/JTAG console driver ready\n");
        fflush(stdout);
    } else {
        printf("[SERIAL] USB-Serial/JTAG driver install failed: %s\n", esp_err_to_name(err));
        fflush(stdout);
    }
#endif
}

void trim_line(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
        line[--len] = '\0';
    }
    char *start = line;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

void dump_snapshot() {
    const size_t len = EPD_FrameBufferSize();
    uint8_t *frame = static_cast<uint8_t *>(malloc(len));
    if (frame == nullptr) {
        printf("[SNAP] ERROR alloc %u\n", static_cast<unsigned>(len));
        fflush(stdout);
        return;
    }
    if (!EPD_CopyFrameBuffer(frame, len)) {
        printf("[SNAP] ERROR framebuffer-unavailable\n");
        free(frame);
        fflush(stdout);
        return;
    }

    printf("[SNAP] BEGIN %d %d %u\n", EPD_WIDTH, EPD_HEIGHT, static_cast<unsigned>(len));
    for (size_t i = 0; i < len; i++) {
        printf("%02X", frame[i]);
        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
    if (len % 32 != 0) {
        printf("\n");
    }
    printf("[SNAP] END\n");
    fflush(stdout);
    free(frame);
}

void command_task(void *) {
    char line[96];
    for (;;) {
        if (fgets(line, sizeof(line), stdin) == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        trim_line(line);
        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "DEBUG 1") == 0) {
            DebugSerial::SetVerbose(true);
            DebugSerial::LogAlways("[DEBUG]", "verbose logs enabled");
        } else if (strcmp(line, "DEBUG 0") == 0) {
            DebugSerial::SetVerbose(false);
            DebugSerial::LogAlways("[DEBUG]", "verbose logs disabled");
        } else if (strcmp(line, "STATUS") == 0) {
            if (g_status_printer != nullptr) {
                g_status_printer();
            } else {
                DebugSerial::LogAlways("[STATUS]", "no status printer registered");
            }
        } else if (strcmp(line, "SNAP") == 0) {
            dump_snapshot();
        } else if (strcmp(line, "HELP") == 0) {
            DebugSerial::LogAlways("[SERIAL]", "commands: DEBUG 0, DEBUG 1, STATUS, SNAP, FORTUNE, MIC, VOICE START, VOICE STOP, WIFI SETUP, WIFI RESET, CONFIG RESET, HELP");
        } else if (g_command_handler != nullptr && g_command_handler(line)) {
            continue;
        } else {
            DebugSerial::LogAlways("[SERIAL]", "unknown command: %s", line);
        }
    }
}

}  // namespace

void DebugSerial::Start(StatusPrinter status_printer, CommandHandler command_handler) {
    g_status_printer = status_printer;
    g_command_handler = command_handler;
    configure_usb_console();
    xTaskCreatePinnedToCore(command_task, "debug_serial", 4096, nullptr, 3, nullptr, 0);
    LogAlways("[SERIAL]", "ready; commands: DEBUG 0, DEBUG 1, STATUS, SNAP, FORTUNE, MIC, WIFI SETUP, WIFI RESET, CONFIG RESET, HELP");
}

void DebugSerial::SetVerbose(bool enabled) {
    g_verbose = enabled;
}

bool DebugSerial::Verbose() {
    return g_verbose;
}

void DebugSerial::Log(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogVa(false, tag, fmt, args);
    va_end(args);
}

void DebugSerial::LogAlways(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogVa(true, tag, fmt, args);
    va_end(args);
}

void DebugSerial::LogVa(bool force, const char *tag, const char *fmt, va_list args) {
    if (!force && !g_verbose) {
        return;
    }
    printf("%s ", tag);
    vprintf(fmt, args);
    printf("\n");
    fflush(stdout);
}

}  // namespace bisc8
