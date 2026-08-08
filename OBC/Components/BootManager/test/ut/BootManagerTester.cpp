#include "BootManagerTester.hpp"

#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

#include "Fw/Types/StringTemplate.hpp"
#include "OBC/Components/BootManager/BootManifestVerifier.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace OBC {

BootManagerTester::BootManagerTester()
    : BootManagerGTestBase("BootManagerTester", MAX_HISTORY_SIZE),
      m_tempRoot(this->makeTempRoot_()),
      m_faultRecorder(),
      component("BootManager") {
    this->initComponents();
    this->connectPorts();
    this->component.configurePersistentFaultRecorderForRuntime(&this->m_faultRecorder);
    this->component.configureStorageRootForTest(this->m_tempRoot + "/persistent-data/boot");
    this->m_faultRecorder.records.clear();
    this->clearHistory();
}

BootManagerTester::~BootManagerTester() {
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/boot/subdir/nested-image.bin.manifest-v1").c_str());
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/boot/subdir/nested-image.bin").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data/boot/subdir").c_str());
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data/boot").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data").c_str());
    (void)Os::FileSystem::removeDirectory(this->m_tempRoot.c_str());
}

void BootManagerTester::from_bootStatusRefreshTlmOut_handler(FwIndexType portNum,
                                                             FwChanIdType id,
                                                             Fw::Time& timeTag,
                                                             Fw::TlmBuffer& val) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_bootStatusRefreshTlmOut(id, timeTag, val);
    this->dispatchTlm(id, timeTag, val);
}

void BootManagerTester::testPrepareVerifyActivateConfirmFlow() {
    const std::string stagingName = "staged-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v1");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    ASSERT_EQ(digestHex, "f9ad90123a27ea5ac3d48e280b288fc47e89933913322f91f8fd1114e6a837af");
    const Fw::StringTemplate<64> digestArg(digestHex.c_str());
    const Fw::StringTemplate<128> stagingPathArg(stagingName.c_str());
    this->writeSignedManifest_(stagingName, digestHex, 13U, 1U);

    this->clearHistory();
    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, digestArg);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_PREPARE_UPDATE, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_UPDATE_PREPARED_SIZE(1);
    ASSERT_TLM_BOOT_UPDATE_PROGRESS_SIZE(1);
    ASSERT_TLM_BOOT_UPDATE_PROGRESS(0, 10U);

    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, stagingPathArg);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_STAGE_VERIFY_OK_SIZE(1);
    ASSERT_EVENTS_BOOT_STAGE_VERIFY_OK(0, OBC::BootSlot::SLOT_B);
    ASSERT_EVENTS_BOOT_TRUST_ACCEPTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_ACCEPTED(0, OBC::BootSlot::SLOT_B, 1U, 1U);
    ASSERT_TLM_BOOT_UPDATE_PROGRESS_SIZE(1);
    ASSERT_TLM_BOOT_UPDATE_PROGRESS(0, 60U);
    ASSERT_TLM_BOOT_TRUST_STATUS_SIZE(1);
    ASSERT_TLM_BOOT_TRUST_STATUS(0, BOOT_TRUST_STATUS_TRUSTED);

    this->clearHistory();
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_ACTIVATE_STAGED_IMAGE, 2, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_SLOT_SWITCHED_SIZE(1);
    ASSERT_TLM_BOOT_ACTIVE_SLOT_SIZE(1);
    ASSERT_TLM_BOOT_ACTIVE_SLOT(0, OBC::BootSlot::SLOT_B);
    ASSERT_TLM_BOOT_PENDING_SLOT_SIZE(1);
    ASSERT_TLM_BOOT_PENDING_SLOT(0, OBC::BootSlot::SLOT_B);
    ASSERT_TLM_BOOT_CONFIRMED_SIZE(1);
    ASSERT_TLM_BOOT_CONFIRMED(0, false);
    ASSERT_TLM_BOOT_LAST_ACCEPTED_VERSION_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ACCEPTED_VERSION(0, 1U);

    this->clearHistory();
    this->sendCmd_BOOT_CONFIRM(TEST_INSTANCE_ID, 3);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_CONFIRM, 3, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_VERSION_CONFIRMED_SIZE(1);
    ASSERT_EVENTS_BOOT_VERSION_CONFIRMED(0, OBC::BootSlot::SLOT_B);
    ASSERT_TLM_BOOT_PENDING_SLOT_SIZE(1);
    ASSERT_TLM_BOOT_PENDING_SLOT(0, OBC::BootSlot::NONE);
    ASSERT_TLM_BOOT_CONFIRMED_SIZE(1);
    ASSERT_TLM_BOOT_CONFIRMED(0, true);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(0);
}

void BootManagerTester::testConfirmTimeoutRollsBack() {
    const std::string stagingName = "timeout-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v2");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    const Fw::StringTemplate<64> digestArg(digestHex.c_str());
    const Fw::StringTemplate<128> stagingPathArg(stagingName.c_str());
    this->writeSignedManifest_(stagingName, digestHex, 13U, 1U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, digestArg);
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, stagingPathArg);
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    this->clearHistory();
    for (U32 i = 0; i < 60U; i++) {
        this->component.tickForTest();
    }

    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED_SIZE(1);
    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED(0, OBC::BootSlot::SLOT_A, 10U);
    ASSERT_TLM_BOOT_ACTIVE_SLOT_SIZE(1);
    ASSERT_TLM_BOOT_ACTIVE_SLOT(0, OBC::BootSlot::SLOT_A);
    ASSERT_TLM_BOOT_PENDING_SLOT_SIZE(1);
    ASSERT_TLM_BOOT_PENDING_SLOT(0, OBC::BootSlot::NONE);
    ASSERT_TLM_BOOT_CONFIRMED_SIZE(1);
    ASSERT_TLM_BOOT_CONFIRMED(0, true);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ERROR(0, 10U);
}

void BootManagerTester::testInvalidMetadataRollsBackToSafeSlot() {
    this->writeFile_(this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt",
                     "active_slot=BAD_SLOT\n"
                     "pending_slot=SLOT_B\n"
                     "last_known_good_slot=SLOT_A\n"
                     "confirmed=0\n"
                     "expected_digest=deadbeef\n"
                     "expected_size=12\n"
                     "last_boot_attempt_time=0\n"
                     "last_error_code=0\n"
                     "stage_verified=1\n"
                     "staged_path=/tmp/staged.bin\n");

    this->clearHistory();
    ASSERT_TRUE(this->component.reloadMetadataForTest());

    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED_SIZE(1);
    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED(0, OBC::BootSlot::SLOT_A, 1U);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ERROR(0, 1U);
}

void BootManagerTester::testLegacyPendingMetadataRollsBackToLastKnownGood() {
    this->writeFile_(this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt",
                     "active_slot=SLOT_B\n"
                     "pending_slot=SLOT_B\n"
                     "last_known_good_slot=SLOT_A\n"
                     "confirmed=0\n"
                     "expected_digest=deadbeef\n"
                     "expected_size=12\n"
                     "last_boot_attempt_time=0\n"
                     "last_error_code=0\n"
                     "stage_verified=1\n"
                     "staged_path=/tmp/staged.bin\n");

    this->clearHistory();
    ASSERT_TRUE(this->component.reloadMetadataForTest());

    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED_SIZE(1);
    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED(0, OBC::BootSlot::SLOT_A, 1U);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ERROR(0, 1U);
    ASSERT_EQ(this->component.getMetadataForRuntime().activeSlot, OBC::BootSlot::SLOT_A);
    ASSERT_EQ(this->component.getMetadataForRuntime().pendingSlot, OBC::BootSlot::NONE);
    ASSERT_FALSE(this->component.getMetadataForRuntime().stageVerified);
}

void BootManagerTester::testRollbackPersistsCleanMetadataFile() {
    const std::string stagingName = "rollback-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v3");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    const Fw::StringTemplate<64> digestArg(digestHex.c_str());
    const Fw::StringTemplate<128> stagingPathArg(stagingName.c_str());
    this->writeSignedManifest_(stagingName, digestHex, 13U, 1U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, digestArg);
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, stagingPathArg);
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    this->clearHistory();
    this->sendCmd_BOOT_ROLLBACK(TEST_INSTANCE_ID, 3);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_ROLLBACK, 3, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED_SIZE(1);

    OBC::BootMetadataStore store(this->m_tempRoot + "/persistent-data/boot");
    OBC::BootMetadata persisted = {};
    ASSERT_EQ(store.load(persisted), OBC::BootMetadataStore::LoadStatus::OK);
    ASSERT_EQ(persisted.activeSlot, OBC::BootSlot::SLOT_A);
    ASSERT_EQ(persisted.pendingSlot, OBC::BootSlot::NONE);
    ASSERT_TRUE(persisted.confirmed);
    ASSERT_FALSE(persisted.stageVerified);
    ASSERT_TRUE(persisted.stagedPath.empty());
    ASSERT_EQ(persisted.trustStatus, BOOT_TRUST_STATUS_CONFIRMED);
    ASSERT_EQ(persisted.trustRejectReason, 9U);
}

void BootManagerTester::testInvalidSignatureRejected() {
    const std::string stagingName = "bad-signature.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v4");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 13U, 2U, OBC::BootSlot::SLOT_B, "repo-dev-boot-signer", 1U, false);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_SIGNATURE_INVALID, 2U, 1U);
    ASSERT_TLM_BOOT_TRUST_STATUS_SIZE(1);
    ASSERT_TLM_BOOT_TRUST_STATUS(0, BOOT_TRUST_STATUS_REJECTED);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON_SIZE(1);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON(0, BOOT_TRUST_REJECT_SIGNATURE_INVALID);
}

void BootManagerTester::testUnknownSignerRejected() {
    const std::string stagingName = "unknown-signer.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v5");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 13U, 2U, OBC::BootSlot::SLOT_B, "unknown-signer", 99U, true);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_SIGNER_UNKNOWN, 2U, 99U);
}

void BootManagerTester::testMalformedManifestRejected() {
    const std::string stagingName = "malformed.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v6");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeFile_(stagingPath + ".manifest-v1", "schema=boot_manifest_v1\nimage_path=malformed.bin\n");

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_MANIFEST_MALFORMED, 0U, 0U);
}

void BootManagerTester::testManifestDigestMismatchRejected() {
    const std::string stagingName = "manifest-digest-mismatch.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v10");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName,
                               "1111111111111111111111111111111111111111111111111111111111111111",
                               14U,
                               2U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 14U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH, 2U, 1U);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON_SIZE(1);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON(0, BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH);
}

void BootManagerTester::testDowngradeRejected() {
    const std::string firstName = "version-two.bin";
    const std::string firstPath = this->makeStagingFile_(firstName, "boot-image-v7");
    std::string firstDigest;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(firstPath, firstDigest));
    this->writeSignedManifest_(firstName, firstDigest, 13U, 2U);
    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, Fw::StringTemplate<64>(firstDigest.c_str()));
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(firstName.c_str()));
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);
    this->sendCmd_BOOT_CONFIRM(TEST_INSTANCE_ID, 3);

    const std::string secondName = "version-one.bin";
    const std::string secondPath = this->makeStagingFile_(secondName, "boot-image-v8");
    std::string secondDigest;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(secondPath, secondDigest));
    this->writeSignedManifest_(secondName, secondDigest, 13U, 1U, OBC::BootSlot::SLOT_A);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 4, 13U, Fw::StringTemplate<64>(secondDigest.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 5, Fw::StringTemplate<128>(secondName.c_str()));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_VERIFY_STAGED_IMAGE, 5, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_VERSION_DOWNGRADE, 1U, 1U);
}

void BootManagerTester::testPostVerifyTamperDoesNotActivate() {
    const std::string stagingName = "post-verify-tamper.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v11");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 14U, 4U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 14U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));

    this->writeFile_(stagingPath, "boot-image-x11");
    this->clearHistory();
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_ACTIVATE_STAGED_IMAGE, 2, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED_SIZE(1);
    ASSERT_EVENTS_BOOT_TRUST_REJECTED(0, BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH, 0U, 0U);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON_SIZE(1);
    ASSERT_TLM_BOOT_TRUST_REJECT_REASON(0, BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH);
    ASSERT_EQ(this->component.getMetadataForRuntime().activeSlot, OBC::BootSlot::SLOT_A);
    ASSERT_EQ(this->component.getMetadataForRuntime().pendingSlot, OBC::BootSlot::NONE);
}

void BootManagerTester::testInvalidTrustStateDoesNotActivate() {
    this->clearHistory();
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_ACTIVATE_STAGED_IMAGE, 0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_TLM_BOOT_ACTIVE_SLOT_SIZE(0);
}

void BootManagerTester::testMetadataReloadPreservesPendingTrustState() {
    const std::string stagingName = "reload-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v9");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 13U, 3U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 13U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    ASSERT_TRUE(this->component.reloadMetadataForTest());
    const OBC::BootMetadata& metadata = this->component.getMetadataForRuntime();
    ASSERT_EQ(metadata.pendingSlot, OBC::BootSlot::SLOT_B);
    ASSERT_EQ(metadata.pendingSoftwareVersion, 3U);
    ASSERT_EQ(metadata.lastAcceptedVersion, 3U);
    ASSERT_EQ(metadata.trustStatus, BOOT_TRUST_STATUS_PENDING);
    ASSERT_EQ(this->component.getRemainingConfirmSecondsForRuntime(), 60U);
}

void BootManagerTester::testNestedRelativeManifestPathActivates() {
    ASSERT_EQ(Os::FileSystem::createDirectory((this->m_tempRoot + "/persistent-data/boot/subdir").c_str()),
              Os::FileSystem::Status::OP_OK);
    const std::string stagingName = "subdir/nested-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v13");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 14U, 6U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 14U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));
    this->clearHistory();
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_ACTIVATE_STAGED_IMAGE, 2, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_SLOT_SWITCHED_SIZE(1);
    ASSERT_EQ(this->component.getMetadataForRuntime().activeSlot, OBC::BootSlot::SLOT_B);
    ASSERT_EQ(this->component.getMetadataForRuntime().pendingSlot, OBC::BootSlot::SLOT_B);
    ASSERT_EQ(this->component.getMetadataForRuntime().manifestImagePath, stagingName);
}

void BootManagerTester::testInvalidPendingMetadataRollsBack() {
    this->writeFile_(this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt",
                     "schema_version=2\n"
                     "active_slot=SLOT_B\n"
                     "pending_slot=SLOT_B\n"
                     "last_known_good_slot=SLOT_A\n"
                     "confirmed=0\n"
                     "expected_digest=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
                     "expected_size=14\n"
                     "last_boot_attempt_time=0\n"
                     "last_error_code=0\n"
                     "stage_verified=0\n"
                     "staged_path=\n"
                     "manifest_path=\n"
                     "trust_status=2\n"
                     "trust_reject_reason=14\n"
                     "image_id=bad-pending\n"
                     "staged_software_version=0\n"
                     "active_software_version=2\n"
                     "pending_software_version=2\n"
                     "last_known_good_software_version=1\n"
                     "last_accepted_version=2\n"
                     "signer_id=repo-dev-boot-signer\n"
                     "key_slot=1\n"
                     "signature_algorithm=hmac-sha256\n");

    this->clearHistory();
    ASSERT_TRUE(this->component.reloadMetadataForTest());

    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED_SIZE(1);
    ASSERT_EVENTS_BOOT_ROLLBACK_TRIGGERED(0, OBC::BootSlot::SLOT_A, 1U);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ERROR(0, 1U);
    ASSERT_EQ(this->component.getMetadataForRuntime().activeSlot, OBC::BootSlot::SLOT_A);
    ASSERT_EQ(this->component.getMetadataForRuntime().pendingSlot, OBC::BootSlot::NONE);
    ASSERT_EQ(this->component.getMetadataForRuntime().trustStatus, BOOT_TRUST_STATUS_CONFIRMED);
}

void BootManagerTester::testPrepareRejectedDuringPendingConfirm() {
    const std::string stagingName = "pending-prepare-image.bin";
    const std::string stagingPath = this->makeStagingFile_(stagingName, "boot-image-v12");
    std::string digestHex;
    ASSERT_TRUE(OBC::BootMetadataStore::computeDigestHex(stagingPath, digestHex));
    this->writeSignedManifest_(stagingName, digestHex, 14U, 5U);

    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID, 0, 14U, Fw::StringTemplate<64>(digestHex.c_str()));
    this->sendCmd_BOOT_VERIFY_STAGED_IMAGE(TEST_INSTANCE_ID, 1, Fw::StringTemplate<128>(stagingName.c_str()));
    this->sendCmd_BOOT_ACTIVATE_STAGED_IMAGE(TEST_INSTANCE_ID, 2);

    this->clearHistory();
    this->sendCmd_BOOT_PREPARE_UPDATE(TEST_INSTANCE_ID,
                                      3,
                                      14U,
                                      Fw::StringTemplate<64>(
                                          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_PREPARE_UPDATE, 3, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_TLM_BOOT_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BOOT_LAST_ERROR(0, 21U);
    ASSERT_EQ(this->component.getMetadataForRuntime().pendingSlot, OBC::BootSlot::SLOT_B);
    ASSERT_EQ(this->component.getMetadataForRuntime().trustStatus, BOOT_TRUST_STATUS_PENDING);

    this->clearHistory();
    this->sendCmd_BOOT_CONFIRM(TEST_INSTANCE_ID, 4);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_CONFIRM, 4, Fw::CmdResponse::OK);
}

void BootManagerTester::testRecoveryIntentPersistsMetadata() {
    this->clearHistory();
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_EPS_TIMEOUT,
                                                                     OBC::RecoveryIncidentSource::EPS_TIMEOUT,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));

    const OBC::BootMetadata& metadata = this->component.getMetadataForRuntime();
    ASSERT_EQ(metadata.resetCause, OBC::ResetCause::RECOVERY_EPS_TIMEOUT);
    ASSERT_EQ(metadata.lastRecoverySource, OBC::RecoveryIncidentSource::EPS_TIMEOUT);
    ASSERT_EQ(metadata.lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EVENTS_BOOT_RECOVERY_INTENT_RECORDED_SIZE(1);

    OBC::BootMetadataStore store(this->m_tempRoot + "/persistent-data/boot");
    OBC::BootMetadata persisted = {};
    ASSERT_EQ(store.load(persisted), OBC::BootMetadataStore::LoadStatus::OK);
    ASSERT_EQ(persisted.resetCause, OBC::ResetCause::RECOVERY_EPS_TIMEOUT);
    ASSERT_EQ(persisted.lastRecoverySource, OBC::RecoveryIncidentSource::EPS_TIMEOUT);
    ASSERT_EQ(persisted.lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
}

void BootManagerTester::testRecoveryProcessRestartPersistsR2Metadata() {
    this->clearHistory();
    ASSERT_TRUE(this->component.recordRecoveryRestartIntentForRuntime(
        OBC::ResetCause::RECOVERY_ADCS_FDIR,
        OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT,
        OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT));

    const OBC::BootMetadata& metadata = this->component.getMetadataForRuntime();
    ASSERT_EQ(metadata.resetCause, OBC::ResetCause::RECOVERY_ADCS_FDIR);
    ASSERT_EQ(metadata.lastRecoverySource, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(metadata.lastRecoveryLevel, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_TRUE(metadata.recoveryResetPending);

    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    ASSERT_EQ(this->component.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_ADCS_FDIR);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), 1U);
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoverySource,
              OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoveryLevel,
              OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_FALSE(this->component.getMetadataForRuntime().recoveryResetPending);
}

void BootManagerTester::testConsecutiveRecoveryBootsRequireSafeFallback() {
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                         OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                         OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    }

    ASSERT_EQ(this->component.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(this->component.getBootCountForRuntime(), 4U);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), 3U);
    ASSERT_TRUE(this->component.isBootSafeFallbackRequiredForRuntime());
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoverySource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
}

void BootManagerTester::testStableAckClearsRecoveryFallback() {
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                         OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                         OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    }

    this->clearHistory();
    ASSERT_TRUE(this->component.acknowledgeRuntimeStableForRuntime());
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), 0U);
    ASSERT_FALSE(this->component.isBootSafeFallbackRequiredForRuntime());
    ASSERT_EVENTS_BOOT_RECOVERY_STABLE_ACK_SIZE(1);

    OBC::BootMetadataStore store(this->m_tempRoot + "/persistent-data/boot");
    OBC::BootMetadata persisted = {};
    ASSERT_EQ(store.load(persisted), OBC::BootMetadataStore::LoadStatus::OK);
    ASSERT_EQ(persisted.consecutiveResetCount, 0U);
    ASSERT_FALSE(persisted.bootSafeFallbackRequired);
}

void BootManagerTester::testBootObservedBreadcrumbWritten() {
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 123U, 0U));
    for (U32 i = 0U; i < 7U; i++) {
        this->component.tickForTest();
    }
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                     OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));
    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    ASSERT_EQ(this->m_faultRecorder.records.size(), 1U);
    const OBC::PersistentFaultRecord& record = this->m_faultRecorder.records.back();
    ASSERT_EQ(record.kind, OBC::PersistentFaultRecordKind::BOOT_OBSERVED);
    ASSERT_EQ(record.source, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(record.level, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(record.resetCause, OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(record.timestampSec, 123U);
    ASSERT_EQ(record.uptimeSec, 7U);
    ASSERT_EQ(record.bootCount, this->component.getBootCountForRuntime());
    ASSERT_EQ(record.consecutiveResetCount, this->component.getConsecutiveResetCountForRuntime());
    ASSERT_EQ(record.flags, 0U);
}

void BootManagerTester::testStableAckBreadcrumbWritten() {
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 456U, 0U));
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                         OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                         OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    }
    for (U32 i = 0U; i < 5U; i++) {
        this->component.tickForTest();
    }

    this->clearHistory();
    ASSERT_TRUE(this->component.acknowledgeRuntimeStableForRuntime());
    ASSERT_EQ(this->m_faultRecorder.records.size(), 4U);
    const OBC::PersistentFaultRecord& record = this->m_faultRecorder.records.back();
    ASSERT_EQ(record.kind, OBC::PersistentFaultRecordKind::RECOVERY_BOOT_ACK);
    ASSERT_EQ(record.source, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(record.level, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(record.resetCause, OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(record.timestampSec, 456U);
    ASSERT_EQ(record.uptimeSec, 5U);
    ASSERT_EQ(record.bootCount, this->component.getBootCountForRuntime());
    ASSERT_EQ(record.consecutiveResetCount, 0U);
    ASSERT_EQ(record.detail, 3U);
}

void BootManagerTester::testBootStatusCommandPublishesForcedRefreshTelemetry() {
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                     OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));
    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());

    this->clearHistory();
    this->sendCmd_BOOT_STATUS(TEST_INSTANCE_ID, 7);
    this->sendCmd_BOOT_STATUS(TEST_INSTANCE_ID, 8);

    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_BOOT_STATUS, 7, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_BOOT_STATUS, 8, Fw::CmdResponse::OK);
    ASSERT_TLM_BOOT_RESET_CAUSE_SIZE(2);
    ASSERT_TLM_BOOT_RESET_CAUSE(0, OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_TLM_BOOT_RESET_CAUSE(1, OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_TLM_BOOT_BOOT_COUNT_SIZE(2);
    ASSERT_TLM_BOOT_BOOT_COUNT(0, 2U);
    ASSERT_TLM_BOOT_BOOT_COUNT(1, 2U);
    ASSERT_TLM_BOOT_SAFE_FALLBACK_REQUIRED_SIZE(2);
    ASSERT_TLM_BOOT_SAFE_FALLBACK_REQUIRED(0, false);
    ASSERT_TLM_BOOT_SAFE_FALLBACK_REQUIRED(1, false);
}

void BootManagerTester::testResetCauseAndBootCountCommands() {
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                     OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));
    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());

    this->clearHistory();
    this->sendCmd_GET_RESET_CAUSE(TEST_INSTANCE_ID, 9);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GET_RESET_CAUSE, 9, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_RECOVERY_STATUS_SIZE(1);
    ASSERT_EQ(this->component.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);

    this->clearHistory();
    this->sendCmd_GET_BOOT_COUNT(TEST_INSTANCE_ID, 10);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GET_BOOT_COUNT, 10, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BOOT_RECOVERY_STATUS_SIZE(1);
    ASSERT_EQ(this->component.getBootCountForRuntime(), 2U);
}

void BootManagerTester::testRecoveryCauseConsumedByNextNormalBoot() {
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                     OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));
    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    ASSERT_EQ(this->component.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), 1U);

    ASSERT_TRUE(this->component.simulateRuntimeBootForTest());
    ASSERT_EQ(this->component.getResetCauseForRuntime(), OBC::ResetCause::UNKNOWN);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), 0U);
    ASSERT_FALSE(this->component.isBootSafeFallbackRequiredForRuntime());
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoverySource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(this->component.getMetadataForRuntime().lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
}

void BootManagerTester::testRuntimeBootInitializationRetriesAfterPersistFailure() {
    ASSERT_TRUE(this->component.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                     OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                     OBC::RecoveryLevel::R6_OBC_REBOOT));
    const U32 bootCountBeforeRetry = this->component.getBootCountForRuntime();
    const U32 consecutiveBeforeRetry = this->component.getConsecutiveResetCountForRuntime();

    const std::string bootRoot = this->m_tempRoot + "/persistent-data/boot";
    const std::string invalidRoot = bootRoot + "/metadata-v1.txt";
    this->component.overrideStorageRootForPersistenceForTest(invalidRoot);
    ASSERT_FALSE(this->component.retryRuntimeBootInitializationForTest());
    ASSERT_EQ(this->component.getBootCountForRuntime(), bootCountBeforeRetry);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), consecutiveBeforeRetry);
    ASSERT_TRUE(this->component.getMetadataForRuntime().recoveryResetPending);

    this->component.overrideStorageRootForPersistenceForTest(bootRoot);
    ASSERT_TRUE(this->component.retryRuntimeBootInitializationForTest());
    ASSERT_EQ(this->component.getBootCountForRuntime(), bootCountBeforeRetry + 1U);
    ASSERT_EQ(this->component.getConsecutiveResetCountForRuntime(), consecutiveBeforeRetry + 1U);
    ASSERT_FALSE(this->component.getMetadataForRuntime().recoveryResetPending);
}

std::string BootManagerTester::makeTempRoot_() const {
    char buffer[] = "/tmp/boot-manager-ut-XXXXXX";
    const char* const path = mkdtemp(buffer);
    EXPECT_NE(path, nullptr);
    return path == nullptr ? "/tmp/boot-manager-ut-fallback" : std::string(path);
}

std::string BootManagerTester::makeStagingFile_(const std::string& fileName, const std::string& contents) const {
    const std::string path = this->m_tempRoot + "/persistent-data/boot/" + fileName;
    this->writeFile_(path, contents);
    return path;
}

void BootManagerTester::writeSignedManifest_(const std::string& fileName,
                                             const std::string& digestHex,
                                             U32 imageSize,
                                             U32 softwareVersion,
                                             OBC::BootSlot targetSlot,
                                             const std::string& signerId,
                                             U32 keySlot,
                                             bool validSignature) const {
    OBC::BootTrustConfig config;
    OBC::BootManifest manifest;
    manifest.imagePath = fileName;
    manifest.imageSize = imageSize;
    manifest.imageDigest = digestHex;
    manifest.targetSlot = targetSlot;
    manifest.imageId = "test-image";
    manifest.softwareVersion = softwareVersion;
    manifest.signerId = signerId;
    manifest.keySlot = keySlot;
    manifest.signatureAlgorithm = "hmac-sha256";

    std::string signature;
    if (signerId == config.trustedSignerId && keySlot == config.trustedKeySlot) {
        ASSERT_TRUE(OBC::computeBootManifestSignatureHex(manifest, config, signature));
    } else {
        signature = "0000000000000000000000000000000000000000000000000000000000000000";
    }
    if (!validSignature) {
        signature[0] = signature[0] == '0' ? '1' : '0';
    }
    manifest.signature = signature;

    const char* const targetSlotName = targetSlot == OBC::BootSlot::SLOT_A ? "SLOT_A" : "SLOT_B";
    this->writeFile_(this->m_tempRoot + "/persistent-data/boot/" + fileName + ".manifest-v1",
                     "schema=boot_manifest_v1\n"
                     "image_path=" + manifest.imagePath + "\n" +
                         "image_size=" + std::to_string(manifest.imageSize) + "\n" +
                         "image_digest_sha256=" + manifest.imageDigest + "\n" +
                         "target_slot=" + targetSlotName + "\n" +
                         "image_id=" + manifest.imageId + "\n" +
                         "software_version=" + std::to_string(manifest.softwareVersion) + "\n" +
                         "signer_id=" + manifest.signerId + "\n" +
                         "key_slot=" + std::to_string(manifest.keySlot) + "\n" +
                         "signature_algorithm=" + manifest.signatureAlgorithm + "\n" +
                         "signature=" + manifest.signature + "\n");
}

void BootManagerTester::writeFile_(const std::string& path, const std::string& contents) const {
    Os::File file;
    ASSERT_EQ(file.open(path.c_str(), Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::Status::OP_OK);

    FwSizeType size = static_cast<FwSizeType>(contents.size());
    ASSERT_EQ(file.write(reinterpret_cast<const U8*>(contents.data()), size, Os::File::WaitType::WAIT),
              Os::File::Status::OP_OK);
    ASSERT_EQ(size, contents.size());
    file.close();
}

}  // namespace OBC
