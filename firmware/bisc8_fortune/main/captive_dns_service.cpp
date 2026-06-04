#include "captive_dns_service.h"

#include <cstring>

#include <esp_err.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>

#include "debug_serial.h"

namespace bisc8 {
namespace {

constexpr uint16_t kDnsPort = 53;
constexpr int kDnsPacketBytes = 512;
constexpr int kDnsHeaderBytes = 12;
constexpr uint32_t kAnswerTtlSeconds = 60;
constexpr UBaseType_t kDnsTaskPriority = 3;
constexpr uint32_t kDnsTaskStackBytes = 3072;

uint16_t ReadBe16(const uint8_t *data) {
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

void WriteBe16(uint8_t *data, uint16_t value) {
    data[0] = static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value & 0xff);
}

void WriteBe32(uint8_t *data, uint32_t value) {
    data[0] = static_cast<uint8_t>((value >> 24) & 0xff);
    data[1] = static_cast<uint8_t>((value >> 16) & 0xff);
    data[2] = static_cast<uint8_t>((value >> 8) & 0xff);
    data[3] = static_cast<uint8_t>(value & 0xff);
}

}  // namespace

esp_err_t CaptiveDnsService::Start(uint32_t captive_ip) {
    if (running_) {
        DebugSerial::LogAlways("[DNS]", "captive DNS already running");
        return ESP_OK;
    }

    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socket_ < 0) {
        DebugSerial::LogAlways("[DNS]", "socket failed");
        return ESP_FAIL;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kDnsPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(socket_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        DebugSerial::LogAlways("[DNS]", "bind udp/53 failed");
        closesocket(socket_);
        socket_ = -1;
        return ESP_FAIL;
    }

    captive_ip_ = captive_ip;
    running_ = true;
    if (xTaskCreate(&CaptiveDnsService::TaskEntry, "bisc8_dns", kDnsTaskStackBytes, this, kDnsTaskPriority, &task_) != pdPASS) {
        DebugSerial::LogAlways("[DNS]", "task create failed");
        closesocket(socket_);
        socket_ = -1;
        running_ = false;
        return ESP_ERR_NO_MEM;
    }

    DebugSerial::LogAlways("[DNS]", "captive DNS started on udp/53");
    return ESP_OK;
}

void CaptiveDnsService::Stop() {
    running_ = false;
    if (socket_ >= 0) {
        shutdown(socket_, SHUT_RDWR);
        closesocket(socket_);
        socket_ = -1;
    }
}

bool CaptiveDnsService::Running() const {
    return running_;
}

void CaptiveDnsService::TaskEntry(void *arg) {
    static_cast<CaptiveDnsService *>(arg)->Run();
    vTaskDelete(nullptr);
}

void CaptiveDnsService::Run() {
    uint8_t query[kDnsPacketBytes] = {};
    uint8_t response[kDnsPacketBytes] = {};

    while (running_) {
        sockaddr_in source = {};
        socklen_t source_len = sizeof(source);
        const int received = recvfrom(socket_, query, sizeof(query), 0, reinterpret_cast<sockaddr *>(&source), &source_len);
        if (received <= 0) {
            if (running_) {
                DebugSerial::Log("[DNS]", "recv failed");
            }
            continue;
        }

        const int response_len = BuildResponse(query, received, response, sizeof(response));
        if (response_len <= 0) {
            continue;
        }
        sendto(socket_, response, response_len, 0, reinterpret_cast<sockaddr *>(&source), source_len);
    }

    task_ = nullptr;
    DebugSerial::LogAlways("[DNS]", "captive DNS stopped");
}

int CaptiveDnsService::BuildResponse(const uint8_t *query, int query_len, uint8_t *response, int response_len) const {
    if (query == nullptr || response == nullptr || query_len < kDnsHeaderBytes || response_len < kDnsPacketBytes) {
        return 0;
    }
    if (ReadBe16(query + 4) == 0) {
        return 0;
    }

    int question_end = kDnsHeaderBytes;
    while (question_end < query_len && query[question_end] != 0) {
        const uint8_t label_len = query[question_end];
        if ((label_len & 0xc0) != 0 || label_len > 63) {
            return 0;
        }
        question_end += 1 + label_len;
    }
    question_end += 1;
    if (question_end + 4 > query_len) {
        return 0;
    }

    const int question_len = question_end + 4 - kDnsHeaderBytes;
    const int total_len = kDnsHeaderBytes + question_len + 16;
    if (total_len > response_len) {
        return 0;
    }

    std::memset(response, 0, total_len);
    std::memcpy(response, query, 2);
    WriteBe16(response + 2, 0x8180);
    WriteBe16(response + 4, 1);
    WriteBe16(response + 6, 1);
    std::memcpy(response + kDnsHeaderBytes, query + kDnsHeaderBytes, question_len);

    uint8_t *answer = response + kDnsHeaderBytes + question_len;
    WriteBe16(answer, 0xc00c);
    WriteBe16(answer + 2, 1);
    WriteBe16(answer + 4, 1);
    WriteBe32(answer + 6, kAnswerTtlSeconds);
    WriteBe16(answer + 10, 4);
    WriteBe32(answer + 12, captive_ip_);
    return total_len;
}

}  // namespace bisc8
