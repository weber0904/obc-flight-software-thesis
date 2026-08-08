#include "PayloadOpsControllerTester.hpp"

TEST(PayloadOpsController, CaptureDeterministicWritesDualArtifactsAndAutoPublishesPreview) {
    OBC::PayloadOpsControllerTester tester;
    tester.testCaptureDeterministicWritesDualArtifactsAndAutoPublishesPreview();
}

TEST(PayloadOpsController, CaptureDeterministicAllowsZeroCaptureIndex) {
    OBC::PayloadOpsControllerTester tester;
    tester.testCaptureDeterministicAllowsZeroCaptureIndex();
}

TEST(PayloadOpsController, PublishCaptureRejectsSynchronousFirstSliceFailure) {
    OBC::PayloadOpsControllerTester tester;
    tester.testPublishCaptureRejectsSynchronousFirstSliceFailure();
}

TEST(PayloadOpsController, PublishRawPromotesStoredCapture) {
    OBC::PayloadOpsControllerTester tester;
    tester.testPublishRawPromotesStoredCapture();
}

TEST(PayloadOpsController, PublishRawRejectsFullResolution) {
    OBC::PayloadOpsControllerTester tester;
    tester.testPublishRawRejectsFullResolution();
}

TEST(PayloadOpsController, ReusingCaptureIndexOverwritesLocalArtifacts) {
    OBC::PayloadOpsControllerTester tester;
    tester.testReusingCaptureIndexOverwritesLocalArtifacts();
}

TEST(PayloadOpsController, CaptureDeterministicRequiresPrepare) {
    OBC::PayloadOpsControllerTester tester;
    tester.testCaptureDeterministicRequiresPrepare();
}

TEST(PayloadOpsController, SingleReadyAllowsAutoThenDeterministicWithoutReprepare) {
    OBC::PayloadOpsControllerTester tester;
    tester.testSingleReadyAllowsAutoThenDeterministicWithoutReprepare();
}

TEST(PayloadOpsController, RetiredImageTuningMaskIsRejected) {
    OBC::PayloadOpsControllerTester tester;
    tester.testRetiredImageTuningMaskIsRejected();
}

TEST(PayloadOpsController, SharedReadyPrepareUsesAutoWarmupDefaults) {
    OBC::PayloadOpsControllerTester tester;
    tester.testSharedReadyPrepareUsesAutoWarmupDefaults();
}

TEST(PayloadOpsController, SetCameraDefaultsRejectsWhilePrepared) {
    OBC::PayloadOpsControllerTester tester;
    tester.testSetCameraDefaultsRejectsWhilePrepared();
}

TEST(PayloadOpsController, RawSensorSessionStillRequiresSpecialPrepare) {
    OBC::PayloadOpsControllerTester tester;
    tester.testRawSensorSessionStillRequiresSpecialPrepare();
}

TEST(PayloadOpsController, PreviewPublishIgnoresUnrelatedDpWriterNotifications) {
    OBC::PayloadOpsControllerTester tester;
    tester.testPreviewPublishIgnoresUnrelatedDpWriterNotifications();
}

TEST(PayloadOpsController, PublishCaptureReadsLegacyManifestCompatibility) {
    OBC::PayloadOpsControllerTester tester;
    tester.testPublishCaptureReadsLegacyManifestCompatibility();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
