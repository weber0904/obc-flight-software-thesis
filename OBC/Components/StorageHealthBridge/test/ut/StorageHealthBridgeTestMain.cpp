#include "StorageHealthBridgeTester.hpp"

#include <gtest/gtest.h>

TEST(StorageHealthBridge, GetStatusRequiresConfiguration) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusRequiresConfiguration();
}

TEST(StorageHealthBridge, GetStatusReportsMissingRootAndWarning) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusReportsMissingRootAndWarning();
}

TEST(StorageHealthBridge, GetStatusPerformsFreshScanAndPublishesDetailedTelemetry) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusPerformsFreshScanAndPublishesDetailedTelemetry();
}

TEST(StorageHealthBridge, GetStatusReplaysTelemetryWhenValuesUnchanged) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusReplaysTelemetryWhenValuesUnchanged();
}

TEST(StorageHealthBridge, SchedInCadenceScansOnFirstAndFourthTickWithSummaryOnlyLive) {
    OBC::StorageHealthBridgeTester tester;
    tester.testSchedInCadenceScansOnFirstAndFourthTickWithSummaryOnlyLive();
}

TEST(StorageHealthBridge, SchedInPublishesOnlyChangeDrivenOperatorFields) {
    OBC::StorageHealthBridgeTester tester;
    tester.testSchedInPublishesOnlyChangeDrivenOperatorFields();
}

TEST(StorageHealthBridge, GetStatusReportsScanFailedEventForExistingNonDirectoryRoot) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusReportsScanFailedEventForExistingNonDirectoryRoot();
}

TEST(StorageHealthBridge, GetStatusReportsDataProductsRootMissingEventAndTelemetry) {
    OBC::StorageHealthBridgeTester tester;
    tester.testGetStatusReportsDataProductsRootMissingEventAndTelemetry();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
