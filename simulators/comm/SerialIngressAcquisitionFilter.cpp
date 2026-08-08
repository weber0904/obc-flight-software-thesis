#include "simulators/comm/SerialIngressAcquisitionFilter.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace OBC {
namespace COMM {

namespace {

constexpr const char* kGatewayPreamblePrefix = "COMM-GATEWAY-PREAMBLE-";
constexpr std::size_t kMaxPendingBytes = 512U;

bool envEnabled(const char* key, bool fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return std::strcmp(value, "0") != 0;
}

}  // namespace

SerialIngressAcquisitionFilter::SerialIngressAcquisitionFilter()
    : m_enabled(envEnabled("COMM_NODE_STRIP_GATEWAY_PREAMBLE", true)),
      m_active(false),
      m_pending() {}

void SerialIngressAcquisitionFilter::onLinkOpened() {
    this->m_pending.clear();
    this->m_active = this->m_enabled;
}

void SerialIngressAcquisitionFilter::onLinkClosed() {
    this->m_pending.clear();
    this->m_active = false;
}

std::vector<std::uint8_t> SerialIngressAcquisitionFilter::filter(const std::uint8_t* data, std::size_t size) {
    std::vector<std::uint8_t> output;
    if (data == nullptr || size == 0U) {
        return output;
    }
    if (!this->m_enabled || !this->m_active) {
        output.assign(data, data + size);
        return output;
    }

    this->m_pending.append(reinterpret_cast<const char*>(data), size);
    if (this->m_pending.size() > kMaxPendingBytes) {
        output.assign(this->m_pending.begin(), this->m_pending.end());
        this->m_pending.clear();
        this->m_active = false;
        return output;
    }

    const std::string prefix(kGatewayPreamblePrefix);
    while (!this->m_pending.empty()) {
        if (this->m_pending.compare(0U, prefix.size(), prefix) == 0) {
            const std::string::size_type newline = this->m_pending.find('\n');
            if (newline == std::string::npos) {
                return output;
            }
            this->m_pending.erase(0U, newline + 1U);
            continue;
        }

        if (prefix.compare(0U, this->m_pending.size(), this->m_pending) == 0) {
            return output;
        }

        output.assign(this->m_pending.begin(), this->m_pending.end());
        this->m_pending.clear();
        this->m_active = false;
        return output;
    }

    return output;
}

bool SerialIngressAcquisitionFilter::enabled() const {
    return this->m_enabled;
}

bool SerialIngressAcquisitionFilter::active() const {
    return this->m_active;
}

}  // namespace COMM
}  // namespace OBC
