#ifndef OBC_StorageHealthBridgeTester_HPP
#define OBC_StorageHealthBridgeTester_HPP

#include <string>

#include "OBC/Components/StorageHealthBridge/StorageHealthBridge.hpp"
#include "OBC/Components/StorageHealthBridge/StorageHealthBridgeGTestBase.hpp"

namespace OBC {

class StorageHealthBridgeTester final : public StorageHealthBridgeGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    StorageHealthBridgeTester();

    ~StorageHealthBridgeTester() override;

    void testGetStatusRequiresConfiguration();

    void testGetStatusReportsMissingRootAndWarning();

    void testGetStatusPerformsFreshScanAndPublishesDetailedTelemetry();

    void testGetStatusReplaysTelemetryWhenValuesUnchanged();

    void testSchedInCadenceScansOnFirstAndFourthTickWithSummaryOnlyLive();

    void testSchedInPublishesOnlyChangeDrivenOperatorFields();

    void testGetStatusReportsScanFailedEventForExistingNonDirectoryRoot();

    void testGetStatusReportsDataProductsRootMissingEventAndTelemetry();

  private:
    void connectPorts();

    void initComponents();

    std::string makeTempRoot_() const;

    void ensureDir_(const std::string& path) const;

    void writeFile_(const std::string& path, const std::string& contents) const;

    void cleanupTree_(const std::string& root) const;

  private:
    OBC::StorageHealthBridge component;
};

}  // namespace OBC

#endif
