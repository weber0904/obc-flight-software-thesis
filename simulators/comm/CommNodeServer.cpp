#include "simulators/comm/CommNodeServer.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <unistd.h>

extern "C" {
#include <csp/csp.h>
#include <csp/csp_buffer.h>
}

#include "CFDP/Checksum/Checksum.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/FilePacket/FilePacket.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"
#include "Os/FileSystem.hpp"
#include "simulators/comm/StreamIo.hpp"

namespace OBC {
namespace COMM {

namespace {

constexpr int kBeaconWriterPollTimeoutMs = 100;
constexpr char kTcFillPattern[] = "sitting well";

bool ingressDiagnosticsEnabled() {
    const char* value = std::getenv("COMM_NODE_INGRESS_DIAGNOSTICS");
    return value != nullptr && std::strcmp(value, "0") != 0;
}

bool stripTcFillPatternEnabled() {
    const char* value = std::getenv("COMM_NODE_STRIP_TC_FILL_PATTERN");
    return value != nullptr && std::strcmp(value, "0") != 0;
}

bool ingressTraceFullEnabled() {
    const char* value = std::getenv("COMM_NODE_INGRESS_TRACE_FULL");
    return value != nullptr && std::strcmp(value, "0") != 0;
}

std::uint32_t parseEnvU32(const char* key, std::uint32_t fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0') {
        return fallback;
    }
    return static_cast<std::uint32_t>(parsed);
}

std::string baseName(const std::string& path) {
    const std::string::size_type slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1U);
}

bool decodeFilePacket(const std::uint8_t* data, std::size_t size, Fw::FilePacket& packet) {
    if (data == nullptr || size == 0U) {
        return false;
    }
    Fw::Buffer buffer(const_cast<std::uint8_t*>(data), static_cast<FwSizeType>(size));
    return packet.fromBuffer(buffer) == Fw::FW_SERIALIZE_OK;
}

bool writeFilePacketData(std::fstream& file, const Fw::FilePacket::DataPacket& packet) {
    file.seekp(packet.getByteOffset(), std::ios::beg);
    if (!file.good()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(packet.getData()), packet.getDataSize());
    file.flush();
    return file.good();
}

std::uint32_t computeFileChecksum(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CFDP::Checksum checksum;
    if (!bytes.empty()) {
        checksum.update(bytes.data(), 0U, static_cast<std::uint32_t>(bytes.size()));
    }
    return checksum.getValue();
}

bool computeFileSha256(const std::string& path, std::array<std::uint8_t, 32>& digest) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    sha256Bytes(bytes.empty() ? nullptr : bytes.data(), bytes.size(), digest.data());
    return true;
}

bool isRepeatedPattern(const std::uint8_t* data, const std::size_t size, const char* pattern) {
    if (data == nullptr || pattern == nullptr || size == 0U) {
        return false;
    }
    const std::size_t patternSize = std::strlen(pattern);
    if (patternSize == 0U || (size % patternSize) != 0U) {
        return false;
    }
    for (std::size_t offset = 0U; offset < size; offset += patternSize) {
        if (std::memcmp(data + offset, pattern, patternSize) != 0) {
            return false;
        }
    }
    return true;
}

const char* streamReadStatusName(const StreamReadStatus status) {
    switch (status) {
        case StreamReadStatus::OK:
            return "OK";
        case StreamReadStatus::TIMEOUT:
            return "TIMEOUT";
        case StreamReadStatus::CLOSED:
            return "CLOSED";
        case StreamReadStatus::IO_ERROR:
            return "IO_ERROR";
        default:
            return "UNKNOWN";
    }
}

const char* externalEndpointName(const CommExternalLinkMode mode) {
    return mode == CommExternalLinkMode::TCP_SERVER ? "tcp-listen" : "serial";
}

const char* downlinkControlV3OpName(const CSP::DownlinkControlV3Op op) {
    switch (op) {
        case CSP::DownlinkControlV3Op::BEGIN:
            return "BEGIN";
        case CSP::DownlinkControlV3Op::ACK_POLL:
            return "ACK_POLL";
        case CSP::DownlinkControlV3Op::COMMIT:
            return "COMMIT";
        case CSP::DownlinkControlV3Op::ABORT:
            return "ABORT";
        case CSP::DownlinkControlV3Op::STATUS:
            return "STATUS";
        default:
            return "UNKNOWN";
    }
}

const char* downlinkResultCodeName(const CSP::ResultCode result) {
    switch (result) {
        case CSP::ResultCode::OK:
            return "OK";
        case CSP::ResultCode::NO_CHUNK:
            return "NO_CHUNK";
        case CSP::ResultCode::INVALID_REQUEST:
            return "INVALID_REQUEST";
        case CSP::ResultCode::IO_ERROR:
            return "IO_ERROR";
        case CSP::ResultCode::NO_CREDIT:
            return "NO_CREDIT";
        case CSP::ResultCode::BUSY:
            return "BUSY";
        case CSP::ResultCode::NO_PROGRESS:
            return "NO_PROGRESS";
        default:
            return "UNKNOWN";
    }
}

std::string hexPreview(const std::uint8_t* data, const std::size_t size, const std::size_t maxBytes = 24U) {
    if (data == nullptr || size == 0U) {
        return "";
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    const std::size_t previewBytes = std::min(size, maxBytes);
    for (std::size_t index = 0U; index < previewBytes; ++index) {
        if (index != 0U) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<unsigned int>(data[index]);
    }
    if (size > previewBytes) {
        stream << " ...";
    }
    return stream.str();
}

bool peekDownlinkDataV3FrameHeader(const void* data, const std::size_t size, CSP::DownlinkDataV3Frame& frameOut) {
    frameOut = {};
    if (data == nullptr || size < CSP::DOWNLINK_V3_FRAME_METADATA_BYTES) {
        return false;
    }
    std::memcpy(&frameOut, data, CSP::DOWNLINK_V3_FRAME_METADATA_BYTES);
    return true;
}

}  // namespace

constexpr std::size_t CommNodeDownlinkV2State::CHUNK_BYTES;
constexpr std::size_t CommNodeDownlinkV2State::STAGING_SLOT_LIMIT;
constexpr std::size_t CommNodeDownlinkV2State::DRAIN_SLOT_LIMIT;
constexpr std::uint32_t CommNodeDownlinkV2State::STAGING_TIMEOUT_MS;
constexpr std::size_t CommNodeDownlinkV3State::FRAME_BYTES;
constexpr std::size_t CommNodeDownlinkV3State::STAGING_FRAME_LIMIT;
constexpr std::size_t CommNodeDownlinkV3State::DRAIN_FRAME_LIMIT;
constexpr std::uint32_t CommNodeDownlinkV3State::STAGING_TIMEOUT_MS;

CommNodeDownlinkV2State::CommNodeDownlinkV2State()
    : m_activeStream(),
      m_drainQueue(),
      m_committedReplyCache(),
      m_acceptedBytes(0U),
      m_flushedBytes(0U),
      m_droppedCommittedBytes(0U) {}

void CommNodeDownlinkV2State::reset() {
    this->clearActiveStream_();
    this->m_drainQueue.clear();
    this->m_committedReplyCache.clear();
    this->m_acceptedBytes = 0U;
    this->m_flushedBytes = 0U;
    this->m_droppedCommittedBytes = 0U;
}

void CommNodeDownlinkV2State::reapTimedOut(std::chrono::steady_clock::time_point now) {
    if (!this->m_activeStream.active || this->m_activeStream.lastActivity == std::chrono::steady_clock::time_point{}) {
        return;
    }
    if ((now - this->m_activeStream.lastActivity) < std::chrono::milliseconds(STAGING_TIMEOUT_MS)) {
        return;
    }
    this->clearActiveStream_();
}

CSP::DownlinkStageV2Reply CommNodeDownlinkV2State::handleStage(const CSP::DownlinkStageV2Request& request,
                                                               std::uint8_t linkFlags,
                                                               bool* acceptedNewChunk) {
    if (acceptedNewChunk != nullptr) {
        *acceptedNewChunk = false;
    }
    this->reapTimedOut(std::chrono::steady_clock::now());

    CSP::DownlinkStageV2Reply reply = CSP::makeDownlinkStageV2Reply(request.header.seq, CSP::ResultCode::OK);
    reply.header.flags = linkFlags;
    reply.streamId = request.streamId;
    reply.acceptedSeq = request.header.seq;
    reply.drainFreeSlots = this->drainFreeSlots_();

    const CachedStageReply* const activeReply = this->findActiveReply_(request);
    if (activeReply != nullptr) {
        return activeReply->reply;
    }
    const CachedStageReply* const committedReply = this->findCommittedReply_(request);
    if (committedReply != nullptr) {
        return committedReply->reply;
    }

    const bool first = (request.flags & CSP::FLAG_DOWNLINK_STAGE_V2_FIRST) != 0U;
    const bool last = (request.flags & CSP::FLAG_DOWNLINK_STAGE_V2_LAST) != 0U;
    if (!validV2Header_(request.header, CSP::ServicePort::DOWNLINK_STAGE_V2) || request.byteCount == 0U ||
        request.byteCount > CHUNK_BYTES || (!first && !this->m_activeStream.active) ||
        (this->m_activeStream.active && request.streamId != this->m_activeStream.streamId)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    if (!this->m_activeStream.active) {
        if (!first || request.header.seq != 1U) {
            reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
            return reply;
        }
        this->m_activeStream.active = true;
        this->m_activeStream.streamId = request.streamId;
        this->m_activeStream.nextSeq = 1U;
        this->m_activeStream.stagedChunks.clear();
        this->m_activeStream.stagedReplies.clear();
        this->m_activeStream.pendingFinal = false;
    }

    if (this->m_activeStream.pendingFinal) {
        if (request.streamId == this->m_activeStream.streamId && request.header.seq == this->m_activeStream.nextSeq &&
            drainChunkEqualsRequest_(this->m_activeStream.pendingFinalChunk, request) &&
            request.flags == this->m_activeStream.pendingFinalFlags) {
            if (!this->tryCommitPendingFinal_(linkFlags)) {
                reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_CREDIT);
                reply.stagedSlotsUsed = static_cast<std::uint16_t>(this->stagedSlotCount_());
                reply.drainFreeSlots = this->drainFreeSlots_();
                return reply;
            }

            reply.stagedSlotsUsed = 0U;
            reply.drainFreeSlots = this->drainFreeSlots_();
            return this->m_committedReplyCache.back().reply;
        }

        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    if (request.header.seq != this->m_activeStream.nextSeq) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    DrainChunk chunk = {};
    chunk.streamId = request.streamId;
    chunk.acceptedSeq = request.header.seq;
    chunk.byteCount = request.byteCount;
    std::memcpy(chunk.data.data(), request.data, request.byteCount);

    if (!last) {
        if (!this->m_activeStream.active || request.header.seq == 0U ||
            (request.header.seq != 1U && first) || this->m_activeStream.stagedChunks.size() >= STAGING_SLOT_LIMIT) {
            reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
            return reply;
        }

        this->m_activeStream.stagedChunks.push_back(chunk);
        this->m_activeStream.nextSeq = static_cast<std::uint16_t>(request.header.seq + 1U);
        this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
        reply.stagedSlotsUsed = static_cast<std::uint16_t>(this->stagedSlotCount_());
        reply.drainFreeSlots = this->drainFreeSlots_();
        this->m_activeStream.stagedReplies.push_back({request.streamId, request.header.seq, chunk, request.flags, reply});
        if (acceptedNewChunk != nullptr) {
            *acceptedNewChunk = true;
        }
        return reply;
    }

    if ((request.header.seq != 1U && first) || (request.header.seq == 1U && !first)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }

    this->m_activeStream.pendingFinal = true;
    this->m_activeStream.pendingFinalChunk = chunk;
    this->m_activeStream.pendingFinalFlags = request.flags;
    this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
    if (!this->tryCommitPendingFinal_(linkFlags)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_CREDIT);
        reply.stagedSlotsUsed = static_cast<std::uint16_t>(this->stagedSlotCount_());
        reply.drainFreeSlots = this->drainFreeSlots_();
        return reply;
    }

    if (acceptedNewChunk != nullptr) {
        *acceptedNewChunk = true;
    }
    return this->m_committedReplyCache.back().reply;
}

CSP::DownlinkStatusV2Reply CommNodeDownlinkV2State::handleStatus(const CSP::DownlinkStatusV2Request& request,
                                                                 std::uint8_t linkFlags) const {
    CSP::DownlinkStatusV2Reply reply = CSP::makeDownlinkStatusV2Reply(request.header.seq, CSP::ResultCode::OK);
    reply.header.flags = linkFlags;
    if (!validV2Header_(request.header, CSP::ServicePort::DOWNLINK_STATUS_V2)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }
    reply.drainQueuedSlots = static_cast<std::uint16_t>(this->m_drainQueue.size());
    reply.drainFreeSlots = this->drainFreeSlots_();
    reply.stagingActive = this->m_activeStream.active ? 1U : 0U;
    reply.stagingStreamId = this->m_activeStream.active ? this->m_activeStream.streamId : 0U;
    reply.acceptedBytes = this->m_acceptedBytes;
    reply.flushedBytes = this->m_flushedBytes;
    reply.droppedCommittedBytes = this->m_droppedCommittedBytes;
    return reply;
}

CSP::DownlinkAbortV2Reply CommNodeDownlinkV2State::handleAbort(const CSP::DownlinkAbortV2Request& request,
                                                               std::uint8_t linkFlags) {
    CSP::DownlinkAbortV2Reply reply = CSP::makeDownlinkAbortV2Reply(request.header.seq, CSP::ResultCode::OK);
    reply.header.flags = linkFlags;
    reply.streamId = request.streamId;
    if (!validV2Header_(request.header, CSP::ServicePort::DOWNLINK_ABORT_V2)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        return reply;
    }
    if (this->m_activeStream.active && this->m_activeStream.streamId == request.streamId) {
        this->clearActiveStream_();
        return reply;
    }
    return reply;
}

bool CommNodeDownlinkV2State::hasDrainWork() const {
    return !this->m_drainQueue.empty();
}

bool CommNodeDownlinkV2State::peekDrainChunk(DrainChunk& out) const {
    if (this->m_drainQueue.empty()) {
        return false;
    }
    out = this->m_drainQueue.front();
    return true;
}

bool CommNodeDownlinkV2State::confirmDrainChunkFlushed(const DrainChunk& chunk) {
    if (this->m_drainQueue.empty()) {
        return false;
    }
    const DrainChunk& front = this->m_drainQueue.front();
    if (front.streamId != chunk.streamId || front.acceptedSeq != chunk.acceptedSeq || front.byteCount != chunk.byteCount) {
        return false;
    }
    this->m_drainQueue.pop_front();
    this->m_flushedBytes += chunk.byteCount;
    if (this->m_drainQueue.empty()) {
        this->m_committedReplyCache.clear();
    }
    return true;
}

void CommNodeDownlinkV2State::handleDrainWriteFailure(std::size_t inFlightBytes) {
    this->m_droppedCommittedBytes +=
        static_cast<std::uint32_t>(std::max<std::uint32_t>(static_cast<std::uint32_t>(inFlightBytes), totalBytes_(this->m_drainQueue)));
    this->m_drainQueue.clear();
    this->clearActiveStream_();
    this->m_committedReplyCache.clear();
}

bool CommNodeDownlinkV2State::validV2Header_(const CSP::RequestHeader& header, CSP::ServicePort service) {
    return header.version == CSP::VERSION_V2 && header.service == static_cast<std::uint8_t>(service);
}

bool CommNodeDownlinkV2State::drainChunkEqualsRequest_(const DrainChunk& chunk,
                                                       const CSP::DownlinkStageV2Request& request) {
    return chunk.streamId == request.streamId && chunk.acceptedSeq == request.header.seq &&
           chunk.byteCount == request.byteCount && std::memcmp(chunk.data.data(), request.data, request.byteCount) == 0;
}

std::uint32_t CommNodeDownlinkV2State::totalBytes_(const std::deque<DrainChunk>& chunks) {
    std::uint32_t total = 0U;
    for (const DrainChunk& chunk : chunks) {
        total += chunk.byteCount;
    }
    return total;
}

std::size_t CommNodeDownlinkV2State::stagedSlotCount_() const {
    if (!this->m_activeStream.active) {
        return 0U;
    }
    return this->m_activeStream.stagedChunks.size() + (this->m_activeStream.pendingFinal ? 1U : 0U);
}

std::uint16_t CommNodeDownlinkV2State::drainFreeSlots_() const {
    const std::size_t freeSlots = DRAIN_SLOT_LIMIT > this->m_drainQueue.size() ? DRAIN_SLOT_LIMIT - this->m_drainQueue.size() : 0U;
    return static_cast<std::uint16_t>(freeSlots);
}

void CommNodeDownlinkV2State::clearActiveStream_() {
    this->m_activeStream.active = false;
    this->m_activeStream.streamId = 0U;
    this->m_activeStream.nextSeq = 1U;
    this->m_activeStream.stagedChunks.clear();
    this->m_activeStream.stagedReplies.clear();
    this->m_activeStream.pendingFinal = false;
    this->m_activeStream.pendingFinalChunk = {};
    this->m_activeStream.pendingFinalFlags = 0U;
    this->m_activeStream.lastActivity = std::chrono::steady_clock::time_point{};
}

void CommNodeDownlinkV2State::cacheCommittedReplies_(std::vector<CachedStageReply>& replies) {
    for (const CachedStageReply& entry : replies) {
        this->m_committedReplyCache.push_back(entry);
    }
    this->trimCommittedReplyCache_();
}

const CommNodeDownlinkV2State::CachedStageReply* CommNodeDownlinkV2State::findActiveReply_(
    const CSP::DownlinkStageV2Request& request) const {
    if (!this->m_activeStream.active || this->m_activeStream.streamId != request.streamId) {
        return nullptr;
    }
    for (const CachedStageReply& entry : this->m_activeStream.stagedReplies) {
        if (cachedReplyMatchesRequest_(entry, request)) {
            return &entry;
        }
    }
    return nullptr;
}

const CommNodeDownlinkV2State::CachedStageReply* CommNodeDownlinkV2State::findCommittedReply_(
    const CSP::DownlinkStageV2Request& request) const {
    for (const CachedStageReply& entry : this->m_committedReplyCache) {
        if (cachedReplyMatchesRequest_(entry, request)) {
            return &entry;
        }
    }
    return nullptr;
}

bool CommNodeDownlinkV2State::cachedReplyMatchesRequest_(const CachedStageReply& entry,
                                                         const CSP::DownlinkStageV2Request& request) {
    return entry.flags == request.flags && drainChunkEqualsRequest_(entry.chunk, request);
}

void CommNodeDownlinkV2State::trimCommittedReplyCache_() {
    while (this->m_committedReplyCache.size() > DRAIN_SLOT_LIMIT + STAGING_SLOT_LIMIT) {
        this->m_committedReplyCache.pop_front();
    }
}

bool CommNodeDownlinkV2State::tryCommitPendingFinal_(std::uint8_t linkFlags) {
    if (!this->m_activeStream.active || !this->m_activeStream.pendingFinal) {
        return false;
    }

    const std::size_t requiredSlots = this->m_activeStream.stagedChunks.size() + 1U;
    if (requiredSlots > this->drainFreeSlots_()) {
        return false;
    }

    for (const DrainChunk& chunk : this->m_activeStream.stagedChunks) {
        this->m_drainQueue.push_back(chunk);
    }
    this->m_drainQueue.push_back(this->m_activeStream.pendingFinalChunk);

    CSP::DownlinkStageV2Reply finalReply =
        CSP::makeDownlinkStageV2Reply(this->m_activeStream.pendingFinalChunk.acceptedSeq, CSP::ResultCode::OK);
    finalReply.header.flags = linkFlags;
    finalReply.streamId = this->m_activeStream.streamId;
    finalReply.acceptedSeq = this->m_activeStream.pendingFinalChunk.acceptedSeq;
    finalReply.stagedSlotsUsed = 0U;
    finalReply.drainFreeSlots = this->drainFreeSlots_();
    this->m_activeStream.stagedReplies.push_back(
        {this->m_activeStream.streamId,
         this->m_activeStream.pendingFinalChunk.acceptedSeq,
         this->m_activeStream.pendingFinalChunk,
         this->m_activeStream.pendingFinalFlags,
         finalReply});

    for (const DrainChunk& chunk : this->m_activeStream.stagedChunks) {
        this->m_acceptedBytes += chunk.byteCount;
    }
    this->m_acceptedBytes += this->m_activeStream.pendingFinalChunk.byteCount;
    this->cacheCommittedReplies_(this->m_activeStream.stagedReplies);
    this->clearActiveStream_();
    return true;
}

CommNodeDownlinkV3State::CommNodeDownlinkV3State()
    : m_activeStream(),
      m_committedStream(),
      m_drainQueue(),
      m_acceptedBytes(0U),
      m_flushedBytes(0U),
      m_droppedCommittedBytes(0U) {}

void CommNodeDownlinkV3State::reset() {
    this->clearActiveStream_();
    this->m_committedStream = {};
    this->m_drainQueue.clear();
    this->m_acceptedBytes = 0U;
    this->m_flushedBytes = 0U;
    this->m_droppedCommittedBytes = 0U;
}

void CommNodeDownlinkV3State::reapTimedOut(std::chrono::steady_clock::time_point now) {
    if (!this->m_activeStream.active || this->m_activeStream.lastActivity == std::chrono::steady_clock::time_point{}) {
        return;
    }
    if ((now - this->m_activeStream.lastActivity) < std::chrono::milliseconds(STAGING_TIMEOUT_MS)) {
        return;
    }
    this->clearActiveStream_();
}

CSP::DownlinkControlV3Reply CommNodeDownlinkV3State::handleControl(const CSP::DownlinkControlV3Request& request,
                                                                   std::uint8_t linkFlags) {
    this->reapTimedOut(std::chrono::steady_clock::now());

    const CSP::DownlinkControlV3Op op = static_cast<CSP::DownlinkControlV3Op>(request.op);
    CSP::DownlinkControlV3Reply reply = CSP::makeDownlinkControlV3Reply(op, request.header.seq, CSP::ResultCode::OK);
    if (!validV3ControlHeader_(request.header)) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        this->populateReplyStats_(reply, linkFlags);
        return reply;
    }

    if (op == CSP::DownlinkControlV3Op::STATUS) {
        this->populateReplyStats_(reply, linkFlags);
        return reply;
    }

    if (op == CSP::DownlinkControlV3Op::BEGIN) {
        if (this->matchesActiveBegin_(request)) {
            reply = this->m_activeStream.beginReply;
            reply.header.seq = request.header.seq;
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }
        if (!this->canAcceptBegin_(request)) {
            reply.header.result = static_cast<std::uint8_t>(this->m_activeStream.active ? CSP::ResultCode::BUSY
                                                                                        : CSP::ResultCode::INVALID_REQUEST);
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }

        this->m_activeStream.active = true;
        this->m_activeStream.streamId = request.streamId;
        this->m_activeStream.totalFrames = request.totalFrames;
        this->m_activeStream.totalBytes = request.totalBytes;
        this->m_activeStream.stagedFrames.assign(request.totalFrames, StagedFrame{});
        this->m_activeStream.duplicateFrames = 0U;
        this->m_activeStream.commitComplete = false;
        this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
        this->m_activeStream.beginReply = reply;
        this->populateReplyStats_(this->m_activeStream.beginReply, linkFlags);
        return this->m_activeStream.beginReply;
    }

    if (op == CSP::DownlinkControlV3Op::ABORT) {
        if (this->m_activeStream.active && request.streamId == this->m_activeStream.streamId) {
            this->clearActiveStream_();
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }
        if (this->matchesCommittedCommit_(request)) {
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        this->populateReplyStats_(reply, linkFlags);
        return reply;
    }

    if (this->matchesCommittedCommit_(request) && op == CSP::DownlinkControlV3Op::COMMIT) {
        reply = this->m_committedStream.commitReply;
        reply.header.seq = request.header.seq;
        this->populateReplyStats_(reply, linkFlags);
        return reply;
    }

    if (!this->m_activeStream.active || request.streamId != this->m_activeStream.streamId ||
        request.totalFrames != this->m_activeStream.totalFrames || request.totalBytes != this->m_activeStream.totalBytes) {
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
        this->populateReplyStats_(reply, linkFlags);
        return reply;
    }

    this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
    if (op == CSP::DownlinkControlV3Op::ACK_POLL) {
        this->populateReplyStats_(reply, linkFlags);
        if (reply.contiguousFrames == request.committedFrames && reply.contiguousBytes == request.committedBytes) {
            reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_PROGRESS);
        }
        return reply;
    }

    if (op == CSP::DownlinkControlV3Op::COMMIT) {
        if (this->contiguousFrames_() != this->m_activeStream.totalFrames ||
            this->contiguousBytes_() != this->m_activeStream.totalBytes) {
            reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_PROGRESS);
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }
        if (!this->tryCommitActive_(linkFlags, reply)) {
            reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::NO_CREDIT);
            this->populateReplyStats_(reply, linkFlags);
            return reply;
        }
        return reply;
    }

    reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::INVALID_REQUEST);
    this->populateReplyStats_(reply, linkFlags);
    return reply;
}

bool CommNodeDownlinkV3State::handleData(const CSP::DownlinkDataV3Frame& frame) {
    this->reapTimedOut(std::chrono::steady_clock::now());
    if (!this->m_activeStream.active || !validV3FrameShape_(frame) || frame.streamId != this->m_activeStream.streamId ||
        frame.frameCount != this->m_activeStream.totalFrames || frame.frameIndex >= this->m_activeStream.totalFrames) {
        return false;
    }
    if (!this->frameFitsActiveLayout_(frame)) {
        return false;
    }

    StagedFrame& staged = this->m_activeStream.stagedFrames[frame.frameIndex];
    if (staged.received) {
        if (staged.frame.byteCount == frame.byteCount &&
            staged.frame.byteOffset == frame.byteOffset &&
            std::memcmp(staged.frame.data.data(), frame.data, frame.byteCount) == 0) {
            this->m_activeStream.duplicateFrames = static_cast<std::uint16_t>(this->m_activeStream.duplicateFrames + 1U);
        }
        this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
        return false;
    }

    staged.received = true;
    staged.frame.streamId = frame.streamId;
    staged.frame.frameIndex = frame.frameIndex;
    staged.frame.frameCount = frame.frameCount;
    staged.frame.byteOffset = frame.byteOffset;
    staged.frame.byteCount = frame.byteCount;
    std::memcpy(staged.frame.data.data(), frame.data, frame.byteCount);
    this->m_activeStream.lastActivity = std::chrono::steady_clock::now();
    return true;
}

bool CommNodeDownlinkV3State::hasDrainWork() const {
    return !this->m_drainQueue.empty();
}

bool CommNodeDownlinkV3State::peekDrainFrame(DrainFrame& out) const {
    if (this->m_drainQueue.empty()) {
        return false;
    }
    out = this->m_drainQueue.front();
    return true;
}

bool CommNodeDownlinkV3State::confirmDrainFrameFlushed(const DrainFrame& frame) {
    if (this->m_drainQueue.empty()) {
        return false;
    }
    const DrainFrame& front = this->m_drainQueue.front();
    if (front.streamId != frame.streamId || front.frameIndex != frame.frameIndex || front.byteCount != frame.byteCount ||
        front.byteOffset != frame.byteOffset) {
        return false;
    }
    this->m_drainQueue.pop_front();
    this->m_flushedBytes += frame.byteCount;
    return true;
}

void CommNodeDownlinkV3State::handleDrainWriteFailure(std::size_t inFlightBytes) {
    this->m_droppedCommittedBytes +=
        static_cast<std::uint32_t>(std::max<std::uint32_t>(static_cast<std::uint32_t>(inFlightBytes), totalBytes_(this->m_drainQueue)));
    this->m_drainQueue.clear();
    this->clearActiveStream_();
    this->m_committedStream = {};
}

bool CommNodeDownlinkV3State::validV3ControlHeader_(const CSP::RequestHeader& header) {
    return header.version == CSP::VERSION_V3 &&
           header.service == static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_CONTROL_V3);
}

bool CommNodeDownlinkV3State::validV3FrameShape_(const CSP::DownlinkDataV3Frame& frame) {
    return frame.version == CSP::VERSION_V3 && frame.kind == CSP::DOWNLINK_V3_DATA &&
           frame.frameCount > 0U && frame.frameCount <= STAGING_FRAME_LIMIT &&
           frame.byteCount > 0U && frame.byteCount <= FRAME_BYTES;
}

std::uint16_t CommNodeDownlinkV3State::drainFreeFrames_(const std::deque<DrainFrame>& drainQueue) {
    const std::size_t freeFrames = DRAIN_FRAME_LIMIT > drainQueue.size() ? DRAIN_FRAME_LIMIT - drainQueue.size() : 0U;
    return static_cast<std::uint16_t>(freeFrames);
}

std::uint32_t CommNodeDownlinkV3State::totalBytes_(const std::deque<DrainFrame>& drainQueue) {
    std::uint32_t total = 0U;
    for (const DrainFrame& frame : drainQueue) {
        total += frame.byteCount;
    }
    return total;
}

std::uint16_t CommNodeDownlinkV3State::contiguousFrames_() const {
    if (!this->m_activeStream.active) {
        return 0U;
    }
    std::uint16_t contiguous = 0U;
    while (contiguous < this->m_activeStream.totalFrames && this->m_activeStream.stagedFrames[contiguous].received) {
        contiguous = static_cast<std::uint16_t>(contiguous + 1U);
    }
    return contiguous;
}

std::uint32_t CommNodeDownlinkV3State::contiguousBytes_() const {
    if (!this->m_activeStream.active) {
        return 0U;
    }
    std::uint32_t bytes = 0U;
    const std::uint16_t contiguous = this->contiguousFrames_();
    for (std::uint16_t index = 0U; index < contiguous; ++index) {
        bytes += this->m_activeStream.stagedFrames[index].frame.byteCount;
    }
    return bytes;
}

std::uint16_t CommNodeDownlinkV3State::windowCredit_() const {
    if (!this->m_activeStream.active) {
        return static_cast<std::uint16_t>(STAGING_FRAME_LIMIT);
    }
    std::uint16_t receivedCount = 0U;
    for (const StagedFrame& frame : this->m_activeStream.stagedFrames) {
        if (frame.received) {
            receivedCount = static_cast<std::uint16_t>(receivedCount + 1U);
        }
    }
    const std::uint16_t totalWindow = static_cast<std::uint16_t>(std::min<std::size_t>(STAGING_FRAME_LIMIT, this->m_activeStream.totalFrames));
    return totalWindow > receivedCount ? static_cast<std::uint16_t>(totalWindow - receivedCount) : 0U;
}

void CommNodeDownlinkV3State::populateReplyStats_(CSP::DownlinkControlV3Reply& reply, std::uint8_t linkFlags) const {
    reply.header.flags = linkFlags;
    reply.streamId = this->m_activeStream.active ? this->m_activeStream.streamId : 0U;
    reply.stagingActive = this->m_activeStream.active ? 1U : 0U;
    reply.stagingStreamId = this->m_activeStream.active ? this->m_activeStream.streamId : 0U;
    reply.totalFrames = this->m_activeStream.active ? this->m_activeStream.totalFrames : 0U;
    reply.contiguousFrames = this->contiguousFrames_();
    reply.contiguousBytes = this->contiguousBytes_();
    reply.windowCredit = this->windowCredit_();
    reply.drainQueuedFrames = static_cast<std::uint16_t>(this->m_drainQueue.size());
    reply.drainFreeFrames = drainFreeFrames_(this->m_drainQueue);
    reply.duplicateFrames = this->m_activeStream.active ? this->m_activeStream.duplicateFrames : 0U;
    reply.acceptedBytes = this->m_acceptedBytes;
    reply.flushedBytes = this->m_flushedBytes;
    reply.droppedCommittedBytes = this->m_droppedCommittedBytes;
}

void CommNodeDownlinkV3State::clearActiveStream_() {
    this->m_activeStream.active = false;
    this->m_activeStream.streamId = 0U;
    this->m_activeStream.totalFrames = 0U;
    this->m_activeStream.totalBytes = 0U;
    this->m_activeStream.stagedFrames.clear();
    this->m_activeStream.duplicateFrames = 0U;
    this->m_activeStream.commitComplete = false;
    this->m_activeStream.beginReply = {};
    this->m_activeStream.commitReply = {};
    this->m_activeStream.lastActivity = std::chrono::steady_clock::time_point{};
}

bool CommNodeDownlinkV3State::canAcceptBegin_(const CSP::DownlinkControlV3Request& request) const {
    if (this->m_activeStream.active || request.streamId == 0U || request.totalFrames == 0U ||
        request.totalFrames > STAGING_FRAME_LIMIT || request.totalBytes == 0U ||
        request.committedFrames != 0U || request.committedBytes != 0U) {
        return false;
    }
    return true;
}

bool CommNodeDownlinkV3State::matchesActiveBegin_(const CSP::DownlinkControlV3Request& request) const {
    return this->m_activeStream.active && request.streamId == this->m_activeStream.streamId &&
           request.totalFrames == this->m_activeStream.totalFrames && request.totalBytes == this->m_activeStream.totalBytes;
}

bool CommNodeDownlinkV3State::matchesCommittedCommit_(const CSP::DownlinkControlV3Request& request) const {
    return this->m_committedStream.valid && request.streamId == this->m_committedStream.streamId &&
           request.totalFrames == this->m_committedStream.totalFrames && request.totalBytes == this->m_committedStream.totalBytes;
}

bool CommNodeDownlinkV3State::frameFitsActiveLayout_(const CSP::DownlinkDataV3Frame& frame) const {
    const std::uint32_t frameEnd = frame.byteOffset + static_cast<std::uint32_t>(frame.byteCount);
    if (frame.byteOffset >= this->m_activeStream.totalBytes || frameEnd > this->m_activeStream.totalBytes) {
        return false;
    }
    if (frame.frameIndex == 0U) {
        if (frame.byteOffset != 0U) {
            return false;
        }
    } else {
        const StagedFrame& previous = this->m_activeStream.stagedFrames[frame.frameIndex - 1U];
        if (previous.received) {
            const std::uint32_t previousEnd = previous.frame.byteOffset + static_cast<std::uint32_t>(previous.frame.byteCount);
            if (frame.byteOffset != previousEnd) {
                return false;
            }
        }
    }

    if (frame.frameIndex + 1U == this->m_activeStream.totalFrames) {
        return frameEnd == this->m_activeStream.totalBytes;
    }

    if (frameEnd >= this->m_activeStream.totalBytes) {
        return false;
    }

    const StagedFrame& next = this->m_activeStream.stagedFrames[frame.frameIndex + 1U];
    if (next.received) {
        return next.frame.byteOffset == frameEnd;
    }
    return true;
}

bool CommNodeDownlinkV3State::tryCommitActive_(std::uint8_t linkFlags, CSP::DownlinkControlV3Reply& replyOut) {
    if (!this->m_activeStream.active || this->m_activeStream.commitComplete ||
        this->m_activeStream.totalFrames > drainFreeFrames_(this->m_drainQueue)) {
        return false;
    }

    for (const StagedFrame& frame : this->m_activeStream.stagedFrames) {
        if (!frame.received) {
            return false;
        }
    }

    for (const StagedFrame& frame : this->m_activeStream.stagedFrames) {
        this->m_drainQueue.push_back(frame.frame);
        this->m_acceptedBytes += frame.frame.byteCount;
    }

    replyOut = CSP::makeDownlinkControlV3Reply(CSP::DownlinkControlV3Op::COMMIT,
                                               replyOut.header.seq,
                                               CSP::ResultCode::OK);
    const std::uint16_t committedFrames = this->m_activeStream.totalFrames;
    const std::uint32_t committedBytes = this->m_activeStream.totalBytes;
    this->m_activeStream.commitComplete = true;
    this->populateReplyStats_(replyOut, linkFlags);
    replyOut.streamId = this->m_activeStream.streamId;
    replyOut.totalFrames = committedFrames;

    this->m_committedStream.valid = true;
    this->m_committedStream.streamId = this->m_activeStream.streamId;
    this->m_committedStream.totalFrames = committedFrames;
    this->m_committedStream.totalBytes = committedBytes;
    this->m_committedStream.commitReply = replyOut;

    this->clearActiveStream_();
    replyOut.drainQueuedFrames = static_cast<std::uint16_t>(this->m_drainQueue.size());
    replyOut.drainFreeFrames = drainFreeFrames_(this->m_drainQueue);
    replyOut.acceptedBytes = this->m_acceptedBytes;
    replyOut.flushedBytes = this->m_flushedBytes;
    replyOut.droppedCommittedBytes = this->m_droppedCommittedBytes;
    replyOut.windowCredit = static_cast<std::uint16_t>(STAGING_FRAME_LIMIT);
    replyOut.stagingActive = 0U;
    replyOut.stagingStreamId = 0U;
    replyOut.contiguousFrames = committedFrames;
    replyOut.contiguousBytes = committedBytes;
    this->m_committedStream.commitReply = replyOut;
    return true;
}

CommNodeServer::CommNodeServer(const CommNodeConfig& config)
    : m_config(config),
      m_running(false),
      m_runtime(),
      m_linkFd(-1),
      m_tcpListenFd(-1),
      m_beaconFd(-1),
      m_linkMutex(),
      m_beaconLinkMutex(),
      m_beaconMutex(),
      m_beaconCv(),
      m_pendingBeaconFrame(),
      m_beaconWriter(),
      m_beaconWriterRunning(false),
      m_beaconFramePending(false),
      m_downlinkV2Mutex(),
      m_downlinkV2Cv(),
      m_downlinkV2DrainWorker(),
      m_downlinkV2DrainWorkerRunning(false),
      m_downlinkV2State(),
      m_downlinkV3Mutex(),
      m_downlinkV3Cv(),
      m_downlinkV3DrainWorker(),
      m_downlinkV3DrainWorkerRunning(false),
      m_downlinkV3State(),
      m_model(config.simConfig),
      m_serialIngressFilter(),
      m_reliableTransfer() {
    const char* outputDir = std::getenv("COMM_RT_OUTPUT_DIR");
    if (outputDir != nullptr && outputDir[0] != '\0') {
        this->m_reliableTransfer.enabled = true;
        this->m_reliableTransfer.outputDir = outputDir;
    }
    this->m_reliableTransfer.staleTimeoutMs = parseEnvU32("COMM_RT_STALE_TIMEOUT_MS", 5000U);
}

CommNodeServer::~CommNodeServer() {
    this->stop();
}

bool CommNodeServer::start() {
    if (this->m_running.load()) {
        return true;
    }

    ::OBC::CSP::RuntimeConfig runtimeConfig = ::OBC::CSP::runtimeConfigFromEnvironment(
        this->m_config.nodeId,
        this->m_config.interfaceName.empty() ? "COMMCSP" : this->m_config.interfaceName.c_str());
    if (this->m_runtime.init(runtimeConfig) != ::OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize COMM CSP node " << this->m_config.nodeId << std::endl;
        return false;
    }

    this->m_running.store(true);
    if (!this->m_config.beaconSerialDevice.empty()) {
        {
            std::lock_guard<std::mutex> lock(this->m_beaconMutex);
            this->m_beaconWriterRunning = true;
        }
        this->m_beaconWriter = std::thread(&CommNodeServer::beaconWriterLoop_, this);
    }
    this->startDownlinkDrainWorker_();
    return true;
}

void CommNodeServer::run() {
    if (!this->start()) {
        return;
    }

    csp_socket_t socket = {};
    csp_bind(&socket, CSP_ANY);
    csp_listen(&socket, 8);

    while (this->m_running.load()) {
        this->reapDownlinkV2Timeout_();
        this->reapDownlinkV3Timeout_();
        this->pollExternalIngress_(20U);

        csp_conn_t* const conn = csp_accept(&socket, 50);
        if (conn == nullptr) {
            continue;
        }

        this->handleConnection_(conn);
        csp_close(conn);
    }

    csp_socket_close(&socket);
}

void CommNodeServer::stop() {
    this->m_running.store(false);
    this->stopDownlinkDrainWorker_();
    this->stopBeaconWriter_();
    this->closeExternalLink_();
    this->closeTcpListen_();
    this->closeBeaconLink_();
    this->m_runtime.shutdown();
}

bool CommNodeServer::ensureExternalLinkOpen_(std::uint32_t timeoutMs) {
    std::lock_guard<std::mutex> linkLock(this->m_linkMutex);
    if (this->m_linkFd >= 0) {
        this->m_model.setPhysicalLinkConnected(true);
        return true;
    }

    bool opened = false;
    if (this->m_config.linkMode == CommExternalLinkMode::TCP_SERVER) {
        opened = this->ensureTcpListenOpen_() && acceptTcpClient(this->m_tcpListenFd, timeoutMs, this->m_linkFd);
    } else {
        opened = openRawSerial(this->m_config.serialDevice, this->m_config.baudrate, this->m_linkFd);
    }
    this->m_model.setPhysicalLinkConnected(opened);
    if (opened && this->m_config.linkMode == CommExternalLinkMode::SERIAL) {
        this->m_serialIngressFilter.onLinkOpened();
    }
    if (opened && ingressDiagnosticsEnabled()) {
        std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                  << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                  << " external-link-open=1"
                  << " fd=" << this->m_linkFd
                  << "\n";
        std::cout.flush();
    }
    return opened;
}

bool CommNodeServer::writeExternalLink_(const std::uint8_t* data, std::size_t size) {
    std::lock_guard<std::mutex> linkLock(this->m_linkMutex);
    return this->m_linkFd >= 0 && writeAll(this->m_linkFd, data, size);
}

bool CommNodeServer::ensureTcpListenOpen_() {
    if (this->m_tcpListenFd >= 0) {
        return true;
    }
    return openTcpServer(this->m_config.tcpListenHost, this->m_config.tcpListenPort, this->m_tcpListenFd);
}

void CommNodeServer::closeExternalLink_() {
    {
        std::lock_guard<std::mutex> linkLock(this->m_linkMutex);
        closeFd(this->m_linkFd);
        this->m_model.setPhysicalLinkConnected(false);
        if (this->m_config.linkMode == CommExternalLinkMode::SERIAL) {
            this->m_serialIngressFilter.onLinkClosed();
        }
    }
    this->handleDownlinkV2Disconnect_();
    this->handleDownlinkV3Disconnect_();
}

void CommNodeServer::closeTcpListen_() {
    closeFd(this->m_tcpListenFd);
}

bool CommNodeServer::ensureBeaconLinkOpen_() {
    std::lock_guard<std::mutex> lock(this->m_beaconLinkMutex);
    if (this->m_config.beaconSerialDevice.empty()) {
        return false;
    }
    if (this->m_beaconFd >= 0) {
        return true;
    }
    int fd = -1;
    if (!openRawSerial(this->m_config.beaconSerialDevice, this->m_config.beaconBaudrate, fd)) {
        return false;
    }
    if (!setFdNonBlocking(fd)) {
        closeFd(fd);
        return false;
    }
    this->m_beaconFd = fd;
    return true;
}

void CommNodeServer::closeBeaconLink_() {
    std::lock_guard<std::mutex> lock(this->m_beaconLinkMutex);
    closeFd(this->m_beaconFd);
}

void CommNodeServer::stopBeaconWriter_() {
    {
        std::lock_guard<std::mutex> lock(this->m_beaconMutex);
        this->m_beaconWriterRunning = false;
    }
    this->m_beaconCv.notify_all();
    if (this->m_beaconWriter.joinable()) {
        this->m_beaconWriter.join();
    }
    {
        std::lock_guard<std::mutex> lock(this->m_beaconMutex);
        this->m_pendingBeaconFrame.clear();
        this->m_beaconFramePending = false;
    }
}

bool CommNodeServer::enqueueBeaconFrame_(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(this->m_beaconMutex);
        if (!this->m_beaconWriterRunning) {
            return false;
        }
        this->m_pendingBeaconFrame.assign(data, data + size);
        this->m_beaconFramePending = true;
    }
    this->m_beaconCv.notify_one();
    return true;
}

void CommNodeServer::beaconWriterLoop_() {
    std::vector<std::uint8_t> frame;
    std::size_t offset = 0U;

    while (true) {
        if (offset >= frame.size()) {
            std::unique_lock<std::mutex> lock(this->m_beaconMutex);
            this->m_beaconCv.wait(lock, [this] {
                return !this->m_beaconWriterRunning || this->m_beaconFramePending;
            });
            if (!this->m_beaconWriterRunning && !this->m_beaconFramePending) {
                break;
            }
            frame = this->m_pendingBeaconFrame;
            this->m_pendingBeaconFrame.clear();
            this->m_beaconFramePending = false;
            offset = 0U;
        }

        {
            std::lock_guard<std::mutex> lock(this->m_beaconMutex);
            if (!this->m_beaconWriterRunning) {
                break;
            }
            if (this->m_beaconFramePending) {
                if (offset > 0U) {
                    this->closeBeaconLink_();
                }
                frame.clear();
                offset = 0U;
                continue;
            }
        }

        if (!this->ensureBeaconLinkOpen_()) {
            frame.clear();
            offset = 0U;
            continue;
        }

        struct pollfd pollFd = {};
        pollFd.fd = this->m_beaconFd;
        pollFd.events = POLLOUT;
        const int pollStatus = ::poll(&pollFd, 1, kBeaconWriterPollTimeoutMs);
        if (pollStatus < 0) {
            if (errno == EINTR) {
                continue;
            }
            this->closeBeaconLink_();
            frame.clear();
            offset = 0U;
            continue;
        }
        if (pollStatus == 0 || (pollFd.revents & POLLOUT) == 0) {
            if (offset > 0U) {
                this->closeBeaconLink_();
            }
            frame.clear();
            offset = 0U;
            continue;
        }

        const ssize_t written = ::write(this->m_beaconFd, frame.data() + offset, frame.size() - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (offset > 0U) {
                    this->closeBeaconLink_();
                }
                frame.clear();
                offset = 0U;
                continue;
            }
            this->closeBeaconLink_();
            frame.clear();
            offset = 0U;
            continue;
        }
        if (written == 0) {
            this->closeBeaconLink_();
            frame.clear();
            offset = 0U;
            continue;
        }
        offset += static_cast<std::size_t>(written);
    }
}

void CommNodeServer::pollExternalIngress_(std::uint32_t timeoutMs) {
    if (!this->ensureExternalLinkOpen_(timeoutMs)) {
        return;
    }

    std::uint8_t buffer[CSP::MAX_CHUNK_BYTES] = {};
    std::size_t bytesRead = 0U;
    int linkFd = -1;
    {
        std::lock_guard<std::mutex> linkLock(this->m_linkMutex);
        linkFd = this->m_linkFd;
    }
    const StreamReadStatus status = readSome(linkFd, timeoutMs, buffer, sizeof(buffer), bytesRead);
    if (status == StreamReadStatus::TIMEOUT) {
        return;
    }
    if (status == StreamReadStatus::CLOSED || status == StreamReadStatus::IO_ERROR) {
        this->m_model.recordRxError();
        if (ingressDiagnosticsEnabled()) {
            const CommSimStatus diagStatus = this->m_model.status();
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " read-status=" << streamReadStatusName(status)
                      << " rx-errors=" << diagStatus.rxErrors
                      << " rx-chunks=" << diagStatus.rxChunks
                      << " uplink-queue-depth=" << diagStatus.uplinkQueueDepth
                      << "\n";
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " external-link-close=1"
                      << " reason=read-" << streamReadStatusName(status)
                      << "\n";
            std::cout.flush();
        }
        this->closeExternalLink_();
        return;
    }

    if (bytesRead == 0U) {
        return;
    }

    if (this->m_config.linkMode == CommExternalLinkMode::SERIAL && stripTcFillPatternEnabled() &&
        isRepeatedPattern(buffer, bytesRead, kTcFillPattern)) {
        if (ingressDiagnosticsEnabled()) {
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=serial"
                      << " bytes-read=" << bytesRead
                      << " dropped-fill-pattern=1"
                      << " preview=" << hexPreview(buffer, bytesRead) << "\n";
            std::cout.flush();
        }
        return;
    }

    const bool acquisitionFilterActiveBefore =
        this->m_config.linkMode == CommExternalLinkMode::SERIAL && this->m_serialIngressFilter.active();
    std::vector<std::uint8_t> filteredBytes;
    if (this->m_config.linkMode == CommExternalLinkMode::SERIAL) {
        filteredBytes = this->m_serialIngressFilter.filter(buffer, bytesRead);
        if (filteredBytes.empty() && acquisitionFilterActiveBefore) {
            if (ingressDiagnosticsEnabled()) {
                const CommSimStatus diagStatus = this->m_model.status();
                std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                          << " endpoint=serial"
                          << " bytes-read=" << bytesRead
                          << " dropped-gateway-preamble=1"
                          << " rx-errors=" << diagStatus.rxErrors
                          << " rx-chunks=" << diagStatus.rxChunks
                          << " uplink-queue-depth=" << diagStatus.uplinkQueueDepth
                          << " preview=" << hexPreview(buffer, bytesRead) << "\n";
                std::cout.flush();
            }
            return;
        }
    }

    const std::uint8_t* ingressData = buffer;
    std::size_t ingressSize = bytesRead;
    if (!filteredBytes.empty()) {
        ingressData = filteredBytes.data();
        ingressSize = filteredBytes.size();
    }

    const bool accepted = this->m_model.ingestUplinkBytes(ingressData, ingressSize);
    if (ingressDiagnosticsEnabled()) {
        const CommSimStatus diagStatus = this->m_model.status();
        std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                  << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                  << " bytes-read=" << bytesRead
                  << " ingress-bytes=" << ingressSize
                  << " accepted=" << (accepted ? 1 : 0)
                  << " rx-errors=" << diagStatus.rxErrors
                  << " rx-chunks=" << diagStatus.rxChunks
                  << " uplink-queue-depth=" << diagStatus.uplinkQueueDepth
                  << " preview=" << hexPreview(ingressData, ingressSize)
                  << "\n";
        if (ingressTraceFullEnabled()) {
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " ingress-full-hex bytes=" << ingressSize
                      << " preview=" << hexPreview(ingressData, ingressSize, ingressSize)
                      << "\n";
        }
        std::cout.flush();
    }
}

void CommNodeServer::handleConnection_(csp_conn_t* conn) {
    while (this->m_running.load()) {
        this->reapDownlinkV2Timeout_();
        this->reapDownlinkV3Timeout_();
        this->pollExternalIngress_(0U);

        csp_packet_t* const packet = csp_read(conn, 50);
        if (packet == nullptr) {
            break;
        }

        const int destinationPort = packet->id.dport;
        if (destinationPort == static_cast<int>(CSP::ServicePort::BEACON_PUSH)) {
            this->handleBeaconPush_(packet);
            break;
        }
        if (destinationPort == static_cast<int>(CSP::ServicePort::RELIABLE_TRANSFER_DATA)) {
            this->handleReliableTransferData_(packet);
            break;
        }
        if (destinationPort == static_cast<int>(CSP::ServicePort::DOWNLINK_DATA_V3)) {
            this->handleDownlinkDataV3_(packet);
            break;
        }
        if (destinationPort < static_cast<int>(CSP::ServicePort::UPLINK_POLL) ||
            destinationPort > static_cast<int>(CSP::ServicePort::DOWNLINK_DATA_V3)) {
            csp_service_handler(packet);
            break;
        }

        switch (static_cast<CSP::ServicePort>(destinationPort)) {
            case CSP::ServicePort::UPLINK_POLL: {
                if (packet->length != sizeof(CSP::UplinkPollRequest)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::UplinkPollRequest request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleUplinkPoll_(conn, request);
                return;
            }
            case CSP::ServicePort::DOWNLINK_WRITE: {
                if (packet->length != sizeof(CSP::DownlinkWriteRequest)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::DownlinkWriteRequest request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleDownlinkWrite_(conn, request);
                return;
            }
            case CSP::ServicePort::LINK_STATUS: {
                if (packet->length != sizeof(CSP::LinkStatusRequest)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::LinkStatusRequest request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleLinkStatus_(conn, request);
                return;
            }
            case CSP::ServicePort::DOWNLINK_STAGE_V2: {
                if (packet->length != sizeof(CSP::DownlinkStageV2Request)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::DownlinkStageV2Request request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleDownlinkStageV2_(conn, request);
                return;
            }
            case CSP::ServicePort::DOWNLINK_STATUS_V2: {
                if (packet->length != sizeof(CSP::DownlinkStatusV2Request)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::DownlinkStatusV2Request request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleDownlinkStatusV2_(conn, request);
                return;
            }
            case CSP::ServicePort::DOWNLINK_ABORT_V2: {
                if (packet->length != sizeof(CSP::DownlinkAbortV2Request)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::DownlinkAbortV2Request request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleDownlinkAbortV2_(conn, request);
                return;
            }
            case CSP::ServicePort::DOWNLINK_CONTROL_V3: {
                if (packet->length != sizeof(CSP::DownlinkControlV3Request)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::DownlinkControlV3Request request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                this->handleDownlinkControlV3_(conn, packet, request);
                return;
            }
            case CSP::ServicePort::RELIABLE_TRANSFER_CONTROL: {
                if (packet->length != sizeof(CSP::ReliableTransferControlRequest)) {
                    csp_buffer_free(packet);
                    continue;
                }
                CSP::ReliableTransferControlRequest request = {};
                std::memcpy(&request, packet->data, sizeof(request));
                csp_buffer_free(packet);
                this->handleReliableTransferControl_(conn, request);
                return;
            }
            default:
                csp_buffer_free(packet);
                return;
        }
    }
}

void CommNodeServer::handleUplinkPoll_(csp_conn_t* conn, const CSP::UplinkPollRequest& request) {
    const CSP::ChunkReply reply = this->m_model.handleUplinkPoll(request);
    this->sendChunkReply_(conn, reply);
}

void CommNodeServer::handleDownlinkWrite_(csp_conn_t* conn, const CSP::DownlinkWriteRequest& request) {
    CSP::ChunkReply reply = this->m_model.validateDownlinkWriteRequest(request);
    if (reply.header.result != static_cast<std::uint8_t>(CSP::ResultCode::OK)) {
        this->sendChunkReply_(conn, reply);
        return;
    }

    if (this->m_model.shouldAttemptSerialDownlink() && !this->ensureExternalLinkOpen_(0U)) {
        bool shouldWrite = false;
        reply = this->m_model.beginDownlinkWrite(request, shouldWrite);
        this->sendChunkReply_(conn, reply);
        return;
    }

    bool shouldWrite = false;
    reply = this->m_model.beginDownlinkWrite(request, shouldWrite);
    if (!shouldWrite) {
        this->sendChunkReply_(conn, reply);
        return;
    }

    if (!this->writeExternalLink_(request.data, request.byteCount)) {
        this->m_model.completeDownlinkWrite(false);
        if (ingressDiagnosticsEnabled()) {
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " external-link-close=1"
                      << " reason=downlink-write-failure"
                      << "\n";
            std::cout.flush();
        }
        this->closeExternalLink_();
        reply.header.result = static_cast<std::uint8_t>(CSP::ResultCode::IO_ERROR);
        reply.header.flags = this->m_model.linkFlags();
        this->sendChunkReply_(conn, reply);
        return;
    }

    this->m_model.completeDownlinkWrite(true);
    reply.header.flags = this->m_model.linkFlags();
    this->sendChunkReply_(conn, reply);
}

void CommNodeServer::handleLinkStatus_(csp_conn_t* conn, const CSP::LinkStatusRequest& request) {
    const CSP::LinkStatusReply reply = this->m_model.handleLinkStatus(request);
    this->sendLinkStatusReply_(conn, reply);
}

void CommNodeServer::handleDownlinkStageV2_(csp_conn_t* conn, const CSP::DownlinkStageV2Request& request) {
    CSP::DownlinkStageV2Reply reply =
        CSP::makeDownlinkStageV2Reply(request.header.seq, CSP::ResultCode::INVALID_REQUEST);
    reply.streamId = request.streamId;
    if (!this->downlinkV2Enabled_()) {
        this->sendDownlinkStageV2Reply_(conn, reply);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
        bool acceptedNewChunk = false;
        reply = this->m_downlinkV2State.handleStage(request, this->m_model.linkFlags(), &acceptedNewChunk);
        if (reply.header.result == static_cast<std::uint8_t>(CSP::ResultCode::OK) && acceptedNewChunk) {
            this->m_model.completeDownlinkWrite(true);
        } else if (reply.header.result != static_cast<std::uint8_t>(CSP::ResultCode::NO_CREDIT)) {
            this->m_model.completeDownlinkWrite(false);
        }
    }
    this->notifyDownlinkDrainWorker_();
    this->sendDownlinkStageV2Reply_(conn, reply);
}

void CommNodeServer::handleDownlinkStatusV2_(csp_conn_t* conn, const CSP::DownlinkStatusV2Request& request) {
    CSP::DownlinkStatusV2Reply reply =
        CSP::makeDownlinkStatusV2Reply(request.header.seq, CSP::ResultCode::INVALID_REQUEST);
    if (this->downlinkV2Enabled_()) {
        std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
        reply = this->m_downlinkV2State.handleStatus(request, this->m_model.linkFlags());
    }
    this->sendDownlinkStatusV2Reply_(conn, reply);
}

void CommNodeServer::handleDownlinkAbortV2_(csp_conn_t* conn, const CSP::DownlinkAbortV2Request& request) {
    CSP::DownlinkAbortV2Reply reply =
        CSP::makeDownlinkAbortV2Reply(request.header.seq, CSP::ResultCode::INVALID_REQUEST);
    reply.streamId = request.streamId;
    if (this->downlinkV2Enabled_()) {
        std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
        reply = this->m_downlinkV2State.handleAbort(request, this->m_model.linkFlags());
    }
    this->sendDownlinkAbortV2Reply_(conn, reply);
}

void CommNodeServer::handleDownlinkControlV3_(csp_conn_t* conn,
                                              csp_packet_t* requestPacket,
                                              const CSP::DownlinkControlV3Request& request) {
    CSP::DownlinkControlV3Reply reply = CSP::makeDownlinkControlV3Reply(
        static_cast<CSP::DownlinkControlV3Op>(request.op), request.header.seq, CSP::ResultCode::INVALID_REQUEST);
    if (this->downlinkV3Enabled_()) {
        std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
        reply = this->m_downlinkV3State.handleControl(request, this->m_model.linkFlags());
    }
    if (ingressDiagnosticsEnabled()) {
        std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                  << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                  << " downlink-v3-control-op=" << downlinkControlV3OpName(static_cast<CSP::DownlinkControlV3Op>(request.op))
                  << " stream-id=" << request.streamId
                  << " total-frames=" << request.totalFrames
                  << " total-bytes=" << request.totalBytes
                  << " committed-frames=" << request.committedFrames
                  << " committed-bytes=" << request.committedBytes
                  << " result=" << downlinkResultCodeName(static_cast<CSP::ResultCode>(reply.header.result))
                  << " reply-contiguous-frames=" << reply.contiguousFrames
                  << " reply-contiguous-bytes=" << reply.contiguousBytes
                  << " reply-window-credit=" << reply.windowCredit
                  << " reply-drain-queued-frames=" << reply.drainQueuedFrames
                  << "\n";
        std::cout.flush();
    }
    this->notifyDownlinkDrainWorker_();
    this->sendDownlinkControlV3Reply_(conn, requestPacket, reply);
}

void CommNodeServer::handleDownlinkDataV3_(csp_packet_t* packet) {
    if (packet == nullptr) {
        return;
    }

    CSP::DownlinkDataV3Frame frame = {};
    const std::uint16_t packetLength = packet->length;
    const bool parsed = this->downlinkV3Enabled_() &&
                        CSP::deserializeDownlinkDataV3Frame(packet->data, packet->length, frame);
    if (!parsed) {
        if (ingressDiagnosticsEnabled()) {
            CSP::DownlinkDataV3Frame previewFrame = {};
            const bool havePreview = peekDownlinkDataV3FrameHeader(packet->data, packet->length, previewFrame);
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " downlink-v3-data-drop=1"
                      << " packet-length=" << packetLength
                      << " min-length=" << CSP::DOWNLINK_V3_FRAME_METADATA_BYTES
                      << " max-length=" << sizeof(CSP::DownlinkDataV3Frame)
                      << " preview=" << hexPreview(packet->data, packetLength)
                      << " peek-valid=" << (havePreview ? 1 : 0);
            if (havePreview) {
                std::cout << " peek-version=" << static_cast<unsigned int>(previewFrame.version)
                          << " peek-kind=" << static_cast<unsigned int>(previewFrame.kind)
                          << " peek-stream-id=" << previewFrame.streamId
                          << " peek-frame-index=" << previewFrame.frameIndex
                          << " peek-frame-count=" << previewFrame.frameCount
                          << " peek-byte-offset=" << previewFrame.byteOffset
                          << " peek-byte-count=" << previewFrame.byteCount;
                if (previewFrame.byteCount <= CSP::DOWNLINK_V3_MAX_DATA_BYTES) {
                    std::cout << " peek-expected-length="
                              << CSP::downlinkDataV3SerializedSize(previewFrame.byteCount);
                } else {
                    std::cout << " peek-expected-length=invalid";
                }
            }
            std::cout
                      << "\n";
            std::cout.flush();
        }
        csp_buffer_free(packet);
        return;
    }
    csp_buffer_free(packet);

    bool accepted = false;
    {
        std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
        accepted = this->m_downlinkV3State.handleData(frame);
    }
    if (ingressDiagnosticsEnabled()) {
        std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                  << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                  << " downlink-v3-data-stream=" << frame.streamId
                  << " frame-index=" << frame.frameIndex
                  << " frame-count=" << frame.frameCount
                  << " byte-offset=" << frame.byteOffset
                  << " byte-count=" << frame.byteCount
                  << " accepted=" << (accepted ? 1 : 0)
                  << "\n";
        std::cout.flush();
    }
    if (accepted) {
        this->m_model.completeDownlinkWrite(true);
        this->notifyDownlinkDrainWorker_();
    }
}

void CommNodeServer::handleReliableTransferData_(csp_packet_t* packet) {
    if (!this->m_reliableTransfer.enabled || packet == nullptr) {
        if (packet != nullptr) {
            csp_buffer_free(packet);
        }
        return;
    }
    if (packet->length != sizeof(CSP::ReliableTransferDataFrame)) {
        csp_buffer_free(packet);
        return;
    }

    CSP::ReliableTransferDataFrame frame = {};
    std::memcpy(&frame, packet->data, sizeof(frame));
    csp_buffer_free(packet);

    if (!this->m_reliableTransfer.active || frame.version != CSP::VERSION ||
        frame.kind != CSP::RELIABLE_DATA_PACKET ||
        frame.transferId != this->m_reliableTransfer.transferId ||
        frame.packetSize == 0U || frame.packetSize > CSP::MAX_RELIABLE_PACKET_BYTES) {
        return;
    }

    Fw::FilePacket packetValue = {};
    if (!decodeFilePacket(frame.packetBytes, frame.packetSize, packetValue) ||
        packetValue.asHeader().getType() != Fw::FilePacket::T_DATA) {
        return;
    }

    const Fw::FilePacket::DataPacket& dataPacket = packetValue.asDataPacket();
    const std::uint32_t sequenceIndex = dataPacket.asHeader().getSequenceIndex();
    if (sequenceIndex == 0U) {
        return;
    }
    const std::uint32_t segmentIndex = sequenceIndex - 1U;
    if (segmentIndex >= this->m_reliableTransfer.segmentCount) {
        return;
    }

    const std::uint64_t expectedOffset =
        static_cast<std::uint64_t>(segmentIndex) * static_cast<std::uint64_t>(CSP::RELIABLE_SEGMENT_DATA_BYTES);
    if (expectedOffset >= this->m_reliableTransfer.fileSize) {
        return;
    }
    const std::uint32_t expectedSize = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(CSP::RELIABLE_SEGMENT_DATA_BYTES,
                                static_cast<std::uint64_t>(this->m_reliableTransfer.fileSize) - expectedOffset));
    const std::uint64_t actualEndOffset =
        static_cast<std::uint64_t>(dataPacket.getByteOffset()) + static_cast<std::uint64_t>(dataPacket.getDataSize());
    if (dataPacket.getByteOffset() != expectedOffset || dataPacket.getDataSize() != expectedSize ||
        actualEndOffset > this->m_reliableTransfer.fileSize) {
        return;
    }

    if (this->m_reliableTransfer.receivedSegments[segmentIndex]) {
        this->m_reliableTransfer.duplicateSegments += 1U;
        return;
    }

    if (!writeFilePacketData(this->m_reliableTransfer.file, dataPacket)) {
        return;
    }

    this->m_reliableTransfer.receivedSegments[segmentIndex] = true;
    while (this->m_reliableTransfer.contiguousSegments < this->m_reliableTransfer.segmentCount &&
           this->m_reliableTransfer.receivedSegments[this->m_reliableTransfer.contiguousSegments]) {
        this->m_reliableTransfer.contiguousSegments += 1U;
    }
    this->m_reliableTransfer.committedBytes =
        std::min(this->m_reliableTransfer.fileSize,
                 this->m_reliableTransfer.contiguousSegments * static_cast<std::uint32_t>(CSP::RELIABLE_SEGMENT_DATA_BYTES));
    this->m_reliableTransfer.lastActivity = std::chrono::steady_clock::now();
}

void CommNodeServer::handleReliableTransferControl_(csp_conn_t* conn, const CSP::ReliableTransferControlRequest& request) {
    const CSP::ReliableTransferOp op = static_cast<CSP::ReliableTransferOp>(request.op);
    CSP::ReliableTransferControlReply reply =
        CSP::makeReliableTransferControlReply(request.header.seq, CSP::ResultCode::OK, op);
    reply.transferId = request.transferId;
    reply.contiguousSegments = this->m_reliableTransfer.contiguousSegments;
    reply.committedBytes = this->m_reliableTransfer.committedBytes;
    reply.duplicateSegments = this->m_reliableTransfer.duplicateSegments;

    std::cout << "COMM reliable transfer control: node=" << this->m_config.nodeId
              << " op=" << static_cast<unsigned int>(request.op)
              << " transferId=" << request.transferId
              << " packetSize=" << request.packetSize
              << " segmentCount=" << request.segmentCount
              << " active=" << (this->m_reliableTransfer.active ? 1 : 0)
              << "\n";
    std::cout.flush();

    if (!this->m_reliableTransfer.enabled) {
        reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
        std::cout << "COMM reliable transfer control rejected: receiver disabled" << "\n";
        std::cout.flush();
        this->sendReliableTransferControlReply_(conn, reply);
        return;
    }

    if (op == CSP::ReliableTransferOp::BEGIN) {
        if (this->m_reliableTransfer.active &&
            this->m_reliableTransfer.lastActivity != std::chrono::steady_clock::time_point{} &&
            (std::chrono::steady_clock::now() - this->m_reliableTransfer.lastActivity) >
                std::chrono::milliseconds(this->m_reliableTransfer.staleTimeoutMs)) {
            std::cout << "COMM reliable transfer begin reaping stale active transfer: transferId="
                      << this->m_reliableTransfer.transferId << "\n";
            std::cout.flush();
            this->resetReliableTransferReceiver_(true);
        }
        if (this->m_reliableTransfer.active) {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::BUSY);
            std::cout << "COMM reliable transfer begin rejected: receiver already active transferId="
                      << this->m_reliableTransfer.transferId << "\n";
            std::cout.flush();
            this->sendReliableTransferControlReply_(conn, reply);
            return;
        }

        Fw::FilePacket startValue = {};
        if (request.packetSize == 0U || request.packetSize > CSP::MAX_RELIABLE_PACKET_BYTES ||
            !decodeFilePacket(request.packetBytes, request.packetSize, startValue) ||
            startValue.asHeader().getType() != Fw::FilePacket::T_START) {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
            std::cout << "COMM reliable transfer begin rejected: invalid start packet packetSize="
                      << request.packetSize << "\n";
            std::cout.flush();
            this->sendReliableTransferControlReply_(conn, reply);
            return;
        }

        const Fw::FilePacket::StartPacket& startPacket = startValue.asStartPacket();
        const std::uint32_t expectedSegmentCount =
            startPacket.getFileSize() == 0U
                ? 0U
                : ((startPacket.getFileSize() - 1U) / static_cast<std::uint32_t>(CSP::RELIABLE_SEGMENT_DATA_BYTES)) + 1U;
        const bool zeroLengthShapeMismatch =
            (startPacket.getFileSize() == 0U && request.segmentCount != 0U) ||
            (startPacket.getFileSize() > 0U && request.segmentCount == 0U);
        const bool segmentCountMismatch = request.segmentCount != expectedSegmentCount;
        if (startPacket.getFileSize() > CSP::MAX_RELIABLE_TRANSFER_FILE_BYTES ||
            request.segmentCount > CSP::MAX_RELIABLE_TRANSFER_SEGMENTS ||
            zeroLengthShapeMismatch ||
            segmentCountMismatch) {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
            std::cout << "COMM reliable transfer begin rejected: bounded ceiling exceeded"
                      << " fileSize=" << startPacket.getFileSize()
                      << " segmentCount=" << request.segmentCount
                      << " expectedSegmentCount=" << expectedSegmentCount
                      << " maxFileBytes=" << CSP::MAX_RELIABLE_TRANSFER_FILE_BYTES
                      << " maxSegments=" << CSP::MAX_RELIABLE_TRANSFER_SEGMENTS << "\n";
            std::cout.flush();
            this->sendReliableTransferControlReply_(conn, reply);
            return;
        }
        const std::string destName = baseName(startPacket.getDestinationPath().getValue());
        const std::string finalPath = this->m_reliableTransfer.outputDir + "/" + destName;
        const std::string tempPath = finalPath + ".part." + std::to_string(static_cast<unsigned int>(request.transferId));
        (void)Os::FileSystem::createDirectory(this->m_reliableTransfer.outputDir.c_str(), false);
        std::remove(tempPath.c_str());

        this->m_reliableTransfer.file.open(tempPath.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!this->m_reliableTransfer.file.is_open()) {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::IO_ERROR);
            std::cout << "COMM reliable transfer begin rejected: failed to open temp file path=" << tempPath << "\n";
            std::cout.flush();
            this->sendReliableTransferControlReply_(conn, reply);
            return;
        }

        this->m_reliableTransfer.active = true;
        this->m_reliableTransfer.transferId = request.transferId;
        this->m_reliableTransfer.fileSize = startPacket.getFileSize();
        this->m_reliableTransfer.segmentCount = request.segmentCount;
        this->m_reliableTransfer.duplicateSegments = 0U;
        this->m_reliableTransfer.contiguousSegments = 0U;
        this->m_reliableTransfer.committedBytes = 0U;
        this->m_reliableTransfer.ackVisibleContiguousSegments = 0U;
        this->m_reliableTransfer.ackVisibleCommittedBytes = 0U;
        this->m_reliableTransfer.noProgressPollsRemaining = parseEnvU32("COMM_RT_ACK_TIMEOUT_POLLS", 0U);
        this->m_reliableTransfer.tempPath = tempPath;
        this->m_reliableTransfer.finalPath = finalPath;
        this->m_reliableTransfer.receivedSegments.assign(request.segmentCount, false);
        std::copy(std::begin(request.sha256), std::end(request.sha256), this->m_reliableTransfer.sha256.begin());
        this->m_reliableTransfer.lastActivity = std::chrono::steady_clock::now();
        reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::OK);
        std::cout << "COMM reliable transfer begin accepted: transferId=" << request.transferId
                  << " finalPath=" << finalPath
                  << " tempPath=" << tempPath
                  << " segmentCount=" << request.segmentCount
                  << "\n";
        std::cout.flush();
        this->sendReliableTransferControlReply_(conn, reply);
        return;
    }

    if (!this->m_reliableTransfer.active || request.transferId != this->m_reliableTransfer.transferId) {
        reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
        std::cout << "COMM reliable transfer control rejected: inactive or transferId mismatch active="
                  << (this->m_reliableTransfer.active ? 1 : 0)
                  << " expectedTransferId=" << this->m_reliableTransfer.transferId
                  << " requestTransferId=" << request.transferId
                  << "\n";
        std::cout.flush();
        this->sendReliableTransferControlReply_(conn, reply);
        return;
    }

    if (op == CSP::ReliableTransferOp::ACK_POLL) {
        this->m_reliableTransfer.lastActivity = std::chrono::steady_clock::now();
        if (this->m_reliableTransfer.noProgressPollsRemaining > 0U) {
            this->m_reliableTransfer.noProgressPollsRemaining -= 1U;
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::NO_PROGRESS);
            reply.contiguousSegments = this->m_reliableTransfer.ackVisibleContiguousSegments;
            reply.committedBytes = this->m_reliableTransfer.ackVisibleCommittedBytes;
        } else {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::OK);
            this->m_reliableTransfer.ackVisibleContiguousSegments = this->m_reliableTransfer.contiguousSegments;
            this->m_reliableTransfer.ackVisibleCommittedBytes = this->m_reliableTransfer.committedBytes;
            reply.contiguousSegments = this->m_reliableTransfer.ackVisibleContiguousSegments;
            reply.committedBytes = this->m_reliableTransfer.ackVisibleCommittedBytes;
        }
        reply.duplicateSegments = this->m_reliableTransfer.duplicateSegments;
        std::cout << "COMM reliable transfer ack reply: transferId=" << request.transferId
                  << " result=" << static_cast<unsigned int>(reply.transferResult)
                  << " contiguousSegments=" << reply.contiguousSegments
                  << " committedBytes=" << reply.committedBytes
                  << " duplicateSegments=" << reply.duplicateSegments
                  << "\n";
        std::cout.flush();
        this->sendReliableTransferControlReply_(conn, reply);
        return;
    }

    if (op == CSP::ReliableTransferOp::COMPLETE) {
        Fw::FilePacket endValue = {};
        if (request.packetSize == 0U || request.packetSize > CSP::MAX_RELIABLE_PACKET_BYTES ||
            !decodeFilePacket(request.packetBytes, request.packetSize, endValue) ||
            endValue.asHeader().getType() != Fw::FilePacket::T_END) {
            reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
            this->sendReliableTransferControlReply_(conn, reply);
            return;
        }

        CFDP::Checksum endChecksum;
        endValue.asEndPacket().getChecksum(endChecksum);
        this->m_reliableTransfer.file.flush();
        this->m_reliableTransfer.file.close();

        std::array<std::uint8_t, 32> actualSha256 = {};
        const bool shaOk = computeFileSha256(this->m_reliableTransfer.tempPath, actualSha256);
        const bool checksumOk = computeFileChecksum(this->m_reliableTransfer.tempPath) == endChecksum.getValue();
        const bool hashOk = shaOk &&
                            std::equal(actualSha256.begin(), actualSha256.end(), this->m_reliableTransfer.sha256.begin());
        if (this->m_reliableTransfer.contiguousSegments != this->m_reliableTransfer.segmentCount || !checksumOk || !hashOk) {
            reply.transferResult = static_cast<std::uint8_t>(hashOk ? CSP::ReliableTransferResult::INVALID
                                                                    : CSP::ReliableTransferResult::HASH_MISMATCH);
            std::remove(this->m_reliableTransfer.tempPath.c_str());
        } else {
            std::remove(this->m_reliableTransfer.finalPath.c_str());
            if (std::rename(this->m_reliableTransfer.tempPath.c_str(), this->m_reliableTransfer.finalPath.c_str()) != 0) {
                reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::IO_ERROR);
                std::remove(this->m_reliableTransfer.tempPath.c_str());
            } else {
                reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::OK);
            }
        }
        std::cout << "COMM reliable transfer complete: transferId=" << request.transferId
                  << " result=" << static_cast<unsigned int>(reply.transferResult)
                  << " contiguousSegments=" << this->m_reliableTransfer.contiguousSegments
                  << " segmentCount=" << this->m_reliableTransfer.segmentCount
                  << " committedBytes=" << this->m_reliableTransfer.committedBytes
                  << "\n";
        std::cout.flush();
    } else if (op == CSP::ReliableTransferOp::CANCEL || op == CSP::ReliableTransferOp::ABORT) {
        this->m_reliableTransfer.file.close();
        std::remove(this->m_reliableTransfer.tempPath.c_str());
        reply.transferResult = static_cast<std::uint8_t>(op == CSP::ReliableTransferOp::CANCEL
                                                             ? CSP::ReliableTransferResult::CANCELLED
                                                             : CSP::ReliableTransferResult::ABORTED);
        std::cout << "COMM reliable transfer closed: transferId=" << request.transferId
                  << " result=" << static_cast<unsigned int>(reply.transferResult)
                  << "\n";
        std::cout.flush();
    } else {
        reply.transferResult = static_cast<std::uint8_t>(CSP::ReliableTransferResult::INVALID);
        std::cout << "COMM reliable transfer control rejected: unsupported op=" << static_cast<unsigned int>(request.op)
                  << "\n";
        std::cout.flush();
    }

    reply.contiguousSegments = this->m_reliableTransfer.contiguousSegments;
    reply.committedBytes = this->m_reliableTransfer.committedBytes;
    reply.duplicateSegments = this->m_reliableTransfer.duplicateSegments;
    this->sendReliableTransferControlReply_(conn, reply);

    if (op == CSP::ReliableTransferOp::COMPLETE || op == CSP::ReliableTransferOp::CANCEL || op == CSP::ReliableTransferOp::ABORT) {
        this->resetReliableTransferReceiver_(false);
    }
}

void CommNodeServer::handleBeaconPush_(csp_packet_t* packet) {
    const std::uint16_t payloadSize = packet->length;
    const char* result = "EMPTY_PACKET";
    if (payloadSize > 0U) {
        if (this->m_config.beaconSerialDevice.empty()) {
            result = "LINK_UNCONFIGURED";
        } else if (!this->ensureBeaconLinkOpen_()) {
            result = "LINK_UNAVAILABLE";
        } else if (this->enqueueBeaconFrame_(packet->data, payloadSize)) {
            result = "OK";
        } else {
            result = "LINK_BACKPRESSURE";
        }
    }

    std::cout << "COMM beacon push: node=" << this->m_config.nodeId
              << " bytes=" << payloadSize
              << " result=" << result;
    if (!this->m_config.beaconSerialDevice.empty()) {
        std::cout << " beacon-serial=" << this->m_config.beaconSerialDevice
                  << " beacon-baudrate=" << this->m_config.beaconBaudrate;
    } else {
        std::cout << " beacon-serial=disabled";
    }
    std::cout << "\n";
    std::cout.flush();

    csp_buffer_free(packet);
}

void CommNodeServer::sendChunkReply_(csp_conn_t* conn, const CSP::ChunkReply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::sendLinkStatusReply_(csp_conn_t* conn, const CSP::LinkStatusReply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::sendDownlinkStageV2Reply_(csp_conn_t* conn, const CSP::DownlinkStageV2Reply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::sendDownlinkStatusV2Reply_(csp_conn_t* conn, const CSP::DownlinkStatusV2Reply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::sendDownlinkAbortV2Reply_(csp_conn_t* conn, const CSP::DownlinkAbortV2Reply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::sendDownlinkControlV3Reply_(csp_conn_t* conn,
                                                 csp_packet_t* requestPacket,
                                                 const CSP::DownlinkControlV3Reply& reply) {
    if (conn == nullptr) {
        if (requestPacket != nullptr) {
            csp_buffer_free(requestPacket);
        }
        return;
    }

    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        if (ingressDiagnosticsEnabled()) {
            std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                      << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                      << " downlink-v3-reply-drop=1"
                      << " reason=no-csp-buffer"
                      << " op=" << downlinkControlV3OpName(static_cast<CSP::DownlinkControlV3Op>(reply.op))
                      << "\n";
            std::cout.flush();
        }
        if (requestPacket != nullptr) {
            csp_buffer_free(requestPacket);
        }
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
    if (requestPacket != nullptr) {
        csp_buffer_free(requestPacket);
    }
}

void CommNodeServer::sendReliableTransferControlReply_(csp_conn_t* conn, const CSP::ReliableTransferControlReply& reply) {
    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        return;
    }

    packet->length = sizeof(reply);
    std::memcpy(packet->data, &reply, sizeof(reply));
    csp_send(conn, packet);
}

void CommNodeServer::resetReliableTransferReceiver_(bool removeTempFile) {
    this->m_reliableTransfer.file.close();
    if (removeTempFile && !this->m_reliableTransfer.tempPath.empty()) {
        std::remove(this->m_reliableTransfer.tempPath.c_str());
    }
    this->m_reliableTransfer.active = false;
    this->m_reliableTransfer.transferId = 0U;
    this->m_reliableTransfer.fileSize = 0U;
    this->m_reliableTransfer.segmentCount = 0U;
    this->m_reliableTransfer.duplicateSegments = 0U;
    this->m_reliableTransfer.contiguousSegments = 0U;
    this->m_reliableTransfer.committedBytes = 0U;
    this->m_reliableTransfer.ackVisibleContiguousSegments = 0U;
    this->m_reliableTransfer.ackVisibleCommittedBytes = 0U;
    this->m_reliableTransfer.noProgressPollsRemaining = 0U;
    this->m_reliableTransfer.receivedSegments.clear();
    this->m_reliableTransfer.tempPath.clear();
    this->m_reliableTransfer.finalPath.clear();
    this->m_reliableTransfer.lastActivity = std::chrono::steady_clock::time_point{};
    std::fill(this->m_reliableTransfer.sha256.begin(), this->m_reliableTransfer.sha256.end(), 0U);
}

bool CommNodeServer::downlinkV2Enabled_() const {
    return this->m_config.nodeId == CSP::DEFAULT_SBAND_COMM_NODE_ID;
}

bool CommNodeServer::downlinkV3Enabled_() const {
    return this->m_config.nodeId == CSP::DEFAULT_SBAND_COMM_NODE_ID;
}

void CommNodeServer::startDownlinkDrainWorker_() {
    if (this->downlinkV2Enabled_()) {
        {
            std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
            this->m_downlinkV2DrainWorkerRunning = true;
        }
        this->m_downlinkV2DrainWorker = std::thread(&CommNodeServer::downlinkDrainWorkerLoop_, this);
    }
    if (this->downlinkV3Enabled_()) {
        {
            std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
            this->m_downlinkV3DrainWorkerRunning = true;
        }
        this->m_downlinkV3DrainWorker = std::thread([this]() {
            const std::uint32_t drainDelayMs = parseEnvU32("COMM_NODE_DOWNLINK_V3_DRAIN_DELAY_MS", 0U);
            while (true) {
                CommNodeDownlinkV3State::DrainFrame frame = {};
                {
                    std::unique_lock<std::mutex> lock(this->m_downlinkV3Mutex);
                    this->m_downlinkV3State.reapTimedOut(std::chrono::steady_clock::now());
                    this->m_downlinkV3Cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                        return !this->m_downlinkV3DrainWorkerRunning || this->m_downlinkV3State.hasDrainWork();
                    });
                    if (!this->m_downlinkV3DrainWorkerRunning) {
                        break;
                    }
                    if (!this->m_downlinkV3State.peekDrainFrame(frame)) {
                        continue;
                    }
                }

                if (!this->ensureExternalLinkOpen_(100U)) {
                    continue;
                }

                if (!this->writeExternalLink_(frame.data.data(), frame.byteCount)) {
                    {
                        std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
                        this->m_downlinkV3State.handleDrainWriteFailure(frame.byteCount);
                    }
                    if (ingressDiagnosticsEnabled()) {
                        std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                                  << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                                  << " external-link-close=1"
                                  << " reason=downlink-v3-drain-write-failure"
                                  << "\n";
                        std::cout.flush();
                    }
                    this->closeExternalLink_();
                    continue;
                }

                if (drainDelayMs > 0U) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(drainDelayMs));
                }

                {
                    std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
                    static_cast<void>(this->m_downlinkV3State.confirmDrainFrameFlushed(frame));
                }
            }
        });
    }
}

void CommNodeServer::stopDownlinkDrainWorker_() {
    {
        std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
        this->m_downlinkV2DrainWorkerRunning = false;
    }
    {
        std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
        this->m_downlinkV3DrainWorkerRunning = false;
    }
    this->m_downlinkV2Cv.notify_all();
    this->m_downlinkV3Cv.notify_all();
    if (this->m_downlinkV2DrainWorker.joinable()) {
        this->m_downlinkV2DrainWorker.join();
    }
    if (this->m_downlinkV3DrainWorker.joinable()) {
        this->m_downlinkV3DrainWorker.join();
    }
}

void CommNodeServer::downlinkDrainWorkerLoop_() {
    const std::uint32_t drainDelayMs = parseEnvU32("COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS", 0U);
    while (true) {
        CommNodeDownlinkV2State::DrainChunk chunk = {};
        {
            std::unique_lock<std::mutex> lock(this->m_downlinkV2Mutex);
            this->m_downlinkV2State.reapTimedOut(std::chrono::steady_clock::now());
            this->m_downlinkV2Cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !this->m_downlinkV2DrainWorkerRunning || this->m_downlinkV2State.hasDrainWork();
            });
            if (!this->m_downlinkV2DrainWorkerRunning) {
                break;
            }
            if (!this->m_downlinkV2State.peekDrainChunk(chunk)) {
                continue;
            }
        }

        if (!this->ensureExternalLinkOpen_(100U)) {
            continue;
        }

        if (!this->writeExternalLink_(chunk.data.data(), chunk.byteCount)) {
            {
                std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
                this->m_downlinkV2State.handleDrainWriteFailure(chunk.byteCount);
            }
            if (ingressDiagnosticsEnabled()) {
                std::cout << "COMM ingress diagnostic: node=" << this->m_config.nodeId
                          << " endpoint=" << externalEndpointName(this->m_config.linkMode)
                          << " external-link-close=1"
                          << " reason=downlink-v2-drain-write-failure"
                          << "\n";
                std::cout.flush();
            }
            this->closeExternalLink_();
            continue;
        }

        if (drainDelayMs > 0U) {
            std::this_thread::sleep_for(std::chrono::milliseconds(drainDelayMs));
        }

        {
            std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
            static_cast<void>(this->m_downlinkV2State.confirmDrainChunkFlushed(chunk));
        }
    }
}

void CommNodeServer::notifyDownlinkDrainWorker_() {
    this->m_downlinkV2Cv.notify_one();
    this->m_downlinkV3Cv.notify_one();
}

void CommNodeServer::reapDownlinkV2Timeout_() {
    if (!this->downlinkV2Enabled_()) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
    this->m_downlinkV2State.reapTimedOut(std::chrono::steady_clock::now());
}

void CommNodeServer::handleDownlinkV2Disconnect_() {
    if (!this->downlinkV2Enabled_()) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->m_downlinkV2Mutex);
    this->m_downlinkV2State.handleDrainWriteFailure(0U);
}

void CommNodeServer::reapDownlinkV3Timeout_() {
    if (!this->downlinkV3Enabled_()) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
    this->m_downlinkV3State.reapTimedOut(std::chrono::steady_clock::now());
}

void CommNodeServer::handleDownlinkV3Disconnect_() {
    if (!this->downlinkV3Enabled_()) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->m_downlinkV3Mutex);
    this->m_downlinkV3State.handleDrainWriteFailure(0U);
}

}  // namespace COMM
}  // namespace OBC
