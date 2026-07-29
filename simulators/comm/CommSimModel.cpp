#include "simulators/comm/CommSimModel.hpp"

#include <algorithm>

namespace OBC {
namespace COMM {

CommSimModel::CommSimModel(const CommSimConfig& config)
    : m_config(config),
      m_physicalLinkConnected(false),
      m_forcedDisconnected(false),
      m_downlinkBackpressure(false),
      m_injectedIoError(false),
      m_uplinkQueue(),
      m_rxChunks(0U),
      m_txChunks(0U),
      m_rxErrors(0U),
      m_txErrors(0U),
      m_uplinkDroppedChunks(0U),
      m_uplinkDroppedBytes(0U) {}

void CommSimModel::reset() {
    this->m_physicalLinkConnected = false;
    this->m_forcedDisconnected = false;
    this->m_downlinkBackpressure = false;
    this->m_injectedIoError = false;
    this->m_uplinkQueue.clear();
    this->m_rxChunks = 0U;
    this->m_txChunks = 0U;
    this->m_rxErrors = 0U;
    this->m_txErrors = 0U;
    this->m_uplinkDroppedChunks = 0U;
    this->m_uplinkDroppedBytes = 0U;
}

void CommSimModel::setPhysicalLinkConnected(bool connected) {
    this->m_physicalLinkConnected = connected;
}

void CommSimModel::setForcedDisconnected(bool forcedDisconnected) {
    this->m_forcedDisconnected = forcedDisconnected;
}

void CommSimModel::setDownlinkBackpressure(bool enabled) {
    this->m_downlinkBackpressure = enabled;
}

void CommSimModel::setInjectedIoError(bool enabled) {
    this->m_injectedIoError = enabled;
}

bool CommSimModel::ingestUplinkBytes(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U || !this->isLinkAvailable_()) {
        if (size != 0U) {
            this->m_rxErrors += 1U;
        }
        return false;
    }

    const std::size_t capacity = this->m_config.uplinkQueueCapacity;
    if (size > capacity - std::min(capacity, this->m_uplinkQueue.size())) {
        this->m_uplinkDroppedChunks += 1U;
        this->m_uplinkDroppedBytes += static_cast<std::uint32_t>(std::min<std::size_t>(size, UINT32_MAX));
        this->m_rxErrors += 1U;
        return false;
    }

    this->m_uplinkQueue.insert(this->m_uplinkQueue.end(), data, data + size);
    this->m_rxChunks += 1U;
    return true;
}

void CommSimModel::recordRxError() {
    this->m_rxErrors += 1U;
}

CSP::ChunkReply CommSimModel::handleUplinkPoll(const CSP::UplinkPollRequest& request) {
    CSP::ChunkReply reply = CSP::makeChunkReply(CSP::ServicePort::UPLINK_POLL, request.header.seq, CSP::ResultCode::OK);
    reply.header.flags = this->linkFlags();

    if (!validHeader_(request.header, CSP::ServicePort::UPLINK_POLL)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    const std::size_t requestedBytes = std::min<std::size_t>(request.maxBytes, CSP::MAX_CHUNK_BYTES);
    const std::size_t availableBytes = std::min<std::size_t>(requestedBytes, this->m_uplinkQueue.size());
    if (availableBytes == 0U) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_CHUNK);
        reply.header.byteCount = 0U;
        return reply;
    }

    reply.header.byteCount = static_cast<std::uint16_t>(availableBytes);
    for (std::size_t index = 0U; index < availableBytes; index++) {
        reply.data[index] = this->m_uplinkQueue.front();
        this->m_uplinkQueue.pop_front();
    }
    return reply;
}

CSP::ChunkReply CommSimModel::validateDownlinkWriteRequest(const CSP::DownlinkWriteRequest& request) const {
    CSP::ChunkReply reply =
        CSP::makeChunkReply(CSP::ServicePort::DOWNLINK_WRITE, request.header.seq, CSP::ResultCode::OK);
    reply.header.flags = this->linkFlags();

    if (!validHeader_(request.header, CSP::ServicePort::DOWNLINK_WRITE) || request.byteCount == 0U ||
        request.byteCount > CSP::MAX_CHUNK_BYTES) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    return reply;
}

bool CommSimModel::shouldAttemptSerialDownlink() const {
    return !this->m_forcedDisconnected && !this->m_downlinkBackpressure && !this->m_injectedIoError;
}

CSP::ChunkReply CommSimModel::beginDownlinkWrite(const CSP::DownlinkWriteRequest& request, bool& shouldWrite) {
    shouldWrite = false;
    CSP::ChunkReply reply = this->validateDownlinkWriteRequest(request);
    if (reply.header.result != static_cast<std::uint8_t>(CSP::ResultCode::OK)) {
        return reply;
    }

    if (!this->isLinkAvailable_() || this->m_downlinkBackpressure || this->m_injectedIoError) {
        this->m_txErrors += 1U;
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::IO_ERROR);
        reply.header.flags = this->linkFlags();
        return reply;
    }

    shouldWrite = true;
    return reply;
}

void CommSimModel::completeDownlinkWrite(bool success) {
    if (success) {
        this->m_txChunks += 1U;
    } else {
        this->m_txErrors += 1U;
    }
}

CSP::LinkStatusReply CommSimModel::handleLinkStatus(const CSP::LinkStatusRequest& request) const {
    CSP::LinkStatusReply reply = CSP::makeLinkStatusReply(request.header.seq, CSP::ResultCode::OK);
    if (!validHeader_(request.header, CSP::ServicePort::LINK_STATUS)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
    }
    reply.header.flags = this->linkFlags();
    reply.rxChunks = this->m_rxChunks;
    reply.txChunks = this->m_txChunks;
    reply.rxErrors = this->m_rxErrors;
    reply.txErrors = this->m_txErrors;
    return reply;
}

CommSimStatus CommSimModel::status() const {
    CommSimStatus status = {};
    status.physicalLinkConnected = this->m_physicalLinkConnected;
    status.forcedDisconnected = this->m_forcedDisconnected;
    status.downlinkBackpressure = this->m_downlinkBackpressure;
    status.injectedIoError = this->m_injectedIoError;
    status.effectiveLinkConnected = this->isLinkAvailable_();
    status.uplinkQueueDepth = this->m_uplinkQueue.size();
    status.uplinkQueueCapacity = this->m_config.uplinkQueueCapacity;
    status.rxChunks = this->m_rxChunks;
    status.txChunks = this->m_txChunks;
    status.rxErrors = this->m_rxErrors;
    status.txErrors = this->m_txErrors;
    status.uplinkDroppedChunks = this->m_uplinkDroppedChunks;
    status.uplinkDroppedBytes = this->m_uplinkDroppedBytes;
    return status;
}

std::uint8_t CommSimModel::linkFlags() const {
    return this->isLinkAvailable_() ? CSP::FLAG_LINK_CONNECTED : 0U;
}

bool CommSimModel::isLinkAvailable_() const {
    return this->m_physicalLinkConnected && !this->m_forcedDisconnected;
}

bool CommSimModel::validHeader_(const CSP::RequestHeader& header, CSP::ServicePort service) {
    return header.version == CSP::VERSION && header.service == static_cast<std::uint8_t>(service);
}

}  // namespace COMM
}  // namespace OBC
