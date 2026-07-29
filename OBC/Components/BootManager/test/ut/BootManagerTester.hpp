#ifndef OBC_BootManagerTester_HPP
#define OBC_BootManagerTester_HPP

#include <string>
#include <vector>

#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/BootManager/BootManagerGTestBase.hpp"

namespace OBC {

class BootManagerTester final : public BootManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 100;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    BootManagerTester();

    ~BootManagerTester() override;

    void testPrepareVerifyActivateConfirmFlow();

    void testConfirmTimeoutRollsBack();

    void testInvalidMetadataRollsBackToSafeSlot();

    void testLegacyPendingMetadataRollsBackToLastKnownGood();

    void testRollbackPersistsCleanMetadataFile();

    void testInvalidSignatureRejected();

    void testUnknownSignerRejected();

    void testMalformedManifestRejected();

    void testManifestDigestMismatchRejected();

    void testDowngradeRejected();

    void testPostVerifyTamperDoesNotActivate();

    void testInvalidTrustStateDoesNotActivate();

    void testMetadataReloadPreservesPendingTrustState();

    void testNestedRelativeManifestPathActivates();

    void testInvalidPendingMetadataRollsBack();

    void testPrepareRejectedDuringPendingConfirm();

    void testRecoveryIntentPersistsMetadata();

    void testRecoveryProcessRestartPersistsR2Metadata();

    void testConsecutiveRecoveryBootsRequireSafeFallback();

    void testStableAckClearsRecoveryFallback();

    void testBootObservedBreadcrumbWritten();

    void testStableAckBreadcrumbWritten();

    void testBootStatusCommandPublishesForcedRefreshTelemetry();

    void testResetCauseAndBootCountCommands();

    void testRecoveryCauseConsumedByNextNormalBoot();

    void testRuntimeBootInitializationRetriesAfterPersistFailure();

  private:
    class FakePersistentFaultRecorder final : public OBC::IPersistentFaultRecorder {
      public:
        bool appendPersistentFaultRecordForRuntime(const OBC::PersistentFaultRecord& record) override {
            this->records.push_back(record);
            return true;
        }

        std::vector<OBC::PersistentFaultRecord> records = {};
    };

    void connectPorts();

    void initComponents();

    std::string makeTempRoot_() const;

    std::string makeStagingFile_(const std::string& fileName, const std::string& contents) const;

    void writeSignedManifest_(const std::string& fileName,
                              const std::string& digestHex,
                              U32 imageSize,
                              U32 softwareVersion,
                              OBC::BootSlot targetSlot = OBC::BootSlot::SLOT_B,
                              const std::string& signerId = "repo-dev-boot-signer",
                              U32 keySlot = 1U,
                              bool validSignature = true) const;

    void writeFile_(const std::string& path, const std::string& contents) const;

  protected:
    void from_bootStatusRefreshTlmOut_handler(FwIndexType portNum,
                                              FwChanIdType id,
                                              Fw::Time& timeTag,
                                              Fw::TlmBuffer& val) override;

  private:
    std::string m_tempRoot;
    FakePersistentFaultRecorder m_faultRecorder;
    OBC::BootManager component;
};

}  // namespace OBC

#endif
