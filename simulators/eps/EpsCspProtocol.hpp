#ifndef OBC_SIMULATORS_EPS_EPSCSPPROTOCOL_HPP
#define OBC_SIMULATORS_EPS_EPSCSPPROTOCOL_HPP

#include <cstdint>

#include "simulators/eps/EpsTypes.hpp"

namespace OBC {
namespace EPS {
namespace CSP {

static constexpr std::uint8_t VERSION = 1U;
static constexpr std::uint16_t DEFAULT_OBC_NODE_ID = 1U;
static constexpr std::uint16_t DEFAULT_EPS_NODE_ID = 2U;

// libcsp reserves ports 0..3 for built-in management services, including ping.
enum class ServicePort : std::uint8_t {
    STATUS = 10U,
    PDU = 11U,
    CONFIG = 12U,
    RESET = 13U,
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

struct PduCommand {
    std::uint8_t channel;
    std::uint8_t enabled;
    std::uint8_t reserved[2];
};

struct HeaterCommand {
    std::uint8_t enabled;
    std::uint8_t reserved[3];
};

struct ResetCommand {
    std::uint8_t reserved[4];
};

union RequestPayload {
    PduCommand pdu;
    HeaterCommand heater;
    ResetCommand reset;
};

struct Request {
    RequestHeader header;
    RequestPayload payload;
};

struct Reply {
    ReplyHeader header;
    StatusData status;
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
}  // namespace EPS
}  // namespace OBC

#endif
