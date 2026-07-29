#include "simulators/adcs/AdcsTransport.hpp"

#include <cerrno>
#include <cstdlib>

namespace OBC {
namespace ADCS {

namespace {

std::uint16_t parseNodeEnv(const char* key, std::uint16_t fallback) {
    const char* const value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed == 0UL || parsed > 65535UL) {
        return fallback;
    }
    return static_cast<std::uint16_t>(parsed);
}

TransportStatus mapRuntimeStatus(::OBC::CSP::RuntimeStatus status) {
    switch (status) {
        case ::OBC::CSP::RuntimeStatus::OK:
            return TransportStatus::OK;
        case ::OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
            return TransportStatus::INVALID_REQUEST;
        case ::OBC::CSP::RuntimeStatus::TIMEOUT:
            return TransportStatus::TIMEOUT;
        case ::OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
        default:
            return TransportStatus::TRANSPORT_ERROR;
    }
}

}  // namespace

std::uint16_t defaultAdcsNodeIdFromEnvironment() {
    return parseNodeEnv("ADCS_CSP_NODE_ID", CSP::DEFAULT_ADCS_NODE_ID);
}

CspAdcsTransport::CspAdcsTransport(std::uint16_t targetNode,
                                   std::uint32_t timeoutMs,
                                   ::OBC::CSP::ICspRuntime& runtime)
    : m_targetNode(targetNode), m_timeoutMs(timeoutMs), m_seq(1U), m_runtime(runtime), m_runtimeReady(false) {}

TransportStatus CspAdcsTransport::getState(StateData& outState) {
    const CSP::Request request = CSP::makeBlankRequest(CSP::ServicePort::STATE, this->nextSeq_());
    return this->request_(request, CSP::ServicePort::STATE, outState);
}

TransportStatus CspAdcsTransport::setMode(std::uint8_t mode, StateData& outState) {
    CSP::Request request = CSP::makeBlankRequest(CSP::ServicePort::MODE, this->nextSeq_());
    request.payload.mode.mode = mode;
    return this->request_(request, CSP::ServicePort::MODE, outState);
}

TransportStatus CspAdcsTransport::setTarget(double q0, double q1, double q2, double q3, StateData& outState) {
    CSP::Request request = CSP::makeBlankRequest(CSP::ServicePort::TARGET, this->nextSeq_());
    request.payload.target.q0 = q0;
    request.payload.target.q1 = q1;
    request.payload.target.q2 = q2;
    request.payload.target.q3 = q3;
    return this->request_(request, CSP::ServicePort::TARGET, outState);
}

TransportStatus CspAdcsTransport::calibrate(std::uint8_t sensorId, StateData& outState) {
    CSP::Request request = CSP::makeBlankRequest(CSP::ServicePort::CALIBRATE, this->nextSeq_());
    request.payload.calibrate.sensor_id = sensorId;
    return this->request_(request, CSP::ServicePort::CALIBRATE, outState);
}

TransportStatus CspAdcsTransport::reset(StateData& outState) {
    const CSP::Request request = CSP::makeBlankRequest(CSP::ServicePort::RESET, this->nextSeq_());
    return this->request_(request, CSP::ServicePort::RESET, outState);
}

bool CspAdcsTransport::ensureRuntime_() {
    if (this->m_runtimeReady) {
        return true;
    }

    if (this->m_runtime.metrics().initialized) {
        this->m_runtimeReady = true;
        return true;
    }

    const ::OBC::CSP::RuntimeConfig config =
        ::OBC::CSP::runtimeConfigFromEnvironment(CSP::DEFAULT_OBC_NODE_ID, "OBCCSP");
    this->m_runtimeReady = this->m_runtime.init(config) == ::OBC::CSP::RuntimeStatus::OK;
    return this->m_runtimeReady;
}

std::uint16_t CspAdcsTransport::nextSeq_() {
    const std::uint16_t current = this->m_seq;
    this->m_seq = static_cast<std::uint16_t>(this->m_seq + 1U);
    if (this->m_seq == 0U) {
        this->m_seq = 1U;
    }
    return current;
}

TransportStatus CspAdcsTransport::request_(const CSP::Request& request,
                                           CSP::ServicePort expectedService,
                                           StateData& outState) {
    if (!this->ensureRuntime_()) {
        return TransportStatus::TRANSPORT_ERROR;
    }

    CSP::Reply reply = {};
    std::size_t replySize = 0U;
    const ::OBC::CSP::RuntimeStatus runtimeStatus =
        this->m_runtime.requestReply(this->m_targetNode,
                                     static_cast<std::uint8_t>(expectedService),
                                     &request,
                                     sizeof(request),
                                     &reply,
                                     sizeof(reply),
                                     replySize,
                                     this->m_timeoutMs);
    if (runtimeStatus != ::OBC::CSP::RuntimeStatus::OK) {
        return mapRuntimeStatus(runtimeStatus);
    }

    return decodeAdcsReply(request, expectedService, &reply, replySize, outState);
}

TransportStatus decodeAdcsReply(const CSP::Request& request,
                                CSP::ServicePort expectedService,
                                const void* replyData,
                                std::size_t replySize,
                                StateData& outState) {
    if (replyData == nullptr || replySize != sizeof(CSP::Reply)) {
        return TransportStatus::INVALID_RESPONSE;
    }

    const auto* reply = static_cast<const CSP::Reply*>(replyData);
    if (reply->header.version != CSP::VERSION || reply->header.service != static_cast<std::uint8_t>(expectedService) ||
        reply->header.seq != request.header.seq) {
        return TransportStatus::INVALID_RESPONSE;
    }

    const ResultCode result = static_cast<ResultCode>(reply->header.result);
    if (result == ResultCode::OK) {
        outState = reply->state;
        return TransportStatus::OK;
    }

    if (result == ResultCode::INVALID_REQUEST) {
        return TransportStatus::INVALID_REQUEST;
    }

    if (result == ResultCode::TIMEOUT) {
        return TransportStatus::TIMEOUT;
    }

    return TransportStatus::REMOTE_ERROR;
}

std::unique_ptr<IAdcsTransport> makeDefaultAdcsTransport(std::uint32_t timeoutMs) {
    return std::unique_ptr<IAdcsTransport>(new CspAdcsTransport(defaultAdcsNodeIdFromEnvironment(), timeoutMs));
}

std::unique_ptr<IAdcsTransport> makeDefaultAdcsTransport(::OBC::CSP::ICspRuntime& runtime, std::uint32_t timeoutMs) {
    return std::unique_ptr<IAdcsTransport>(
        new CspAdcsTransport(defaultAdcsNodeIdFromEnvironment(), timeoutMs, runtime));
}

}  // namespace ADCS
}  // namespace OBC
