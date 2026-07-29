#ifndef OBC_SIMULATORS_COMM_COMMNODESERVER_HPP
#define OBC_SIMULATORS_COMM_COMMNODESERVER_HPP

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/comm/CommSimModel.hpp"
#include "simulators/comm/SerialIngressAcquisitionFilter.hpp"
#include "simulators/csp/CspRuntime.hpp"

struct csp_conn_s;
typedef struct csp_conn_s csp_conn_t;
struct csp_packet_s;
typedef struct csp_packet_s csp_packet_t;

namespace OBC {
namespace COMM {

enum class CommExternalLinkMode {
    SERIAL,
    TCP_SERVER,
};

struct CommNodeConfig {
    std::uint16_t nodeId = CSP::DEFAULT_COMM_NODE_ID;
    CommExternalLinkMode linkMode = CommExternalLinkMode::SERIAL;
    std::string serialDevice;
    std::string tcpListenHost = "127.0.0.1";
    std::uint16_t tcpListenPort = 0U;
    std::uint32_t baudrate = 115200U;
    std::string beaconSerialDevice;
    std::uint32_t beaconBaudrate = 115200U;
    std::string interfaceName = "COMMCSP";
    CommSimConfig simConfig = {};
};

class CommNodeDownlinkV2State {
  public:
    static constexpr std::size_t CHUNK_BYTES = CSP::DOWNLINK_STAGE_V2_MAX_DATA_BYTES;
    static constexpr std::size_t STAGING_SLOT_LIMIT = CSP::DOWNLINK_STAGE_V2_STAGING_SLOTS;
    static constexpr std::size_t DRAIN_SLOT_LIMIT = CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS;
    static constexpr std::uint32_t STAGING_TIMEOUT_MS = 2000U;

    struct DrainChunk {
        std::uint16_t streamId = 0U;
        std::uint16_t acceptedSeq = 0U;
        std::uint16_t byteCount = 0U;
        std::array<std::uint8_t, CHUNK_BYTES> data = {};
    };

    CommNodeDownlinkV2State();

    void reset();

    void reapTimedOut(std::chrono::steady_clock::time_point now);

    CSP::DownlinkStageV2Reply handleStage(const CSP::DownlinkStageV2Request& request,
                                          std::uint8_t linkFlags,
                                          bool* acceptedNewChunk = nullptr);

    CSP::DownlinkStatusV2Reply handleStatus(const CSP::DownlinkStatusV2Request& request, std::uint8_t linkFlags) const;

    CSP::DownlinkAbortV2Reply handleAbort(const CSP::DownlinkAbortV2Request& request, std::uint8_t linkFlags);

    bool hasDrainWork() const;

    bool peekDrainChunk(DrainChunk& out) const;

    bool confirmDrainChunkFlushed(const DrainChunk& chunk);

    void handleDrainWriteFailure(std::size_t inFlightBytes);

  private:
    struct CachedStageReply {
        std::uint16_t streamId = 0U;
        std::uint16_t seq = 0U;
        DrainChunk chunk = {};
        std::uint8_t flags = 0U;
        CSP::DownlinkStageV2Reply reply = {};
    };

    struct ActiveStreamState {
        bool active = false;
        std::uint16_t streamId = 0U;
        std::uint16_t nextSeq = 1U;
        std::vector<DrainChunk> stagedChunks;
        std::vector<CachedStageReply> stagedReplies;
        bool pendingFinal = false;
        DrainChunk pendingFinalChunk = {};
        std::uint8_t pendingFinalFlags = 0U;
        std::chrono::steady_clock::time_point lastActivity = {};
    };

    static bool validV2Header_(const CSP::RequestHeader& header, CSP::ServicePort service);

    static bool drainChunkEqualsRequest_(const DrainChunk& chunk, const CSP::DownlinkStageV2Request& request);

    static std::uint32_t totalBytes_(const std::deque<DrainChunk>& chunks);

    std::size_t stagedSlotCount_() const;

    std::uint16_t drainFreeSlots_() const;

    void clearActiveStream_();

    void cacheCommittedReplies_(std::vector<CachedStageReply>& replies);

    const CachedStageReply* findActiveReply_(const CSP::DownlinkStageV2Request& request) const;

    const CachedStageReply* findCommittedReply_(const CSP::DownlinkStageV2Request& request) const;

    static bool cachedReplyMatchesRequest_(const CachedStageReply& entry,
                                           const CSP::DownlinkStageV2Request& request);

    void trimCommittedReplyCache_();

    bool tryCommitPendingFinal_(std::uint8_t linkFlags);

    ActiveStreamState m_activeStream;
    std::deque<DrainChunk> m_drainQueue;
    std::deque<CachedStageReply> m_committedReplyCache;
    std::uint32_t m_acceptedBytes;
    std::uint32_t m_flushedBytes;
    std::uint32_t m_droppedCommittedBytes;
};

class CommNodeDownlinkV3State {
  public:
    static constexpr std::size_t FRAME_BYTES = CSP::DOWNLINK_V3_MAX_DATA_BYTES;
    static constexpr std::size_t STAGING_FRAME_LIMIT = CSP::DOWNLINK_V3_STAGING_FRAME_LIMIT;
    static constexpr std::size_t DRAIN_FRAME_LIMIT = CSP::DOWNLINK_V3_DRAIN_FRAME_LIMIT;
    static constexpr std::uint32_t STAGING_TIMEOUT_MS = 2000U;

    struct DrainFrame {
        std::uint16_t streamId = 0U;
        std::uint16_t frameIndex = 0U;
        std::uint16_t frameCount = 0U;
        std::uint32_t byteOffset = 0U;
        std::uint16_t byteCount = 0U;
        std::array<std::uint8_t, FRAME_BYTES> data = {};
    };

    CommNodeDownlinkV3State();

    void reset();

    void reapTimedOut(std::chrono::steady_clock::time_point now);

    CSP::DownlinkControlV3Reply handleControl(const CSP::DownlinkControlV3Request& request, std::uint8_t linkFlags);

    bool handleData(const CSP::DownlinkDataV3Frame& frame);

    bool hasDrainWork() const;

    bool peekDrainFrame(DrainFrame& out) const;

    bool confirmDrainFrameFlushed(const DrainFrame& frame);

    void handleDrainWriteFailure(std::size_t inFlightBytes);

  private:
    struct StagedFrame {
        bool received = false;
        DrainFrame frame = {};
    };

    struct ActiveStreamState {
        bool active = false;
        std::uint16_t streamId = 0U;
        std::uint16_t totalFrames = 0U;
        std::uint32_t totalBytes = 0U;
        std::vector<StagedFrame> stagedFrames;
        std::uint16_t duplicateFrames = 0U;
        bool commitComplete = false;
        CSP::DownlinkControlV3Reply beginReply = {};
        CSP::DownlinkControlV3Reply commitReply = {};
        std::chrono::steady_clock::time_point lastActivity = {};
    };

    struct CommittedStreamCache {
        bool valid = false;
        std::uint16_t streamId = 0U;
        std::uint16_t totalFrames = 0U;
        std::uint32_t totalBytes = 0U;
        CSP::DownlinkControlV3Reply commitReply = {};
    };

    static bool validV3ControlHeader_(const CSP::RequestHeader& header);

    static bool validV3FrameShape_(const CSP::DownlinkDataV3Frame& frame);

    static std::uint16_t drainFreeFrames_(const std::deque<DrainFrame>& drainQueue);

    static std::uint32_t totalBytes_(const std::deque<DrainFrame>& drainQueue);

    std::uint16_t contiguousFrames_() const;

    std::uint32_t contiguousBytes_() const;

    std::uint16_t windowCredit_() const;

    void populateReplyStats_(CSP::DownlinkControlV3Reply& reply, std::uint8_t linkFlags) const;

    void clearActiveStream_();

    bool canAcceptBegin_(const CSP::DownlinkControlV3Request& request) const;

    bool matchesActiveBegin_(const CSP::DownlinkControlV3Request& request) const;

    bool matchesCommittedCommit_(const CSP::DownlinkControlV3Request& request) const;

    bool frameFitsActiveLayout_(const CSP::DownlinkDataV3Frame& frame) const;

    bool tryCommitActive_(std::uint8_t linkFlags, CSP::DownlinkControlV3Reply& replyOut);

    ActiveStreamState m_activeStream;
    CommittedStreamCache m_committedStream;
    std::deque<DrainFrame> m_drainQueue;
    std::uint32_t m_acceptedBytes;
    std::uint32_t m_flushedBytes;
    std::uint32_t m_droppedCommittedBytes;
};

class CommNodeServer {
  public:
    explicit CommNodeServer(const CommNodeConfig& config);

    ~CommNodeServer();

    bool start();

    void run();

    void stop();

  private:
    bool ensureExternalLinkOpen_(std::uint32_t timeoutMs);

    bool writeExternalLink_(const std::uint8_t* data, std::size_t size);

    bool ensureTcpListenOpen_();

    void closeExternalLink_();

    void closeTcpListen_();

    bool ensureBeaconLinkOpen_();

    void closeBeaconLink_();

    void stopBeaconWriter_();

    bool enqueueBeaconFrame_(const std::uint8_t* data, std::size_t size);

    void beaconWriterLoop_();

    void pollExternalIngress_(std::uint32_t timeoutMs);

    void handleConnection_(csp_conn_t* conn);

    void handleUplinkPoll_(csp_conn_t* conn, const CSP::UplinkPollRequest& request);

    void handleDownlinkWrite_(csp_conn_t* conn, const CSP::DownlinkWriteRequest& request);

    void handleLinkStatus_(csp_conn_t* conn, const CSP::LinkStatusRequest& request);

    void handleDownlinkStageV2_(csp_conn_t* conn, const CSP::DownlinkStageV2Request& request);

    void handleDownlinkStatusV2_(csp_conn_t* conn, const CSP::DownlinkStatusV2Request& request);

    void handleDownlinkAbortV2_(csp_conn_t* conn, const CSP::DownlinkAbortV2Request& request);

    void handleDownlinkControlV3_(csp_conn_t* conn, csp_packet_t* requestPacket, const CSP::DownlinkControlV3Request& request);

    void handleDownlinkDataV3_(csp_packet_t* packet);

    void handleReliableTransferData_(csp_packet_t* packet);

    void handleReliableTransferControl_(csp_conn_t* conn, const CSP::ReliableTransferControlRequest& request);

    void handleBeaconPush_(csp_packet_t* packet);

    void sendChunkReply_(csp_conn_t* conn, const CSP::ChunkReply& reply);

    void sendLinkStatusReply_(csp_conn_t* conn, const CSP::LinkStatusReply& reply);

    void sendDownlinkStageV2Reply_(csp_conn_t* conn, const CSP::DownlinkStageV2Reply& reply);

    void sendDownlinkStatusV2Reply_(csp_conn_t* conn, const CSP::DownlinkStatusV2Reply& reply);

    void sendDownlinkAbortV2Reply_(csp_conn_t* conn, const CSP::DownlinkAbortV2Reply& reply);

    void sendDownlinkControlV3Reply_(csp_conn_t* conn, csp_packet_t* requestPacket, const CSP::DownlinkControlV3Reply& reply);

    void sendReliableTransferControlReply_(csp_conn_t* conn, const CSP::ReliableTransferControlReply& reply);

    void resetReliableTransferReceiver_(bool removeTempFile);

    bool downlinkV2Enabled_() const;

    bool downlinkV3Enabled_() const;

    void startDownlinkDrainWorker_();

    void stopDownlinkDrainWorker_();

    void downlinkDrainWorkerLoop_();

    void notifyDownlinkDrainWorker_();

    void reapDownlinkV2Timeout_();

    void handleDownlinkV2Disconnect_();

    void reapDownlinkV3Timeout_();

    void handleDownlinkV3Disconnect_();

  private:
    struct ReliableTransferReceiverState {
        bool enabled = false;
        bool active = false;
        std::uint16_t transferId = 0U;
        std::uint32_t fileSize = 0U;
        std::uint32_t segmentCount = 0U;
        std::uint32_t duplicateSegments = 0U;
        std::uint32_t contiguousSegments = 0U;
        std::uint32_t committedBytes = 0U;
        std::uint32_t ackVisibleContiguousSegments = 0U;
        std::uint32_t ackVisibleCommittedBytes = 0U;
        std::uint32_t noProgressPollsRemaining = 0U;
        std::uint32_t staleTimeoutMs = 5000U;
        std::uint32_t checksum = 0U;
        std::array<std::uint8_t, 32> sha256 = {};
        std::string outputDir;
        std::string tempPath;
        std::string finalPath;
        std::vector<bool> receivedSegments;
        std::fstream file;
        std::chrono::steady_clock::time_point lastActivity = {};
    };

    CommNodeConfig m_config;
    std::atomic<bool> m_running;
    ::OBC::CSP::LibCspRuntime m_runtime;
    int m_linkFd;
    int m_tcpListenFd;
    int m_beaconFd;
    std::mutex m_linkMutex;
    std::mutex m_beaconLinkMutex;
    std::mutex m_beaconMutex;
    std::condition_variable m_beaconCv;
    std::vector<std::uint8_t> m_pendingBeaconFrame;
    std::thread m_beaconWriter;
    bool m_beaconWriterRunning;
    bool m_beaconFramePending;
    std::mutex m_downlinkV2Mutex;
    std::condition_variable m_downlinkV2Cv;
    std::thread m_downlinkV2DrainWorker;
    bool m_downlinkV2DrainWorkerRunning;
    CommNodeDownlinkV2State m_downlinkV2State;
    std::mutex m_downlinkV3Mutex;
    std::condition_variable m_downlinkV3Cv;
    std::thread m_downlinkV3DrainWorker;
    bool m_downlinkV3DrainWorkerRunning;
    CommNodeDownlinkV3State m_downlinkV3State;
    CommSimModel m_model;
    SerialIngressAcquisitionFilter m_serialIngressFilter;
    ReliableTransferReceiverState m_reliableTransfer;
};

}  // namespace COMM
}  // namespace OBC

#endif
