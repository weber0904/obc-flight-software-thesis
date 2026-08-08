#ifndef OBC_GroundLinkHealthProviderTester_HPP
#define OBC_GroundLinkHealthProviderTester_HPP

#include <deque>
#include <memory>

#include "OBC/Components/GroundLinkDriver/GroundLinkDriver.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProvider.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProviderGTestBase.hpp"

namespace OBC {

class FakeGroundLinkHealthBackend final : public OBC::COMM::IGroundLinkBackend {
  public:
    struct HealthReply {
        bool success;
        bool connected;
    };

    explicit FakeGroundLinkHealthBackend(
        OBC::COMM::GroundLinkBackendMode mode = OBC::COMM::GroundLinkBackendMode::COMM_CSP);

    bool start() override;

    void stop() override;

    OBC::COMM::GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) override;

    OBC::COMM::GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) override;

    OBC::COMM::GroundLinkStats getStats() const override;

    OBC::COMM::GroundLinkObservationState getObservationState() const override;

    bool observeHealth() override;

    void setConnected(bool connected);

    void setMode(OBC::COMM::GroundLinkBackendMode mode);

    void setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics);

    void addRxChunk();

    void addTxChunk();

    void addErrors(U32 txErrors, U32 rxErrors);

    void queueHealthObservation(bool success, bool connected);

  private:
    OBC::COMM::GroundLinkStats m_stats;
    OBC::COMM::GroundLinkHealthSemantics m_healthSemantics;
    U32 m_successfulStatusObservations;
    std::deque<HealthReply> m_healthReplies;
};

class GroundLinkHealthProviderTester final : public GroundLinkHealthProviderGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 50;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    GroundLinkHealthProviderTester();

    ~GroundLinkHealthProviderTester() override;

    void testCommCspStatusObservationKeepsAvailabilityHealthy();

    void testCommCspStaleTransitionAndRecovery();

    void testCommCspStaleAgeTelemetryKeepsAdvancing();

    void testMarkTelemetryDirtyRepublishesActivityAgeTelemetry();

    void testErrorGrowthTracksByCycle();

    void testConnectedOnlyCompatibilitySuppressesTransportGrowthAndStale();

    void testDirectTcpConnectedFallbackDoesNotGoStale();

  private:
    void connectPorts();

    void initComponents();

  private:
    OBC::GroundLinkHealthProvider component;
    OBC::GroundLinkDriver m_sbandGroundLinkDriver;
    OBC::GroundLinkDriver m_uhfGroundLinkDriver;
    std::unique_ptr<FakeGroundLinkHealthBackend> m_sbandBackend;
    std::unique_ptr<FakeGroundLinkHealthBackend> m_uhfBackend;
};

}  // namespace OBC

#endif
