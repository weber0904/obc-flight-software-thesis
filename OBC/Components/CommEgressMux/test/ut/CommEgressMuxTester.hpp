#ifndef OBC_CommEgressMuxTester_HPP
#define OBC_CommEgressMuxTester_HPP

#include <memory>
#include <vector>

#include "OBC/Components/CommEgressMux/CommEgressMux.hpp"
#include "OBC/Components/CommEgressMux/CommEgressMuxGTestBase.hpp"

namespace OBC {

class CommEgressMuxTester final : public CommEgressMuxGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct ForwardedPacket {
        FwIndexType portNum;
        Fw::ComBuffer data;
        U32 context;
    };

    struct ForwardedBuffer {
        FwIndexType portNum;
        Fw::Buffer buffer;
    };

    CommEgressMuxTester();

    ~CommEgressMuxTester() override;

    void testDefaultsSuppressSbandUntilEnabled();

    void testSbandLiveObservabilityRoutesAfterEnable();

    void testSwitchRoutesTelemetryAndFileToUhf();

    void testUhfFileTransferSuspendsAllPacketEgress();

    void testQuietModeSuppressesPacketEgress();
    void testQuietModeSuppressesSbandPacketEgress();

    void testQuietModeReturnsFileBuffersLocally();

    void testUhfPrimaryPacketQuietSuppressesPacketEgressOnly();

    void testUhfRouteCountersTrackEventAndTelemetryPackets();

    void testUhfSuppressCountersTrackQuietPackets();

    void testSbandCountersTrackSuppressedAndRoutedPackets();

    void testTelemetryLinkSwitchRepublishesUhfCounters();

    void testReturnPathMergesBothBranches();

  private:
    void connectPorts();

    void initComponents();

    void from_sbandPacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void from_uhfPacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void from_sbandFileBufferOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void from_uhfFileBufferOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void from_fileBufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void clearObservations();

    Fw::ComBuffer makePacket_(U8 value) const;

    Fw::Buffer makeBuffer_(U8 value, U32 size = 4U);

  private:
    OBC::CommEgressMux component;
    std::vector<std::unique_ptr<U8[]>> m_ownedBuffers;
    std::vector<ForwardedPacket> m_sbandPackets;
    std::vector<ForwardedPacket> m_uhfPackets;
    std::vector<ForwardedBuffer> m_sbandBuffers;
    std::vector<ForwardedBuffer> m_uhfBuffers;
    std::vector<ForwardedBuffer> m_returnBuffers;
};

}  // namespace OBC

#endif
