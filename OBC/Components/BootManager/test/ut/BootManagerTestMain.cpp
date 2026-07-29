#include "BootManagerTester.hpp"

#include <gtest/gtest.h>

TEST(BootManager, PrepareVerifyActivateConfirmFlow) {
    OBC::BootManagerTester tester;
    tester.testPrepareVerifyActivateConfirmFlow();
}

TEST(BootManager, ConfirmTimeoutRollsBack) {
    OBC::BootManagerTester tester;
    tester.testConfirmTimeoutRollsBack();
}

TEST(BootManager, InvalidMetadataRollsBackToSafeSlot) {
    OBC::BootManagerTester tester;
    tester.testInvalidMetadataRollsBackToSafeSlot();
}

TEST(BootManager, LegacyPendingMetadataRollsBackToLastKnownGood) {
    OBC::BootManagerTester tester;
    tester.testLegacyPendingMetadataRollsBackToLastKnownGood();
}

TEST(BootManager, RollbackPersistsCleanMetadataFile) {
    OBC::BootManagerTester tester;
    tester.testRollbackPersistsCleanMetadataFile();
}

TEST(BootManager, InvalidSignatureRejected) {
    OBC::BootManagerTester tester;
    tester.testInvalidSignatureRejected();
}

TEST(BootManager, UnknownSignerRejected) {
    OBC::BootManagerTester tester;
    tester.testUnknownSignerRejected();
}

TEST(BootManager, MalformedManifestRejected) {
    OBC::BootManagerTester tester;
    tester.testMalformedManifestRejected();
}

TEST(BootManager, ManifestDigestMismatchRejected) {
    OBC::BootManagerTester tester;
    tester.testManifestDigestMismatchRejected();
}

TEST(BootManager, DowngradeRejected) {
    OBC::BootManagerTester tester;
    tester.testDowngradeRejected();
}

TEST(BootManager, PostVerifyTamperDoesNotActivate) {
    OBC::BootManagerTester tester;
    tester.testPostVerifyTamperDoesNotActivate();
}

TEST(BootManager, InvalidTrustStateDoesNotActivate) {
    OBC::BootManagerTester tester;
    tester.testInvalidTrustStateDoesNotActivate();
}

TEST(BootManager, MetadataReloadPreservesPendingTrustState) {
    OBC::BootManagerTester tester;
    tester.testMetadataReloadPreservesPendingTrustState();
}

TEST(BootManager, NestedRelativeManifestPathActivates) {
    OBC::BootManagerTester tester;
    tester.testNestedRelativeManifestPathActivates();
}

TEST(BootManager, InvalidPendingMetadataRollsBack) {
    OBC::BootManagerTester tester;
    tester.testInvalidPendingMetadataRollsBack();
}

TEST(BootManager, PrepareRejectedDuringPendingConfirm) {
    OBC::BootManagerTester tester;
    tester.testPrepareRejectedDuringPendingConfirm();
}

TEST(BootManager, RecoveryIntentPersistsMetadata) {
    OBC::BootManagerTester tester;
    tester.testRecoveryIntentPersistsMetadata();
}

TEST(BootManager, RecoveryProcessRestartPersistsR2Metadata) {
    OBC::BootManagerTester tester;
    tester.testRecoveryProcessRestartPersistsR2Metadata();
}

TEST(BootManager, ConsecutiveRecoveryBootsRequireSafeFallback) {
    OBC::BootManagerTester tester;
    tester.testConsecutiveRecoveryBootsRequireSafeFallback();
}

TEST(BootManager, StableAckClearsRecoveryFallback) {
    OBC::BootManagerTester tester;
    tester.testStableAckClearsRecoveryFallback();
}

TEST(BootManager, BootObservedBreadcrumbWritten) {
    OBC::BootManagerTester tester;
    tester.testBootObservedBreadcrumbWritten();
}

TEST(BootManager, StableAckBreadcrumbWritten) {
    OBC::BootManagerTester tester;
    tester.testStableAckBreadcrumbWritten();
}

TEST(BootManager, BootStatusCommandPublishesForcedRefreshTelemetry) {
    OBC::BootManagerTester tester;
    tester.testBootStatusCommandPublishesForcedRefreshTelemetry();
}

TEST(BootManager, ResetCauseAndBootCountCommands) {
    OBC::BootManagerTester tester;
    tester.testResetCauseAndBootCountCommands();
}

TEST(BootManager, RecoveryCauseConsumedByNextNormalBoot) {
    OBC::BootManagerTester tester;
    tester.testRecoveryCauseConsumedByNextNormalBoot();
}

TEST(BootManager, RuntimeBootInitializationRetriesAfterPersistFailure) {
    OBC::BootManagerTester tester;
    tester.testRuntimeBootInitializationRetriesAfterPersistFailure();
}
