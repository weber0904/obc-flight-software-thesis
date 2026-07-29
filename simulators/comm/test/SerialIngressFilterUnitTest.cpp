#include "simulators/comm/SerialIngressAcquisitionFilter.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

std::string asString(const std::vector<std::uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

void enableFilter() {
    ::setenv("COMM_NODE_STRIP_GATEWAY_PREAMBLE", "1", 1);
}

void testNoPreambleBytesPassThrough() {
    enableFilter();
    OBC::COMM::SerialIngressAcquisitionFilter filter;
    filter.onLinkOpened();

    const std::uint8_t payload[] = {0x5a, 0x5a, 0x5a, 0x5a, 0x00, 0x01};
    const std::vector<std::uint8_t> result = filter.filter(payload, sizeof(payload));
    assert(result.size() == sizeof(payload));
    assert(asString(result) == std::string(reinterpret_cast<const char*>(payload), sizeof(payload)));
    assert(!filter.active());
}

void testCompletePreambleLineDropped() {
    enableFilter();
    OBC::COMM::SerialIngressAcquisitionFilter filter;
    filter.onLinkOpened();

    const std::string input = "COMM-GATEWAY-PREAMBLE-0\nZZZZpayload";
    const std::vector<std::uint8_t> result =
        filter.filter(reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
    assert(asString(result) == "ZZZZpayload");
    assert(!filter.active());
}

void testSplitPreambleLineDroppedAcrossChunks() {
    enableFilter();
    OBC::COMM::SerialIngressAcquisitionFilter filter;
    filter.onLinkOpened();

    const std::string first = "COMM-GATEWAY-PREAM";
    std::vector<std::uint8_t> result =
        filter.filter(reinterpret_cast<const std::uint8_t*>(first.data()), first.size());
    assert(result.empty());
    assert(filter.active());

    const std::string second = "BLE-0\nCOMM-GATEWAY-PREAMBLE-1\nZZZZ";
    result = filter.filter(reinterpret_cast<const std::uint8_t*>(second.data()), second.size());
    assert(asString(result) == "ZZZZ");
    assert(!filter.active());
}

void testLinkCloseRearmsAcquisitionFilter() {
    enableFilter();
    OBC::COMM::SerialIngressAcquisitionFilter filter;
    filter.onLinkOpened();

    const std::uint8_t payload[] = {0x01, 0x02};
    std::vector<std::uint8_t> result = filter.filter(payload, sizeof(payload));
    assert(result.size() == sizeof(payload));
    assert(!filter.active());

    filter.onLinkClosed();
    filter.onLinkOpened();
    const std::string input = "COMM-GATEWAY-PREAMBLE-0\nAB";
    result = filter.filter(reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
    assert(asString(result) == "AB");
    assert(!filter.active());
}

}  // namespace

int main() {
    testNoPreambleBytesPassThrough();
    testCompletePreambleLineDropped();
    testSplitPreambleLineDroppedAcrossChunks();
    testLinkCloseRearmsAcquisitionFilter();
    return 0;
}
