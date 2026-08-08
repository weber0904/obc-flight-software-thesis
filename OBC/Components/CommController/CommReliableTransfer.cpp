#include "OBC/Components/CommController/CommReliableTransfer.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

#include "CFDP/Checksum/Checksum.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/FilePacket/FilePacket.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"

namespace OBC {

CommReliableTransfer::CommReliableTransfer(OBC::CSP::ICspRuntime& runtime)
    : m_config(),
      m_runtime(&runtime),
      m_runtimeReady(false),
      m_seq(1U),
      m_nextTransferId(1U),
      m_phase(Phase::IDLE),
      m_state(),
      m_fileBytes(),
      m_sha256(),
      m_checksum(0U),
      m_nextSegmentToSend(0U),
      m_ticksSinceProgress(0U),
      m_currentRetryCount(0U),
      m_lastControlStatus(OBC::CSP::RuntimeStatus::OK),
      m_lastControlReplySize(0U) {}

void CommReliableTransfer::configure(const CommReliableTransferConfig& config) {
    this->m_config = config;
}

void CommReliableTransfer::setRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime) {
    this->m_runtime = &runtime;
    this->m_runtimeReady = false;
}

Svc::SendFileResponse CommReliableTransfer::start(U32 requestContext,
                                                  const Fw::StringBase& sourceFileName,
                                                  const Fw::StringBase& destFileName,
                                                  U32 offset,
                                                  U32 length) {
    this->m_state.requestContext = requestContext;
    this->m_state.lastStartFailureStage = CommReliableTransferStartFailureStage::NONE;
    this->m_state.lastStartFailureDetail = 0U;
    if (this->m_state.active) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::ALREADY_ACTIVE, this->m_state.transferId);
        std::cout << "COMM reliable transfer start rejected: already active transferId="
                  << this->m_state.transferId << "\n";
        std::cout.flush();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, 0U);
    }
    if (offset != 0U || length != 0U) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::UNSUPPORTED_RANGE, offset != 0U ? offset : length);
        std::cout << "COMM reliable transfer start rejected: unsupported range"
                  << " offset=" << offset
                  << " length=" << length << "\n";
        std::cout.flush();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }
    if (!this->ensureRuntime_()) {
        this->recordStartFailure_(requestContext,
                                  CommReliableTransferStartFailureStage::RUNTIME_UNAVAILABLE,
                                  this->m_config.targetNode);
        std::cout << "COMM reliable transfer start rejected: runtime unavailable"
                  << " targetNode=" << this->m_config.targetNode << "\n";
        std::cout.flush();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    const std::string sourcePath = sourceFileName.toChar();
    std::size_t sourceFileSize = 0U;
    if (!queryFileSize_(sourcePath, sourceFileSize)) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::SOURCE_LOAD_FAILED, 0U);
        std::cout << "COMM reliable transfer start rejected: source size query failed"
                  << " sourcePath=" << sourcePath << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    if (sourceFileSize > this->m_config.maxFileBytes) {
        this->recordStartFailure_(requestContext,
                                  CommReliableTransferStartFailureStage::FILE_TOO_LARGE,
                                  static_cast<U32>(sourceFileSize));
        std::cout << "COMM reliable transfer start rejected: file too large"
                  << " sourcePath=" << sourcePath
                  << " fileSize=" << sourceFileSize
                  << " maxFileBytes=" << this->m_config.maxFileBytes << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    this->reset_();
    if (!loadWholeFile_(sourcePath, this->m_fileBytes)) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::SOURCE_LOAD_FAILED, 0U);
        std::cout << "COMM reliable transfer start rejected: source load failed"
                  << " sourcePath=" << sourcePath << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    const std::string sourceBase = baseName_(sourceFileName);
    const std::string destBase = baseName_(destFileName);
    if (sourceBase.empty() || destBase.empty()) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::EMPTY_BASENAME, 0U);
        std::cout << "COMM reliable transfer start rejected: empty basename"
                  << " sourcePath=" << sourceFileName.toChar()
                  << " destPath=" << destFileName.toChar() << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    if (!computeSha256_(this->m_fileBytes, this->m_sha256)) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::SHA256_FAILED, 0U);
        std::cout << "COMM reliable transfer start rejected: sha256 failed"
                  << " sourceBase=" << sourceBase << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }
    this->m_checksum = computeChecksum_(this->m_fileBytes);

    const U32 fileSize = static_cast<U32>(this->m_fileBytes.size());
    const U32 totalSegments = fileSize == 0U ? 0U : ((fileSize - 1U) / this->m_config.segmentPayloadBytes) + 1U;
    const std::uint16_t transferId = this->allocateTransferId_();
    BuiltPacket startPacket = makeStartPacket_(sourceBase, destBase, fileSize);
    if (startPacket.bytes.empty()) {
        this->recordStartFailure_(requestContext, CommReliableTransferStartFailureStage::START_PACKET_BUILD_FAILED, fileSize);
        std::cout << "COMM reliable transfer start rejected: start packet build failed"
                  << " sourceBase=" << sourceBase
                  << " destBase=" << destBase
                  << " fileSize=" << fileSize << "\n";
        std::cout.flush();
        this->reset_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, requestContext);
    }

    this->m_state.active = true;
    this->m_state.requestContext = requestContext;
    this->m_state.transferId = transferId;
    this->m_state.fileSize = fileSize;
    this->m_state.totalSegments = totalSegments;
    this->m_state.contiguousSegments = 0U;
    this->m_state.committedBytes = 0U;
    this->m_state.resendCount = 0U;
    this->m_state.duplicateSegments = 0U;
    this->m_state.lastResult = CommReliableTransferResult::NONE;

    OBC::COMM::CSP::ReliableTransferControlReply reply = {};
    if (!this->requestControl_(OBC::COMM::CSP::ReliableTransferOp::BEGIN, &startPacket, reply)) {
        if (this->m_lastControlStatus != OBC::CSP::RuntimeStatus::OK) {
            this->recordStartFailure_(requestContext,
                                      CommReliableTransferStartFailureStage::BEGIN_REQUEST_FAILED,
                                      static_cast<U32>(this->m_lastControlStatus));
        } else {
            this->recordStartFailure_(requestContext,
                                      CommReliableTransferStartFailureStage::BEGIN_REPLY_INVALID,
                                      static_cast<U32>(this->m_lastControlReplySize));
        }
        const Svc::SendFileResponse response(Svc::SendFileStatus::STATUS_ERROR, requestContext);
        this->reset_();
        return response;
    }

    const auto transferResult =
        static_cast<OBC::COMM::CSP::ReliableTransferResult>(reply.transferResult);
    if (transferResult != OBC::COMM::CSP::ReliableTransferResult::OK) {
        this->recordStartFailure_(requestContext,
                                  CommReliableTransferStartFailureStage::BEGIN_REJECTED,
                                  static_cast<U32>(transferResult));
        const Svc::SendFileResponse response(mapReplyToSendFileStatus_(transferResult), requestContext);
        this->m_state.lastResult = mapReplyToTransferResult_(transferResult);
        this->reset_();
        return response;
    }

    this->m_phase = Phase::SENDING;
    return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, requestContext);
}

CommReliableTransferStepResult CommReliableTransfer::step() {
    CommReliableTransferStepResult result = {};
    if (!this->m_state.active || this->m_phase == Phase::IDLE) {
        return result;
    }

    while (this->m_nextSegmentToSend < this->m_state.totalSegments &&
           this->m_nextSegmentToSend < (this->m_state.contiguousSegments + this->m_config.windowSize)) {
        if (!this->sendDataFrame_(this->m_nextSegmentToSend)) {
            result.finished = true;
            result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, this->m_state.requestContext);
            result.finalResult = CommReliableTransferResult::IO_ERROR;
            this->m_state.lastResult = result.finalResult;
            this->reset_();
            return result;
        }
        this->m_nextSegmentToSend += 1U;
    }

    if (this->m_state.contiguousSegments < this->m_state.totalSegments) {
        OBC::COMM::CSP::ReliableTransferControlReply ackReply = {};
        if (!this->pollAck_(ackReply)) {
            result.finished = true;
            result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, this->m_state.requestContext);
            result.finalResult = CommReliableTransferResult::IO_ERROR;
            this->m_state.lastResult = result.finalResult;
            this->reset_();
            return result;
        }

        const U32 previousSegments = this->m_state.contiguousSegments;
        auto ackResult = static_cast<OBC::COMM::CSP::ReliableTransferResult>(ackReply.transferResult);
        U32 ackContiguousSegments = ackReply.contiguousSegments;
        U32 ackCommittedBytes = ackReply.committedBytes;
        this->m_state.contiguousSegments =
            std::max(previousSegments, std::min(ackContiguousSegments, this->m_state.totalSegments));
        this->m_state.committedBytes =
            std::max(this->m_state.committedBytes, std::min(ackCommittedBytes, this->m_state.fileSize));
        result.duplicateObserved = ackReply.duplicateSegments > this->m_state.duplicateSegments;
        this->m_state.duplicateSegments = ackReply.duplicateSegments;

        if (ackResult == OBC::COMM::CSP::ReliableTransferResult::OK &&
            this->m_state.contiguousSegments > previousSegments) {
            result.progressAdvanced = true;
            this->m_ticksSinceProgress = 0U;
            this->m_currentRetryCount = 0U;
        } else {
            this->m_ticksSinceProgress += 1U;
        }

        const bool outstanding = this->m_nextSegmentToSend > this->m_state.contiguousSegments;
        if (outstanding && this->m_ticksSinceProgress >= this->m_config.ackTimeoutTicks) {
            if (this->m_currentRetryCount >= this->m_config.resendBudget) {
                result = this->cancel(true);
                if (result.finalResult == CommReliableTransferResult::ABORTED) {
                    result.finalResult = CommReliableTransferResult::RETRY_EXHAUSTED;
                    this->m_state.lastResult = result.finalResult;
                }
                result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, result.response.get_context());
                result.finished = true;
                return result;
            }

            const U32 resendLimit = std::min(this->m_state.contiguousSegments + this->m_config.windowSize, this->m_state.totalSegments);
            for (U32 segment = this->m_state.contiguousSegments; segment < resendLimit; segment++) {
                if (!this->sendDataFrame_(segment)) {
                    result.finished = true;
                    result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, this->m_state.requestContext);
                    result.finalResult = CommReliableTransferResult::IO_ERROR;
                    this->m_state.lastResult = result.finalResult;
                    this->reset_();
                    return result;
                }
            }
            this->m_ticksSinceProgress = 0U;
            this->m_currentRetryCount += 1U;
            this->m_state.resendCount += 1U;
            result.resendAttempted = true;
        }
    }

    if (this->m_state.contiguousSegments == this->m_state.totalSegments) {
        BuiltPacket endPacket = this->makeEndPacket_();
        OBC::COMM::CSP::ReliableTransferControlReply reply = {};
        if (!this->requestControl_(OBC::COMM::CSP::ReliableTransferOp::COMPLETE, &endPacket, reply)) {
            result.finished = true;
            result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, this->m_state.requestContext);
            result.finalResult = CommReliableTransferResult::IO_ERROR;
            this->m_state.lastResult = result.finalResult;
            this->reset_();
            return result;
        }

        const auto transferResult = static_cast<OBC::COMM::CSP::ReliableTransferResult>(reply.transferResult);
        result.finished = true;
        result.response = Svc::SendFileResponse(mapReplyToSendFileStatus_(transferResult), this->m_state.requestContext);
        result.finalResult = mapReplyToTransferResult_(transferResult);
        if (transferResult == OBC::COMM::CSP::ReliableTransferResult::OK) {
            result.finalResult = CommReliableTransferResult::SUCCESS;
        }
        this->m_state.lastResult = result.finalResult;
        this->reset_();
    }

    return result;
}

CommReliableTransferStepResult CommReliableTransfer::cancel(bool abortTransfer) {
    CommReliableTransferStepResult result = {};
    if (!this->m_state.active) {
        return result;
    }

    BuiltPacket cancelPacket = this->makeCancelPacket_();
    OBC::COMM::CSP::ReliableTransferControlReply reply = {};
    const OBC::COMM::CSP::ReliableTransferOp op =
        abortTransfer ? OBC::COMM::CSP::ReliableTransferOp::ABORT : OBC::COMM::CSP::ReliableTransferOp::CANCEL;
    const bool ok = this->requestControl_(op, &cancelPacket, reply);

    result.finished = true;
    result.response = Svc::SendFileResponse(ok ? Svc::SendFileStatus::STATUS_BUSY : Svc::SendFileStatus::STATUS_ERROR,
                                            this->m_state.requestContext);
    result.finalResult = ok ? (abortTransfer ? CommReliableTransferResult::ABORTED : CommReliableTransferResult::CANCELLED)
                            : CommReliableTransferResult::IO_ERROR;
    if (ok) {
        result.duplicateObserved = reply.duplicateSegments > this->m_state.duplicateSegments;
    }
    if (this->m_state.lastResult == CommReliableTransferResult::NONE) {
        this->m_state.lastResult = result.finalResult;
    }
    this->reset_();
    return result;
}

const CommReliableTransferState& CommReliableTransfer::state() const {
    return this->m_state;
}

bool CommReliableTransfer::isActive() const {
    return this->m_state.active;
}

bool CommReliableTransfer::ensureRuntime_() {
    if (this->m_runtimeReady) {
        return true;
    }
    if (this->m_runtime->metrics().initialized) {
        this->m_runtimeReady = true;
        return true;
    }
    const OBC::CSP::RuntimeConfig config =
        OBC::CSP::runtimeConfigFromEnvironment(OBC::COMM::CSP::DEFAULT_OBC_NODE_ID, "OBCCSP");
    this->m_runtimeReady = this->m_runtime->init(config) == OBC::CSP::RuntimeStatus::OK;
    return this->m_runtimeReady;
}

std::uint16_t CommReliableTransfer::nextSeq_() {
    const std::uint16_t current = this->m_seq;
    this->m_seq = static_cast<std::uint16_t>(this->m_seq + 1U);
    if (this->m_seq == 0U) {
        this->m_seq = 1U;
    }
    return current;
}

std::uint16_t CommReliableTransfer::allocateTransferId_() {
    const std::uint16_t current = this->m_nextTransferId;
    this->m_nextTransferId = static_cast<std::uint16_t>(this->m_nextTransferId + 1U);
    if (this->m_nextTransferId == 0U) {
        this->m_nextTransferId = 1U;
    }
    return current;
}

std::string CommReliableTransfer::baseName_(const Fw::StringBase& path) {
    const std::string value = path.toChar();
    const std::string::size_type slash = value.find_last_of("/\\");
    return slash == std::string::npos ? value : value.substr(slash + 1U);
}

bool CommReliableTransfer::queryFileSize_(const std::string& path, std::size_t& size) {
    size = 0U;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }
    const std::streamsize streamSize = input.tellg();
    if (streamSize < 0) {
        return false;
    }
    size = static_cast<std::size_t>(streamSize);
    return true;
}

bool CommReliableTransfer::loadWholeFile_(const std::string& path, std::vector<U8>& bytes) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }
    const std::streamsize size = input.tellg();
    if (size < 0) {
        return false;
    }
    input.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(size));
    if (size > 0) {
        input.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    return input.good() || input.eof();
}

bool CommReliableTransfer::computeSha256_(const std::vector<U8>& bytes, std::array<U8, 32>& digest) {
    sha256Bytes(bytes.empty() ? nullptr : bytes.data(), bytes.size(), digest.data());
    return true;
}

U32 CommReliableTransfer::computeChecksum_(const std::vector<U8>& bytes) {
    CFDP::Checksum checksum;
    if (!bytes.empty()) {
        checksum.update(bytes.data(), 0U, static_cast<U32>(bytes.size()));
    }
    return checksum.getValue();
}

CommReliableTransfer::BuiltPacket CommReliableTransfer::makeStartPacket_(const std::string& sourceBase,
                                                                         const std::string& destBase,
                                                                         U32 fileSize) {
    BuiltPacket packet = {};
    if (sourceBase.size() > 63U || destBase.size() > 63U) {
        return packet;
    }
    Fw::FilePacket::StartPacket startPacket = {};
    startPacket.initialize(fileSize, sourceBase.c_str(), destBase.c_str());
    Fw::FilePacket filePacket = {};
    filePacket.fromStartPacket(startPacket);
    const U32 size = filePacket.bufferSize();
    if (size == 0U || size > OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
        return packet;
    }
    packet.bytes.resize(size);
    Fw::Buffer buffer(packet.bytes.data(), size);
    if (filePacket.toBuffer(buffer) != Fw::FW_SERIALIZE_OK) {
        packet.bytes.clear();
    }
    return packet;
}

CommReliableTransfer::BuiltPacket CommReliableTransfer::makeDataPacket_(U32 segmentIndex) const {
    BuiltPacket packet = {};
    const U32 byteOffset = segmentIndex * this->m_config.segmentPayloadBytes;
    const U32 remaining = static_cast<U32>(this->m_fileBytes.size()) - byteOffset;
    const U16 dataSize = static_cast<U16>(std::min<U32>(remaining, this->m_config.segmentPayloadBytes));
    Fw::FilePacket::DataPacket dataPacket = {};
    dataPacket.initialize(segmentIndex + 1U, byteOffset, dataSize, this->m_fileBytes.data() + byteOffset);
    Fw::FilePacket filePacket = {};
    filePacket.fromDataPacket(dataPacket);
    const U32 size = filePacket.bufferSize();
    if (size == 0U || size > OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
        return packet;
    }
    packet.bytes.resize(size);
    Fw::Buffer buffer(packet.bytes.data(), size);
    if (filePacket.toBuffer(buffer) != Fw::FW_SERIALIZE_OK) {
        packet.bytes.clear();
    }
    return packet;
}

CommReliableTransfer::BuiltPacket CommReliableTransfer::makeEndPacket_() const {
    BuiltPacket packet = {};
    Fw::FilePacket::EndPacket endPacket = {};
    endPacket.initialize(this->m_state.totalSegments + 1U, CFDP::Checksum(this->m_checksum));
    Fw::FilePacket filePacket = {};
    filePacket.fromEndPacket(endPacket);
    const U32 size = filePacket.bufferSize();
    if (size == 0U || size > OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
        return packet;
    }
    packet.bytes.resize(size);
    Fw::Buffer buffer(packet.bytes.data(), size);
    if (filePacket.toBuffer(buffer) != Fw::FW_SERIALIZE_OK) {
        packet.bytes.clear();
    }
    return packet;
}

CommReliableTransfer::BuiltPacket CommReliableTransfer::makeCancelPacket_() const {
    BuiltPacket packet = {};
    Fw::FilePacket::CancelPacket cancelPacket = {};
    cancelPacket.initialize(this->m_state.totalSegments + 1U);
    Fw::FilePacket filePacket = {};
    filePacket.fromCancelPacket(cancelPacket);
    const U32 size = filePacket.bufferSize();
    if (size == 0U || size > OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
        return packet;
    }
    packet.bytes.resize(size);
    Fw::Buffer buffer(packet.bytes.data(), size);
    if (filePacket.toBuffer(buffer) != Fw::FW_SERIALIZE_OK) {
        packet.bytes.clear();
    }
    return packet;
}

Svc::SendFileStatus CommReliableTransfer::mapReplyToSendFileStatus_(OBC::COMM::CSP::ReliableTransferResult result) {
    switch (result) {
        case OBC::COMM::CSP::ReliableTransferResult::OK:
            return Svc::SendFileStatus::STATUS_OK;
        case OBC::COMM::CSP::ReliableTransferResult::BUSY:
            return Svc::SendFileStatus::STATUS_BUSY;
        default:
            return Svc::SendFileStatus::STATUS_ERROR;
    }
}

CommReliableTransferResult CommReliableTransfer::mapReplyToTransferResult_(OBC::COMM::CSP::ReliableTransferResult result) {
    switch (result) {
        case OBC::COMM::CSP::ReliableTransferResult::OK:
            return CommReliableTransferResult::SUCCESS;
        case OBC::COMM::CSP::ReliableTransferResult::BUSY:
            return CommReliableTransferResult::BUSY;
        case OBC::COMM::CSP::ReliableTransferResult::HASH_MISMATCH:
            return CommReliableTransferResult::HASH_MISMATCH;
        case OBC::COMM::CSP::ReliableTransferResult::ABORTED:
            return CommReliableTransferResult::ABORTED;
        case OBC::COMM::CSP::ReliableTransferResult::CANCELLED:
            return CommReliableTransferResult::CANCELLED;
        case OBC::COMM::CSP::ReliableTransferResult::IO_ERROR:
            return CommReliableTransferResult::IO_ERROR;
        case OBC::COMM::CSP::ReliableTransferResult::INVALID:
            return CommReliableTransferResult::INVALID;
        case OBC::COMM::CSP::ReliableTransferResult::NO_PROGRESS:
            return CommReliableTransferResult::TIMEOUT;
        default:
            return CommReliableTransferResult::INVALID;
    }
}

void CommReliableTransfer::recordStartFailure_(U32 requestContext,
                                               CommReliableTransferStartFailureStage stage,
                                               U32 detail) {
    this->m_state.requestContext = requestContext;
    this->m_state.lastStartFailureStage = stage;
    this->m_state.lastStartFailureDetail = detail;
}

bool CommReliableTransfer::requestControl_(OBC::COMM::CSP::ReliableTransferOp op,
                                           const BuiltPacket* packet,
                                           OBC::COMM::CSP::ReliableTransferControlReply& reply) {
    OBC::COMM::CSP::ReliableTransferControlRequest request =
        OBC::COMM::CSP::makeReliableTransferControlRequest(op, this->nextSeq_());
    request.transferId = this->m_state.transferId;
    request.segmentCount = this->m_state.totalSegments;
    if (packet != nullptr) {
        if (packet->bytes.size() > OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
            return false;
        }
        request.packetSize = static_cast<std::uint16_t>(packet->bytes.size());
        std::copy(packet->bytes.begin(), packet->bytes.end(), request.packetBytes);
    }
    std::copy(this->m_sha256.begin(), this->m_sha256.end(), request.sha256);

    this->m_lastControlStatus = OBC::CSP::RuntimeStatus::OK;
    this->m_lastControlReplySize = 0U;
    std::size_t replySize = 0U;
    const OBC::CSP::RuntimeStatus status =
        this->m_runtime->requestReply(this->m_config.targetNode,
                                      static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_CONTROL),
                                      &request,
                                      sizeof(request),
                                      &reply,
                                      sizeof(reply),
                                      replySize,
                                      this->m_config.timeoutMs);
    if (status != OBC::CSP::RuntimeStatus::OK) {
        this->m_lastControlStatus = status;
        std::cout << "COMM reliable transfer request failed: op=" << static_cast<unsigned int>(op)
                  << " transferId=" << request.transferId
                  << " targetNode=" << this->m_config.targetNode
                  << " requestSize=" << sizeof(request)
                  << " packetSize=" << request.packetSize
                  << " runtimeStatus=" << static_cast<unsigned int>(status)
                  << "\n";
        std::cout.flush();
        return false;
    }

    const bool validReply = replySize == sizeof(reply) &&
                            reply.header.version == OBC::COMM::CSP::VERSION &&
                            reply.header.service ==
                                static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_CONTROL) &&
                            reply.header.seq == request.header.seq;
    this->m_lastControlReplySize = replySize;
    if (!validReply) {
        std::cout << "COMM reliable transfer reply mismatch: op=" << static_cast<unsigned int>(op)
                  << " transferId=" << request.transferId
                  << " replySize=" << replySize
                  << " version=" << static_cast<unsigned int>(reply.header.version)
                  << " service=" << static_cast<unsigned int>(reply.header.service)
                  << " seq=" << reply.header.seq
                  << " expectedSeq=" << request.header.seq
                  << "\n";
        std::cout.flush();
        return false;
    }

    return true;
}

bool CommReliableTransfer::pollAck_(OBC::COMM::CSP::ReliableTransferControlReply& reply) {
    if (this->requestControl_(OBC::COMM::CSP::ReliableTransferOp::ACK_POLL, nullptr, reply)) {
        return true;
    }

    if (this->m_lastControlStatus == OBC::CSP::RuntimeStatus::TIMEOUT) {
        reply = OBC::COMM::CSP::makeReliableTransferControlReply(
            0U, OBC::COMM::CSP::ResultCode::OK, OBC::COMM::CSP::ReliableTransferOp::ACK_POLL);
        reply.transferId = this->m_state.transferId;
        reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::NO_PROGRESS);
        reply.contiguousSegments = this->m_state.contiguousSegments;
        reply.committedBytes = this->m_state.committedBytes;
        reply.duplicateSegments = this->m_state.duplicateSegments;
        std::cout << "COMM reliable transfer ack poll timeout: transferId=" << this->m_state.transferId
                  << " contiguousSegments=" << this->m_state.contiguousSegments
                  << " committedBytes=" << this->m_state.committedBytes
                  << " duplicateSegments=" << this->m_state.duplicateSegments
                  << "\n";
        std::cout.flush();
        return true;
    }

    return false;
}

bool CommReliableTransfer::sendDataFrame_(U32 segmentIndex) {
    BuiltPacket packet = this->makeDataPacket_(segmentIndex);
    if (packet.bytes.empty()) {
        return false;
    }
    OBC::COMM::CSP::ReliableTransferDataFrame frame =
        OBC::COMM::CSP::makeReliableTransferDataFrame(this->m_state.transferId);
    if (packet.bytes.size() > sizeof(frame.packetBytes)) {
        return false;
    }
    frame.packetSize = static_cast<std::uint16_t>(packet.bytes.size());
    std::copy(packet.bytes.begin(), packet.bytes.end(), frame.packetBytes);
    const std::string payload(reinterpret_cast<const char*>(&frame), sizeof(frame));
    return this->m_runtime->sendRaw(this->m_config.targetNode,
                                    static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_DATA),
                                    payload) == OBC::CSP::RuntimeStatus::OK;
}

void CommReliableTransfer::reset_() {
    this->m_state.active = false;
    this->m_fileBytes.clear();
    this->m_checksum = 0U;
    this->m_nextSegmentToSend = 0U;
    this->m_ticksSinceProgress = 0U;
    this->m_currentRetryCount = 0U;
    this->m_phase = Phase::IDLE;
    std::fill(this->m_sha256.begin(), this->m_sha256.end(), 0U);
}

}  // namespace OBC
