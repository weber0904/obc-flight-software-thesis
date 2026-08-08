#ifndef OBC_BEACONPUBLISHER_TESTER_HPP
#define OBC_BEACONPUBLISHER_TESTER_HPP

#include <vector>

#include "OBC/Components/BeaconPublisher/BeaconPublisher.hpp"
#include "OBC/Components/BeaconPublisher/BeaconPublisherGTestBase.hpp"

namespace OBC {

class BeaconPublisherTester final : public BeaconPublisherGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    BeaconPublisherTester();
    ~BeaconPublisherTester() override;

    void testPublishEmitsPacket();
    void testCadenceWaitsBetweenPackets();
    void testNotConfiguredReportsDistinctError();
    void testDisabledSchedulerIsSilent();
    void testUnavailableSourceReportsError();
    void testSinkFailureReportsError();
    void testRuntimeSuppressIsSilent();
    void testRuntimeSuppressClearResumesOnNextTick();

  private:
    class FakeReducedSource final : public OBC::StateData::IReducedStateSource {
      public:
        bool getReducedStateForRuntime(OBC::StateData::ReducedStateV1& state) const override;

        bool available = true;
        OBC::StateData::ReducedStateV1 state = {};
    };

    class FakeBeaconSink final : public OBC::StateData::IBeaconSink {
      public:
        bool sendBeacon(const U8* data, U32 size) override;

        bool allowSend = true;
        std::vector<std::vector<U8>> packets;
    };

  private:
    void connectPorts();
    void initComponents();
    void configureNominal_();

  private:
    FakeReducedSource m_source;
    FakeBeaconSink m_sink;
    OBC::BeaconPublisher component;
};

}  // namespace OBC

#endif
