#ifndef OBC_COMPONENTS_HKTRENDPRODUCTPRODUCER_HKTRENDPRODUCTPRODUCER_HPP
#define OBC_COMPONENTS_HKTRENDPRODUCTPRODUCER_HKTRENDPRODUCTPRODUCER_HPP

#include "OBC/Components/HkTrendProductProducer/HkTrendProductProducerComponentAc.hpp"
#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"
#include "Os/Mutex.hpp"

#include <vector>

namespace OBC {

enum class HkTrendProductStatus : U32 {
    OK = 0U,
    NOT_CONFIGURED = 1U,
    SOURCE_UNAVAILABLE = 2U,
    DP_PORTS_NOT_CONNECTED = 3U,
    DP_BUFFER_UNAVAILABLE = 4U,
    SERIALIZE_ERROR = 5U,
    BACKLOG_FULL = 6U,
};

class HkTrendProductProducer final : public HkTrendProductProducerComponentBase {
  public:
    static constexpr U32 DEFAULT_PERIOD_TICKS = 30U;
    static constexpr U32 DEFAULT_TARGET_FILE_BYTES = 8192U;
    static constexpr U32 MAX_TARGET_FILE_BYTES = 10240U;
    static constexpr U32 MAX_PENDING_CHUNKS = 2U;

    explicit HkTrendProductProducer(const char* const compName);

    ~HkTrendProductProducer() override;

    void configureRuntime(OBC::StateData::IStateSnapshotSource* source);

    void configurePeriodForTest(U32 periodTicks);

    void configureTargetFileBytesForTest(U32 targetFileBytes);

    HkTrendProductStatus captureNow();

  private:
    void HK_TREND_FLUSH_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void HK_TREND_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void parameterUpdated(FwPrmIdType id) override;

    void parametersLoaded() override;

    OBC::HkTrendRecordV6 buildRecord_(const OBC::StateData::StateSnapshot& snapshot, U32 sequence) const;

    HkTrendProductStatus flushPending_(OBC::HkTrendFlushReason reason);

    bool haveProductPorts_();

    U32 normalizeTargetFileBytes_(U32 requested) const;

    FwSizeType computePendingDataSize_(FwSizeType sampleCount) const;

    U32 computePendingPacketSize_(FwSizeType sampleCount) const;

    FwSizeType computeMaxPendingSampleCount_() const;

    U32 getPendingEstimatedBytes_() const;

    void recomputePendingChunkSampleCount_();

    HkTrendProductStatus retainPendingSample_(const OBC::HkTrendRecordV6& record, U32 sequence);

    void publishState_(HkTrendProductStatus status);

    void publishTelemetry_();

    void resetTicks_();

  private:
    OBC::StateData::IStateSnapshotSource* m_source;
    U32 m_periodTicks;
    U32 m_ticksUntilCapture;
    U32 m_targetFileBytes;
    U32 m_nextSampleSequence;
    U32 m_nextChunkSequence;
    U32 m_lastSequence;
    U32 m_lastChunkSequence;
    U32 m_productCount;
    U32 m_lastSizeBytes;
    HkTrendProductStatus m_lastError;
    std::vector<OBC::HkTrendRecordV6> m_pendingSamples;
    FwSizeType m_pendingChunkSampleCount;
    Os::Mutex m_lock;
};

}  // namespace OBC

#endif
