#ifndef OBC_PAYLOADOPSCONTROLLER_TESTER_HPP
#define OBC_PAYLOADOPSCONTROLLER_TESTER_HPP

#include <array>
#include <string>

#include "OBC/Components/PayloadOpsController/PayloadOpsController.hpp"
#include "OBC/Components/PayloadOpsController/PayloadOpsControllerGTestBase.hpp"
#include "OBC/Components/test/TestSupport.hpp"

namespace OBC {

class PayloadOpsControllerTester final : public PayloadOpsControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    PayloadOpsControllerTester();
    ~PayloadOpsControllerTester() override;

    void testCaptureDeterministicWritesDualArtifactsAndAutoPublishesPreview();
    void testCaptureDeterministicAllowsZeroCaptureIndex();
    void testPublishRawPromotesStoredCapture();
    void testPublishRawRejectsFullResolution();
    void testReusingCaptureIndexOverwritesLocalArtifacts();
    void testCaptureDeterministicRequiresPrepare();
    void testSingleReadyAllowsAutoThenDeterministicWithoutReprepare();
    void testRetiredImageTuningMaskIsRejected();
    void testSharedReadyPrepareUsesAutoWarmupDefaults();
    void testSetCameraDefaultsRejectsWhilePrepared();
    void testRawSensorSessionStillRequiresSpecialPrepare();
    void testPreviewPublishIgnoresUnrelatedDpWriterNotifications();
    void testPublishCaptureRejectsSynchronousFirstSliceFailure();
    void testPublishCaptureReadsLegacyManifestCompatibility();

  private:
    class DeterministicDualArtifactDriver;

    class FakeModeProvider final : public OBC::IModeSafetyModeControl {
      public:
        OBC::SatMode getModeForRuntime() const override { return this->mode; }
        void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override {
            static_cast<void>(source);
            this->mode = mode;
        }

        OBC::SatMode mode = OBC::SatMode::PAYLOAD;
    };

    class FakeEpsControl final : public OBC::IPayloadEpsControl {
      public:
        bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override;
        Fw::CmdResponse setPayloadProxyPower(bool enabled, OBC::EPS::StatusData& status) override;

        U32 callCount = 0U;
        bool lastEnabled = false;
    };

    class FakeBootControl final : public OBC::IRecoveryBootControl {
      public:
        bool isBootSafeFallbackRequiredForRuntime() const override { return false; }
        U32 getBootCountForRuntime() const override { return this->bootCount; }
        U32 getConsecutiveResetCountForRuntime() const override { return 0U; }
        U32 getUptimeForRuntime() const override { return 0U; }
        bool recordRecoveryRestartIntentForRuntime(OBC::ResetCause cause,
                                                   OBC::RecoveryIncidentSource source,
                                                   OBC::RecoveryLevel level) override {
            static_cast<void>(cause);
            static_cast<void>(source);
            static_cast<void>(level);
            return true;
        }
        bool acknowledgeRuntimeStableForRuntime() override { return true; }

        U32 bootCount = 7U;
    };

    Fw::Success::T productGet_handler(FwDpIdType id, FwSizeType dataSize, Fw::Buffer& buffer) override;
    void initComponents();
    void connectPorts();
    void driveTicks(U32 count, U32 sleepMs = 10U);
    void waitForProductSendCount_(U32 expectedCount, U32 maxTicks = 45U, U32 sleepMs = 10U);
    void prepareGeneric_();
    void prepareNonRaw_();
    void captureDeterministic_(U32 cmdSeq,
                               U8 captureIndex,
                               const char* tag,
                               U32 applyMask = 0U,
                               U32 exposureUsec = 10000U,
                               U32 gainX100 = 100U);
    void completePendingPublishes_();
    std::string dataProductPathFromSendHistory_(U32 historyIndex) const;
    std::string rawPathForIndex_(U8 captureIndex) const;
    std::string previewPathForIndex_(U8 captureIndex) const;
    std::string manifestPathForIndex_(U8 captureIndex) const;
    OBC::PayloadCaptureMetadata getLastMetadata_() const;
    OBC::PayloadStatusSnapshot getStatus_() const;

  private:
    TestSupport::TempDirectory m_runtimeRoot;
    FakeModeProvider m_mode;
    FakeEpsControl m_eps;
    FakeBootControl m_boot;
    DeterministicDualArtifactDriver* m_driver = nullptr;
    std::array<U8, 262144> m_dpBuffer;
    bool m_failNextProductGet = false;
    PayloadOpsController component;
};

}  // namespace OBC

#endif
