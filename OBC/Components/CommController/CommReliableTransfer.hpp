#ifndef OBC_COMPONENTS_COMMCONTROLLER_COMMRELIABLETRANSFER_HPP
#define OBC_COMPONENTS_COMMCONTROLLER_COMMRELIABLETRANSFER_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Fw/Types/FileNameString.hpp"
#include "Svc/FileDownlinkPorts/SendFileResponseSerializableAc.hpp"
#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace OBC {

enum class CommReliableTransferResult : U32 {
    NONE = 0U,
    SUCCESS = 1U,
    TIMEOUT = 2U,
    RETRY_EXHAUSTED = 3U,
    CANCELLED = 4U,
    ABORTED = 5U,
    IO_ERROR = 6U,
    INVALID = 7U,
    BUSY = 8U,
    HASH_MISMATCH = 9U,
};

enum class CommReliableTransferStartFailureStage : U32 {
    NONE = 0U,
    ALREADY_ACTIVE = 1U,
    UNSUPPORTED_RANGE = 2U,
    RUNTIME_UNAVAILABLE = 3U,
    SOURCE_LOAD_FAILED = 4U,
    FILE_TOO_LARGE = 5U,
    EMPTY_BASENAME = 6U,
    SHA256_FAILED = 7U,
    START_PACKET_BUILD_FAILED = 8U,
    BEGIN_REQUEST_FAILED = 9U,
    BEGIN_REPLY_INVALID = 10U,
    BEGIN_REJECTED = 11U,
};

struct CommReliableTransferConfig {
    std::uint16_t targetNode = OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID;
    std::uint32_t timeoutMs = 1000U;
    std::uint32_t ackTimeoutTicks = 1U;
    std::uint32_t resendBudget = 3U;
    std::uint16_t segmentPayloadBytes = static_cast<std::uint16_t>(OBC::COMM::CSP::RELIABLE_SEGMENT_DATA_BYTES);
    std::uint16_t windowSize = 2U;
    std::uint32_t maxFileBytes = OBC::COMM::CSP::MAX_RELIABLE_TRANSFER_FILE_BYTES;
};

struct CommReliableTransferState {
    bool active = false;
    U32 requestContext = 0U;
    std::uint16_t transferId = 0U;
    U32 fileSize = 0U;
    U32 totalSegments = 0U;
    U32 contiguousSegments = 0U;
    U32 committedBytes = 0U;
    U32 resendCount = 0U;
    U32 duplicateSegments = 0U;
    CommReliableTransferResult lastResult = CommReliableTransferResult::NONE;
    CommReliableTransferStartFailureStage lastStartFailureStage = CommReliableTransferStartFailureStage::NONE;
    U32 lastStartFailureDetail = 0U;
};

struct CommReliableTransferStepResult {
    bool finished = false;
    Svc::SendFileResponse response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 0U);
    CommReliableTransferResult finalResult = CommReliableTransferResult::NONE;
    bool progressAdvanced = false;
    bool resendAttempted = false;
    bool duplicateObserved = false;
};

class CommReliableTransfer final {
  public:
    explicit CommReliableTransfer(OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime());

    void configure(const CommReliableTransferConfig& config);

    void setRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime);

    Svc::SendFileResponse start(U32 requestContext,
                                const Fw::StringBase& sourceFileName,
                                const Fw::StringBase& destFileName,
                                U32 offset,
                                U32 length);

    CommReliableTransferStepResult step();

    CommReliableTransferStepResult cancel(bool abortTransfer);

    const CommReliableTransferState& state() const;

    bool isActive() const;

  private:
    enum class Phase {
        IDLE,
        SENDING,
        COMPLETE_PENDING,
    };

    struct BuiltPacket {
        std::vector<U8> bytes;
    };

  private:
    bool ensureRuntime_();
    std::uint16_t nextSeq_();
    std::uint16_t allocateTransferId_();
    static std::string baseName_(const Fw::StringBase& path);
    static bool queryFileSize_(const std::string& path, std::size_t& size);
    static bool loadWholeFile_(const std::string& path, std::vector<U8>& bytes);
    static bool computeSha256_(const std::vector<U8>& bytes, std::array<U8, 32>& digest);
    static U32 computeChecksum_(const std::vector<U8>& bytes);
    static BuiltPacket makeStartPacket_(const std::string& sourceBase, const std::string& destBase, U32 fileSize);
    BuiltPacket makeDataPacket_(U32 segmentIndex) const;
    BuiltPacket makeEndPacket_() const;
    BuiltPacket makeCancelPacket_() const;
    static Svc::SendFileStatus mapReplyToSendFileStatus_(OBC::COMM::CSP::ReliableTransferResult result);
    static CommReliableTransferResult mapReplyToTransferResult_(OBC::COMM::CSP::ReliableTransferResult result);
    void recordStartFailure_(U32 requestContext, CommReliableTransferStartFailureStage stage, U32 detail);
    bool requestControl_(OBC::COMM::CSP::ReliableTransferOp op,
                         const BuiltPacket* packet,
                         OBC::COMM::CSP::ReliableTransferControlReply& reply);
    bool pollAck_(OBC::COMM::CSP::ReliableTransferControlReply& reply);
    bool sendDataFrame_(U32 segmentIndex);
    void reset_();

  private:
    CommReliableTransferConfig m_config;
    OBC::CSP::ICspRuntime* m_runtime;
    bool m_runtimeReady;
    std::uint16_t m_seq;
    std::uint16_t m_nextTransferId;
    Phase m_phase;
    CommReliableTransferState m_state;
    std::vector<U8> m_fileBytes;
    std::array<U8, 32> m_sha256;
    U32 m_checksum;
    U32 m_nextSegmentToSend;
    U32 m_ticksSinceProgress;
    U32 m_currentRetryCount;
    OBC::CSP::RuntimeStatus m_lastControlStatus;
    std::size_t m_lastControlReplySize;
};

}  // namespace OBC

#endif
