#ifndef OBC_SIMULATORS_COMM_COMMCSPPROTOCOL_HPP
#define OBC_SIMULATORS_COMM_COMMCSPPROTOCOL_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace OBC {
namespace COMM {
namespace CSP {

static constexpr std::uint8_t VERSION = 1U;
static constexpr std::uint8_t VERSION_V2 = 2U;
static constexpr std::uint8_t VERSION_V3 = 3U;
static constexpr std::uint16_t DEFAULT_OBC_NODE_ID = 1U;
static constexpr std::uint16_t DEFAULT_COMM_NODE_ID = 4U;
static constexpr std::uint16_t DEFAULT_GENERIC_COMM_NODE_ID = DEFAULT_COMM_NODE_ID;
static constexpr std::uint16_t DEFAULT_SBAND_COMM_NODE_ID = 5U;
static constexpr std::uint16_t DEFAULT_UHF_COMM_NODE_ID = 6U;
static constexpr std::size_t MAX_CHUNK_BYTES = 192U;
static constexpr std::size_t DOWNLINK_STAGE_V2_MAX_DATA_BYTES = 246U;
static constexpr std::size_t DOWNLINK_STAGE_V2_STAGING_SLOTS = 24U;
static constexpr std::size_t DOWNLINK_STAGE_V2_DRAIN_SLOTS = 64U;
static constexpr std::size_t RELIABLE_SEGMENT_DATA_BYTES = 160U;
static constexpr std::size_t MAX_RELIABLE_PACKET_BYTES = 176U;
static constexpr std::uint32_t MAX_RELIABLE_TRANSFER_SEGMENTS = 64U;
static constexpr std::uint32_t MAX_RELIABLE_TRANSFER_FILE_BYTES =
    static_cast<std::uint32_t>(RELIABLE_SEGMENT_DATA_BYTES) * MAX_RELIABLE_TRANSFER_SEGMENTS;
static constexpr std::size_t DOWNLINK_V3_MAX_DATA_BYTES = 2032U;
static constexpr std::size_t DOWNLINK_V3_STAGING_FRAME_LIMIT = 24U;
static constexpr std::size_t DOWNLINK_V3_DRAIN_FRAME_LIMIT = 32U;

enum class ServicePort : std::uint8_t {
    UPLINK_POLL = 30U,
    DOWNLINK_WRITE = 31U,
    LINK_STATUS = 32U,
    BEACON_PUSH = 33U,
    DOWNLINK_STAGE_V2 = 34U,
    DOWNLINK_STATUS_V2 = 35U,
    DOWNLINK_ABORT_V2 = 36U,
    RELIABLE_TRANSFER_DATA = 37U,
    RELIABLE_TRANSFER_CONTROL = 38U,
    DOWNLINK_CONTROL_V3 = 39U,
    DOWNLINK_DATA_V3 = 40U,
};

enum class ResultCode : std::uint8_t {
    OK = 0U,
    NO_CHUNK = 1U,
    INVALID_REQUEST = 2U,
    IO_ERROR = 3U,
    NO_CREDIT = 4U,
    BUSY = 5U,
    NO_PROGRESS = 6U,
};

enum LinkFlags : std::uint8_t {
    FLAG_LINK_CONNECTED = 0x01U,
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
    std::uint8_t flags;
    std::uint16_t byteCount;
};

struct UplinkPollRequest {
    RequestHeader header;
    std::uint16_t maxBytes;
    std::uint16_t reserved;
};

struct DownlinkWriteRequest {
    RequestHeader header;
    std::uint16_t byteCount;
    std::uint16_t reserved;
    std::uint8_t data[MAX_CHUNK_BYTES];
};

struct LinkStatusRequest {
    RequestHeader header;
    std::uint32_t reserved;
};

enum DownlinkStageV2Flags : std::uint8_t {
    FLAG_DOWNLINK_STAGE_V2_FIRST = 0x01U,
    FLAG_DOWNLINK_STAGE_V2_LAST = 0x02U,
};

struct DownlinkStageV2Request {
    RequestHeader header;
    std::uint16_t streamId;
    std::uint16_t byteCount;
    std::uint8_t flags;
    std::uint8_t reserved;
    std::uint8_t data[DOWNLINK_STAGE_V2_MAX_DATA_BYTES];
};

struct DownlinkStageV2Reply {
    ReplyHeader header;
    std::uint16_t streamId;
    std::uint16_t acceptedSeq;
    std::uint16_t stagedSlotsUsed;
    std::uint16_t drainFreeSlots;
};

struct DownlinkStatusV2Request {
    RequestHeader header;
    std::uint32_t reserved;
};

struct DownlinkStatusV2Reply {
    ReplyHeader header;
    std::uint16_t drainQueuedSlots;
    std::uint16_t drainFreeSlots;
    std::uint8_t stagingActive;
    std::uint8_t reserved;
    std::uint16_t stagingStreamId;
    std::uint32_t acceptedBytes;
    std::uint32_t flushedBytes;
    std::uint32_t droppedCommittedBytes;
};

struct DownlinkAbortV2Request {
    RequestHeader header;
    std::uint16_t streamId;
    std::uint16_t reserved;
};

struct DownlinkAbortV2Reply {
    ReplyHeader header;
    std::uint16_t streamId;
    std::uint16_t reserved;
};

enum class DownlinkControlV3Op : std::uint8_t {
    BEGIN = 1U,
    ACK_POLL = 2U,
    COMMIT = 3U,
    ABORT = 4U,
    STATUS = 5U,
};

struct DownlinkControlV3Request {
    RequestHeader header;
    std::uint8_t op;
    std::uint8_t reserved;
    std::uint16_t streamId;
    std::uint16_t totalFrames;
    std::uint32_t totalBytes;
    std::uint16_t committedFrames;
    std::uint16_t reserved2;
    std::uint32_t committedBytes;
};

struct DownlinkControlV3Reply {
    ReplyHeader header;
    std::uint8_t op;
    std::uint8_t stagingActive;
    std::uint16_t streamId;
    std::uint16_t contiguousFrames;
    std::uint16_t totalFrames;
    std::uint32_t contiguousBytes;
    std::uint16_t windowCredit;
    std::uint16_t drainQueuedFrames;
    std::uint16_t drainFreeFrames;
    std::uint16_t duplicateFrames;
    std::uint16_t stagingStreamId;
    std::uint32_t acceptedBytes;
    std::uint32_t flushedBytes;
    std::uint32_t droppedCommittedBytes;
};

struct DownlinkDataV3Frame {
    std::uint8_t version;
    std::uint8_t kind;
    std::uint16_t streamId;
    std::uint16_t frameIndex;
    std::uint16_t frameCount;
    std::uint32_t byteOffset;
    std::uint16_t byteCount;
    std::uint16_t reserved;
    std::uint8_t data[DOWNLINK_V3_MAX_DATA_BYTES];
};

enum DownlinkDataV3FrameKind : std::uint8_t {
    DOWNLINK_V3_DATA = 1U,
};

static constexpr std::size_t DOWNLINK_V3_FRAME_METADATA_BYTES =
    sizeof(DownlinkDataV3Frame) - DOWNLINK_V3_MAX_DATA_BYTES;

inline std::size_t downlinkDataV3SerializedSize(const std::uint16_t byteCount) {
    return DOWNLINK_V3_FRAME_METADATA_BYTES + static_cast<std::size_t>(byteCount);
}

inline bool serializeDownlinkDataV3Frame(const DownlinkDataV3Frame& frame, std::string& payloadOut) {
    if (frame.byteCount > DOWNLINK_V3_MAX_DATA_BYTES) {
        return false;
    }
    payloadOut.assign(reinterpret_cast<const char*>(&frame), downlinkDataV3SerializedSize(frame.byteCount));
    return true;
}

inline bool deserializeDownlinkDataV3Frame(const void* data, const std::size_t size, DownlinkDataV3Frame& frameOut) {
    frameOut = {};
    if (data == nullptr || size < DOWNLINK_V3_FRAME_METADATA_BYTES) {
        return false;
    }

    std::memcpy(&frameOut, data, DOWNLINK_V3_FRAME_METADATA_BYTES);
    const std::size_t expectedSize = downlinkDataV3SerializedSize(frameOut.byteCount);
    if (frameOut.byteCount > DOWNLINK_V3_MAX_DATA_BYTES || size < expectedSize) {
        return false;
    }

    if (frameOut.byteCount > 0U) {
        const auto* const payloadBytes =
            static_cast<const std::uint8_t*>(data) + DOWNLINK_V3_FRAME_METADATA_BYTES;
        std::memcpy(frameOut.data, payloadBytes, frameOut.byteCount);
    }
    return true;
}

struct ChunkReply {
    ReplyHeader header;
    std::uint8_t data[MAX_CHUNK_BYTES];
};

struct LinkStatusReply {
    ReplyHeader header;
    std::uint32_t rxChunks;
    std::uint32_t txChunks;
    std::uint32_t rxErrors;
    std::uint32_t txErrors;
};

enum class ReliableTransferOp : std::uint8_t {
    BEGIN = 1U,
    ACK_POLL = 2U,
    COMPLETE = 3U,
    CANCEL = 4U,
    ABORT = 5U,
};

enum class ReliableTransferResult : std::uint8_t {
    OK = 0U,
    BUSY = 1U,
    NO_PROGRESS = 2U,
    INVALID = 3U,
    IO_ERROR = 4U,
    HASH_MISMATCH = 5U,
    ABORTED = 6U,
    CANCELLED = 7U,
};

struct ReliableTransferControlRequest {
    RequestHeader header;
    std::uint8_t op;
    std::uint8_t reserved;
    std::uint16_t transferId;
    std::uint16_t packetSize;
    std::uint16_t reserved2;
    std::uint32_t segmentCount;
    std::uint8_t sha256[32];
    std::uint8_t packetBytes[MAX_RELIABLE_PACKET_BYTES];
};

struct ReliableTransferControlReply {
    ReplyHeader header;
    std::uint8_t op;
    std::uint8_t transferResult;
    std::uint16_t transferId;
    std::uint32_t contiguousSegments;
    std::uint32_t committedBytes;
    std::uint32_t duplicateSegments;
};

struct ReliableTransferDataFrame {
    std::uint8_t version;
    std::uint8_t kind;
    std::uint16_t transferId;
    std::uint16_t packetSize;
    std::uint16_t reserved;
    std::uint8_t packetBytes[MAX_RELIABLE_PACKET_BYTES];
};

enum ReliableTransferDataFrameKind : std::uint8_t {
    RELIABLE_DATA_PACKET = 1U,
};

inline RequestHeader makeRequestHeader(ServicePort service, std::uint16_t seq) {
    RequestHeader header = {};
    header.version = VERSION;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    return header;
}

inline RequestHeader makeRequestHeaderWithVersion(std::uint8_t version, ServicePort service, std::uint16_t seq) {
    RequestHeader header = {};
    header.version = version;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    return header;
}

inline ReplyHeader makeReplyHeader(ServicePort service, std::uint16_t seq, ResultCode result) {
    ReplyHeader header = {};
    header.version = VERSION;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    header.result = static_cast<std::uint8_t>(result);
    return header;
}

inline ReplyHeader makeReplyHeaderWithVersion(std::uint8_t version,
                                              ServicePort service,
                                              std::uint16_t seq,
                                              ResultCode result) {
    ReplyHeader header = {};
    header.version = version;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    header.result = static_cast<std::uint8_t>(result);
    return header;
}

inline UplinkPollRequest makeUplinkPollRequest(std::uint16_t seq, std::uint16_t maxBytes = MAX_CHUNK_BYTES) {
    UplinkPollRequest request = {};
    request.header = makeRequestHeader(ServicePort::UPLINK_POLL, seq);
    request.maxBytes = maxBytes > MAX_CHUNK_BYTES ? MAX_CHUNK_BYTES : maxBytes;
    return request;
}

inline DownlinkWriteRequest makeDownlinkWriteRequest(std::uint16_t seq) {
    DownlinkWriteRequest request = {};
    request.header = makeRequestHeader(ServicePort::DOWNLINK_WRITE, seq);
    return request;
}

inline LinkStatusRequest makeLinkStatusRequest(std::uint16_t seq) {
    LinkStatusRequest request = {};
    request.header = makeRequestHeader(ServicePort::LINK_STATUS, seq);
    return request;
}

inline DownlinkStageV2Request makeDownlinkStageV2Request(std::uint16_t seq, std::uint16_t streamId) {
    DownlinkStageV2Request request = {};
    request.header = makeRequestHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_STAGE_V2, seq);
    request.streamId = streamId;
    return request;
}

inline DownlinkStatusV2Request makeDownlinkStatusV2Request(std::uint16_t seq) {
    DownlinkStatusV2Request request = {};
    request.header = makeRequestHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_STATUS_V2, seq);
    return request;
}

inline DownlinkAbortV2Request makeDownlinkAbortV2Request(std::uint16_t seq, std::uint16_t streamId) {
    DownlinkAbortV2Request request = {};
    request.header = makeRequestHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_ABORT_V2, seq);
    request.streamId = streamId;
    return request;
}

inline ChunkReply makeChunkReply(ServicePort service, std::uint16_t seq, ResultCode result) {
    ChunkReply reply = {};
    reply.header = makeReplyHeader(service, seq, result);
    return reply;
}

inline LinkStatusReply makeLinkStatusReply(std::uint16_t seq, ResultCode result) {
    LinkStatusReply reply = {};
    reply.header = makeReplyHeader(ServicePort::LINK_STATUS, seq, result);
    return reply;
}

inline DownlinkStageV2Reply makeDownlinkStageV2Reply(std::uint16_t seq, ResultCode result) {
    DownlinkStageV2Reply reply = {};
    reply.header = makeReplyHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_STAGE_V2, seq, result);
    return reply;
}

inline DownlinkStatusV2Reply makeDownlinkStatusV2Reply(std::uint16_t seq, ResultCode result) {
    DownlinkStatusV2Reply reply = {};
    reply.header = makeReplyHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_STATUS_V2, seq, result);
    return reply;
}

inline DownlinkAbortV2Reply makeDownlinkAbortV2Reply(std::uint16_t seq, ResultCode result) {
    DownlinkAbortV2Reply reply = {};
    reply.header = makeReplyHeaderWithVersion(VERSION_V2, ServicePort::DOWNLINK_ABORT_V2, seq, result);
    return reply;
}

inline DownlinkControlV3Request makeDownlinkControlV3Request(DownlinkControlV3Op op,
                                                             std::uint16_t seq,
                                                             std::uint16_t streamId) {
    DownlinkControlV3Request request = {};
    request.header = makeRequestHeaderWithVersion(VERSION_V3, ServicePort::DOWNLINK_CONTROL_V3, seq);
    request.op = static_cast<std::uint8_t>(op);
    request.streamId = streamId;
    return request;
}

inline DownlinkControlV3Reply makeDownlinkControlV3Reply(DownlinkControlV3Op op,
                                                         std::uint16_t seq,
                                                         ResultCode result) {
    DownlinkControlV3Reply reply = {};
    reply.header = makeReplyHeaderWithVersion(VERSION_V3, ServicePort::DOWNLINK_CONTROL_V3, seq, result);
    reply.op = static_cast<std::uint8_t>(op);
    return reply;
}

inline DownlinkDataV3Frame makeDownlinkDataV3Frame(std::uint16_t streamId) {
    DownlinkDataV3Frame frame = {};
    frame.version = VERSION_V3;
    frame.kind = DOWNLINK_V3_DATA;
    frame.streamId = streamId;
    return frame;
}

inline ReliableTransferControlRequest makeReliableTransferControlRequest(ReliableTransferOp op, std::uint16_t seq) {
    ReliableTransferControlRequest request = {};
    request.header = makeRequestHeader(ServicePort::RELIABLE_TRANSFER_CONTROL, seq);
    request.op = static_cast<std::uint8_t>(op);
    return request;
}

inline ReliableTransferControlReply makeReliableTransferControlReply(std::uint16_t seq,
                                                                     ResultCode result,
                                                                     ReliableTransferOp op) {
    ReliableTransferControlReply reply = {};
    reply.header = makeReplyHeader(ServicePort::RELIABLE_TRANSFER_CONTROL, seq, result);
    reply.op = static_cast<std::uint8_t>(op);
    reply.transferResult = static_cast<std::uint8_t>(ReliableTransferResult::OK);
    return reply;
}

inline ReliableTransferDataFrame makeReliableTransferDataFrame(std::uint16_t transferId) {
    ReliableTransferDataFrame frame = {};
    frame.version = VERSION;
    frame.kind = RELIABLE_DATA_PACKET;
    frame.transferId = transferId;
    return frame;
}

static_assert(sizeof(DownlinkStageV2Request) <= 256U, "DOWNLINK_STAGE_V2 request must fit one CSP buffer");
static_assert(sizeof(DownlinkStageV2Reply) <= 256U, "DOWNLINK_STAGE_V2 reply must fit one CSP buffer");
static_assert(sizeof(DownlinkStatusV2Reply) <= 256U, "DOWNLINK_STATUS_V2 reply must fit one CSP buffer");
static_assert(sizeof(DownlinkAbortV2Reply) <= 256U, "DOWNLINK_ABORT_V2 reply must fit one CSP buffer");
static_assert(sizeof(DownlinkControlV3Request) <= 2048U, "DOWNLINK_CONTROL_V3 request must fit one CSP buffer");
static_assert(sizeof(DownlinkControlV3Reply) <= 2048U, "DOWNLINK_CONTROL_V3 reply must fit one CSP buffer");
static_assert(sizeof(DownlinkDataV3Frame) <= 2048U, "DOWNLINK_DATA_V3 frame must fit one CSP buffer");

}  // namespace CSP
}  // namespace COMM
}  // namespace OBC

#endif
