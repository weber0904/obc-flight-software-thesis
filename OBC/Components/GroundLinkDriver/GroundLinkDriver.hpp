#ifndef OBC_Components_GroundLinkDriver_HPP
#define OBC_Components_GroundLinkDriver_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "OBC/Components/GroundLinkDriver/GroundLinkDriverComponentAc.hpp"
#include "simulators/comm/GroundLinkBackend.hpp"

namespace OBC {

class GroundLinkDriver final : public GroundLinkDriverComponentBase {
  public:
    explicit GroundLinkDriver(const char* const compName);

    ~GroundLinkDriver() override;

    void configureDirectTcp(const std::string& host, std::uint16_t port, std::size_t maxChunkBytes = 512U);

    void configureCommCsp(std::uint16_t targetNode = OBC::COMM::CSP::DEFAULT_COMM_NODE_ID,
                          OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime(),
                          OBC::COMM::GroundLinkHealthSemantics healthSemantics =
                              OBC::COMM::GroundLinkHealthSemantics::DISABLED);

    void clearConfiguration();

    void setBackendForTest(OBC::COMM::IGroundLinkBackend* backend);

    bool start();

    void stop();

    void join();

    bool pumpOnceForTest(std::uint32_t timeoutMs = 0U);

    OBC::COMM::GroundLinkStats getStatsForRuntime() const;

    OBC::COMM::GroundLinkObservationState getObservationForRuntime() const;

    bool isRunningForRuntime() const;

    bool observeHealthForRuntime();

  private:
    Drv::ByteStreamStatus send_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void workerLoop_();

    bool runReceiveOnce_(std::uint32_t timeoutMs);

    void installOwnedBackend_(std::unique_ptr<OBC::COMM::IGroundLinkBackend> backend);

    void publishStats_(const OBC::COMM::GroundLinkStats& stats);

    void stopWithObservability_(bool publishObservability);

    void updateConnectionState_(const OBC::COMM::GroundLinkStats& stats);

    void emitError_(U32 code);

    std::shared_ptr<OBC::COMM::IGroundLinkBackend> getBackend_() const;

    bool shouldPublishStats_(const OBC::COMM::GroundLinkStats& stats);

    void updateObservationCache_(const std::shared_ptr<OBC::COMM::IGroundLinkBackend>& backend);

  private:
    mutable std::mutex m_mutex;
    std::shared_ptr<OBC::COMM::IGroundLinkBackend> m_backend;
    std::thread m_workerThread;
    std::atomic<bool> m_running;
    bool m_connectedLatched;
    bool m_havePublishedStats;
    OBC::COMM::GroundLinkStats m_lastPublishedStats;
    OBC::COMM::GroundLinkObservationState m_lastObservation;
};

}  // namespace OBC

#endif
