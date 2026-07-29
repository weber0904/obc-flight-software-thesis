#ifndef OBC_HKTRENDPRODUCTPRODUCER_TESTER_HPP
#define OBC_HKTRENDPRODUCTPRODUCER_TESTER_HPP

#include <vector>

#include "OBC/Components/HkTrendProductProducer/HkTrendProductProducer.hpp"
#include "OBC/Components/HkTrendProductProducer/HkTrendProductProducerGTestBase.hpp"

namespace OBC {

class HkTrendProductProducerTester final : public HkTrendProductProducerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 256;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    HkTrendProductProducerTester();
    ~HkTrendProductProducerTester() override;

    void testManualFlushEmitsChunkWithRawValues();
    void testCadenceWaitsBetweenSamples();
    void testSizeThresholdChunkingKeepsContiguousSequences();
    void testMissingStateUsesValidityFlags();
    void testFlushCommandOnEmptyChunkIsNoOp();
    void testGetStatusCommandReportsPendingState();
    void testThresholdChangeFlushesImmediately();
    void testParameterLoadDefaultPathUses8192();
    void testOutOfRangeParameterIsClampedAndPersisted();
    void testUnavailableSourceReportsError();
    void testSerializeFailureReturnsDpBuffer();
    void testThresholdFlushFailureRetainsCurrentSample();
    void testRetainedBacklogIsBoundedWhenFlushKeepsFailing();

  private:
    class FakeSnapshotSource final : public OBC::StateData::IStateSnapshotSource {
      public:
        bool readStateSnapshot(OBC::StateData::StateSnapshot& output) const override;

        bool available = true;
        OBC::StateData::StateSnapshot snapshot = {};
    };

    struct DecodedSentProduct {
        FwDpIdType metaRecordId = 0U;
        OBC::HkTrendChunkMetaV1 meta = {};
        FwDpIdType sampleRecordId = 0U;
        std::vector<OBC::HkTrendRecordV6> samples;
    };

  private:
    Fw::Success::T productGet_handler(FwDpIdType id, FwSizeType dataSize, Fw::Buffer& buffer) override;
    bool decodeLastSentProduct_(DecodedSentProduct& decoded) const;
    static U32 computePacketSizeForSamples_(FwSizeType sampleCount);
    static U32 computeMaxPendingSamples_();
    void connectPorts();
    void initComponents();
    void configureNominal_();

  private:
    FakeSnapshotSource m_source;
    U8 m_dpBuffer[16384];
    bool m_returnShortDpBuffer = false;
    OBC::HkTrendProductProducer component;
};

}  // namespace OBC

#endif
