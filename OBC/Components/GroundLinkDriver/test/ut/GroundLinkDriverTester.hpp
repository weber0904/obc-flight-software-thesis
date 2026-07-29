#ifndef OBC_GroundLinkDriverTester_HPP
#define OBC_GroundLinkDriverTester_HPP

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "OBC/Components/GroundLinkDriver/GroundLinkDriver.hpp"
#include "OBC/Components/GroundLinkDriver/GroundLinkDriverGTestBase.hpp"

namespace OBC {

class FakeGroundLinkBackend final : public OBC::COMM::IGroundLinkBackend {
  public:
    struct ReceiveReply {
        OBC::COMM::GroundLinkReceiveStatus status;
        std::string chunk;
        bool connected;
    };

    struct HealthReply {
        bool success;
        bool connected;
    };

    explicit FakeGroundLinkBackend(OBC::COMM::GroundLinkBackendMode mode = OBC::COMM::GroundLinkBackendMode::DIRECT_TCP);

    bool start() override;

    void stop() override;

    OBC::COMM::GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) override;

    OBC::COMM::GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) override;

    OBC::COMM::GroundLinkStats getStats() const override;

    OBC::COMM::GroundLinkObservationState getObservationState() const override;

    bool observeHealth() override;

    void queueReceive(const ReceiveReply& reply);

    void queueSend(OBC::COMM::GroundLinkSendStatus status, bool connected = true);

    void queueHealthObservation(bool success, bool connected = true);

    void setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics);

    U32 getObservationStateCallCount() const;

  private:
    OBC::COMM::GroundLinkStats m_stats;
    OBC::COMM::GroundLinkHealthSemantics m_healthSemantics;
    U32 m_successfulStatusObservations;
    U32 m_observationStateCallCount;
    std::deque<ReceiveReply> m_receiveReplies;
    std::deque<std::pair<OBC::COMM::GroundLinkSendStatus, bool>> m_sendReplies;
    std::deque<HealthReply> m_healthReplies;
};

class GroundLinkDriverTester final : public GroundLinkDriverGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 50;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    GroundLinkDriverTester();

    ~GroundLinkDriverTester() override;

    void testPumpPublishesReadyAndReceive();

    void testSendSuccessPublishesCounters();

    void testSendRetryReturnsRetry();

    void testReceiveErrorPublishesWarning();

    void testRecvReturnDeallocates();

    void testObserveHealthPublishesObservationState();

    void testRuntimeObservationReadsCachedSnapshot();

  private:
    void installBackend(std::unique_ptr<OBC::FakeGroundLinkBackend> backend);

    void connectPorts();

    void initComponents();

    void from_recv_handler(FwIndexType portNum, Fw::Buffer& recvBuffer, const Drv::ByteStreamStatus& recvStatus) override;

    Fw::Buffer from_allocate_handler(FwIndexType portNum, FwSizeType size) override;

    void from_deallocate_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

  private:
    std::unique_ptr<OBC::FakeGroundLinkBackend> m_backend;
    OBC::GroundLinkDriver component;
    std::vector<std::string> m_receivedPayloads;
    std::vector<U8*> m_ownedReceiveBuffers;
};

}  // namespace OBC

#endif
