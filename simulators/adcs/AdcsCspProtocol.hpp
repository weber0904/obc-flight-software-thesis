#ifndef OBC_SIMULATORS_ADCS_ADCSCSPPROTOCOL_HPP
#define OBC_SIMULATORS_ADCS_ADCSCSPPROTOCOL_HPP

#include <cstdint>

#include "simulators/adcs/AdcsTypes.hpp"

namespace OBC {
namespace ADCS {
namespace CSP {

static constexpr std::uint8_t VERSION = 1U;
static constexpr std::uint16_t DEFAULT_OBC_NODE_ID = 1U;
static constexpr std::uint16_t DEFAULT_ADCS_NODE_ID = 3U;

// libcsp reserves low management ports. ADCS-owned application services use a
// separate range from EPS to keep hosted subsystem traffic distinguishable.
enum class ServicePort : std::uint8_t {
    STATE = 20U,
    MODE = 21U,
    TARGET = 22U,
    CALIBRATE = 23U,
    RESET = 24U,
};

struct RequestHeader {
    std::uint8_t version;
    std::uint8_t service;
    std::uint16_t seq;
};

struct ReplyHeader {
    std::uint8_t version;
    std::uint8_t service;
    std::uint16_t seq;
    std::uint8_t result;
    std::uint8_t reserved[3];
};

struct ModeCommand {
    std::uint8_t mode;
    std::uint8_t reserved[3];
};

struct TargetCommand {
    double q0;
    double q1;
    double q2;
    double q3;
};

struct CalibrateCommand {
    std::uint8_t sensor_id;
    std::uint8_t reserved[3];
};

struct ResetCommand {
    std::uint8_t reserved[4];
};

union RequestPayload {
    ModeCommand mode;
    TargetCommand target;
    CalibrateCommand calibrate;
    ResetCommand reset;
};

struct Request {
    RequestHeader header;
    RequestPayload payload;
};

struct Reply {
    ReplyHeader header;
    StateData state;
};

inline Request makeBlankRequest(ServicePort service, std::uint16_t seq) {
    Request request = {};
    request.header.version = VERSION;
    request.header.service = static_cast<std::uint8_t>(service);
    request.header.seq = seq;
    return request;
}

inline Reply makeBlankReply(ServicePort service, std::uint16_t seq) {
    Reply reply = {};
    reply.header.version = VERSION;
    reply.header.service = static_cast<std::uint8_t>(service);
    reply.header.seq = seq;
    return reply;
}

}  // namespace CSP
}  // namespace ADCS
}  // namespace OBC

#endif
