#include "OBC/TopCcsds/PayloadCspService.hpp"

#include <cstring>
#include <iostream>
#include <memory>

#include "OBC/TopCcsds/PayloadCspGateway.hpp"
#include "OBC/TopCcsds/PayloadCspProtocol.hpp"

extern "C" {
#include <csp/csp.h>
#include <csp/csp_buffer.h>
}

namespace OBCApp {

namespace {

constexpr std::array<PayloadCSP::ServicePort, 3> PAYLOAD_SERVICE_PORTS = {
    PayloadCSP::ServicePort::STATUS,
    PayloadCSP::ServicePort::CAPABILITIES,
    PayloadCSP::ServicePort::LAST_CAPTURE_METADATA,
};

template <typename TReply>
void sendReply(const csp_packet_t* request, const TReply& reply) {
    static_assert(sizeof(TReply) <= CSP_BUFFER_SIZE, "Payload CSP reply exceeds CSP packet buffer capacity");
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }
    if (sizeof(reply) > sizeof(packet->data)) {
        csp_buffer_free(packet);
        return;
    }
    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_sendto_reply(request, packet, CSP_O_SAME);
}

template <typename TReply>
void sendErrorReply(const csp_packet_t* request,
                    PayloadCSP::ServicePort service,
                    std::uint16_t seq,
                    PayloadCSP::ResultCode result) {
    TReply reply = {};
    reply.header = PayloadCSP::makeReplyHeader(service, seq, result);
    reply.header.byteCount = static_cast<std::uint16_t>(sizeof(reply) - sizeof(reply.header));
    sendReply(request, reply);
}

}  // namespace

PayloadCspService::PayloadCspService() : m_controller(nullptr), m_running(false), m_worker(), m_boundSockets{} {}

PayloadCspService::~PayloadCspService() {
    this->stop();
}

void PayloadCspService::configure(OBC::PayloadOpsController* controller) {
    this->m_controller = controller;
}

bool PayloadCspService::start() {
    if (this->m_controller == nullptr) {
        std::cerr << "PayloadCspService start rejected: controller not configured\n";
        return false;
    }
    if (this->m_worker.joinable()) {
        return true;
    }

    if (!this->bindSockets_()) {
        return false;
    }
    this->m_running.store(true);
    this->m_worker = std::thread([this]() { this->run_(); });
    return true;
}

void PayloadCspService::stop() {
    this->m_running.store(false);
    if (this->m_worker.joinable()) {
        this->m_worker.join();
    }
}

bool PayloadCspService::bindSockets_() {
    for (std::size_t index = 0; index < this->m_boundSockets.size(); ++index) {
        const auto port = static_cast<std::uint8_t>(PAYLOAD_SERVICE_PORTS[index]);
        auto socket = std::make_unique<csp_socket_t>();
        *socket = {};
        socket->opts = CSP_SO_CONN_LESS;
        const int bindStatus = csp_bind(socket.get(), port);
        const int listenStatus = bindStatus == CSP_ERR_NONE ? csp_listen(socket.get(), 2) : bindStatus;
        if (bindStatus != CSP_ERR_NONE || listenStatus != CSP_ERR_NONE) {
            std::cerr << "PayloadCspService failed to bind/listen on port " << static_cast<unsigned>(port)
                      << ": bindStatus=" << bindStatus << " listenStatus=" << listenStatus << "\n";
            for (csp_socket_t* bound : this->m_boundSockets) {
                if (bound != nullptr) {
                    csp_socket_close(bound);
                    delete bound;
                }
            }
            this->m_boundSockets.fill(nullptr);
            return false;
        }
        this->m_boundSockets[index] = socket.release();
    }
    return true;
}

void PayloadCspService::run_() {
    while (this->m_running.load()) {
        bool handledPacket = false;
        for (std::size_t index = 0; index < this->m_boundSockets.size(); ++index) {
            csp_socket_t* const socket = this->m_boundSockets[index];
            if (socket == nullptr) {
                continue;
            }
            const unsigned timeoutMs = (index + 1U == this->m_boundSockets.size()) ? 50U : 0U;
            csp_packet_t* const packet = csp_recvfrom(socket, timeoutMs);
            if (packet == nullptr) {
                continue;
            }
            handledPacket = true;
            this->handlePacket_(packet);
        }
        if (!handledPacket) {
            std::this_thread::yield();
        }
    }

    for (csp_socket_t* socket : this->m_boundSockets) {
        if (socket != nullptr) {
            csp_socket_close(socket);
            delete socket;
        }
    }
    this->m_boundSockets.fill(nullptr);
}

void PayloadCspService::handlePacket_(csp_packet_t* packet) {
    const int destinationPort = packet->id.dport;
    const auto servicePort = static_cast<PayloadCSP::ServicePort>(destinationPort);
    if (destinationPort < static_cast<int>(PayloadCSP::ServicePort::STATUS) ||
        destinationPort > static_cast<int>(PayloadCSP::ServicePort::LAST_CAPTURE_METADATA)) {
        csp_service_handler(packet);
        return;
    }

    PayloadCSP::BasicRequest request = {};
    if (packet->length != sizeof(request)) {
        PayloadCSP::RequestHeader header = {};
        const std::uint16_t seq = packet->length >= sizeof(header)
                                      ? (std::memcpy(&header, packet->data, sizeof(header)), header.seq)
                                      : 0U;
        switch (servicePort) {
            case PayloadCSP::ServicePort::STATUS:
                sendErrorReply<PayloadCSP::StatusReply>(packet, servicePort, seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
            case PayloadCSP::ServicePort::CAPABILITIES:
                sendErrorReply<PayloadCSP::CapabilitiesReply>(
                    packet, servicePort, seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
            case PayloadCSP::ServicePort::LAST_CAPTURE_METADATA:
                sendErrorReply<PayloadCSP::MetadataReply>(
                    packet, servicePort, seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
        }
        csp_buffer_free(packet);
        return;
    }

    std::memcpy(&request, packet->data, sizeof(request));

    if (request.header.version != PayloadCSP::VERSION ||
        request.header.service != static_cast<std::uint8_t>(destinationPort)) {
        switch (servicePort) {
            case PayloadCSP::ServicePort::STATUS:
                sendErrorReply<PayloadCSP::StatusReply>(
                    packet, servicePort, request.header.seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
            case PayloadCSP::ServicePort::CAPABILITIES:
                sendErrorReply<PayloadCSP::CapabilitiesReply>(
                    packet, servicePort, request.header.seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
            case PayloadCSP::ServicePort::LAST_CAPTURE_METADATA:
                sendErrorReply<PayloadCSP::MetadataReply>(
                    packet, servicePort, request.header.seq, PayloadCSP::ResultCode::INVALID_REQUEST);
                break;
        }
        csp_buffer_free(packet);
        return;
    }

    switch (servicePort) {
        case PayloadCSP::ServicePort::STATUS: {
            OBC::PayloadStatusSnapshot snapshot = {};
            if (!this->m_controller->getStatusSnapshotForRuntime(snapshot)) {
                sendErrorReply<PayloadCSP::StatusReply>(
                    packet, PayloadCSP::ServicePort::STATUS, request.header.seq, PayloadCSP::ResultCode::NOT_CONFIGURED);
                break;
            }
            PayloadCSP::StatusReply reply = {};
            PayloadCspGateway::populateStatusReply(request.header.seq, snapshot, reply);
            sendReply(packet, reply);
            break;
        }
        case PayloadCSP::ServicePort::CAPABILITIES: {
            OBC::PayloadCapabilities capabilities = {};
            if (!this->m_controller->getCapabilitiesForRuntime(capabilities)) {
                sendErrorReply<PayloadCSP::CapabilitiesReply>(
                    packet, PayloadCSP::ServicePort::CAPABILITIES, request.header.seq, PayloadCSP::ResultCode::NOT_CONFIGURED);
                break;
            }
            PayloadCSP::CapabilitiesReply reply = {};
            PayloadCspGateway::populateCapabilitiesReply(request.header.seq, capabilities, reply);
            sendReply(packet, reply);
            break;
        }
        case PayloadCSP::ServicePort::LAST_CAPTURE_METADATA: {
            OBC::PayloadCaptureMetadata metadata = {};
            if (!this->m_controller->getLastCaptureMetadataForRuntime(metadata)) {
                sendErrorReply<PayloadCSP::MetadataReply>(
                    packet, PayloadCSP::ServicePort::LAST_CAPTURE_METADATA, request.header.seq, PayloadCSP::ResultCode::NOT_CONFIGURED);
                break;
            }
            PayloadCSP::MetadataReply reply = {};
            PayloadCspGateway::populateMetadataReply(request.header.seq, metadata, reply);
            sendReply(packet, reply);
            break;
        }
    }

    csp_buffer_free(packet);
}

}  // namespace OBCApp
