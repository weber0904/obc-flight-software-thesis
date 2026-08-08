#include "StorageHealthBridgeTester.hpp"

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace OBC {

namespace {

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

}  // namespace

StorageHealthBridgeTester::StorageHealthBridgeTester()
    : StorageHealthBridgeGTestBase("StorageHealthBridgeTester", MAX_HISTORY_SIZE), component("StorageHealthBridge") {
    this->initComponents();
    this->connectPorts();
}

StorageHealthBridgeTester::~StorageHealthBridgeTester() = default;

void StorageHealthBridgeTester::testGetStatusRequiresConfiguration() {
    this->clearHistory();
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(0);
    ASSERT_EVENTS_STORAGE_ROOT_MISSING_SIZE(0);
    ASSERT_TLM_STORAGE_HAVE_SCAN_SIZE(0);
}

void StorageHealthBridgeTester::testGetStatusReportsMissingRootAndWarning() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(dataProductsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->writeFile_(joinPath(persistentRoot, "data.bin"), "1234");
    this->writeFile_(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF0123456789ABCDEF");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 16U);
    this->clearHistory();
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(1);
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED(0, 1U, static_cast<U8>(1U << 1U));
    ASSERT_EVENTS_STORAGE_ROOT_MISSING_SIZE(1);
    ASSERT_EVENTS_STORAGE_ROOT_MISSING(0, OBC::StorageRootKind::LOGS);
    ASSERT_EVENTS_STORAGE_SCAN_FAILED_SIZE(0);
    ASSERT_EVENTS_STORAGE_WARNING_THRESHOLD_EXCEEDED_SIZE(1);
    ASSERT_EVENTS_STORAGE_WARNING_THRESHOLD_EXCEEDED(0, OBC::StorageRootKind::STAGING, 32U, 16U);
    ASSERT_TLM_STORAGE_HAVE_SCAN_SIZE(1);
    ASSERT_TLM_STORAGE_HAVE_SCAN(0, 1U);
    ASSERT_TLM_STORAGE_WARNING_ACTIVE_SIZE(1);
    ASSERT_TLM_STORAGE_WARNING_ACTIVE(0, 1U);
    ASSERT_TLM_STORAGE_DEGRADED_MASK_SIZE(1);
    ASSERT_TLM_STORAGE_DEGRADED_MASK(0, static_cast<U8>(1U << 2U));
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_SCAN_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_SCAN_ERRORS_SIZE(1);
    ASSERT_TLM_STORAGE_SCAN_ERRORS(0, 1U);

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testGetStatusPerformsFreshScanAndPublishesDetailedTelemetry() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(logsRoot);
    this->ensureDir_(dataProductsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->writeFile_(joinPath(logsRoot, "runtime.log"), "zzzz");
    this->writeFile_(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789ABCDEF");
    this->writeFile_(joinPath(persistentRoot, "data.bin"), "123456");
    this->writeFile_(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 8U, 15U);

    this->clearHistory();
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 2);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 2, Fw::CmdResponse::OK);
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(1);
    ASSERT_TLM_STORAGE_PERSISTENT_FILE_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_PERSISTENT_FILE_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_LOG_FILE_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_LOG_FILE_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_FILE_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_FILE_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_BYTES_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_BYTES(0, 16U);
    ASSERT_TLM_STORAGE_WARNING_MASK_SIZE(1);
    ASSERT_TLM_STORAGE_WARNING_MASK(0, static_cast<U8>((1U << 1U) | (1U << 3U)));
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_SCAN_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(0, OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_RETENTION_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_RETENTION_STATUS(0, OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY);

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testGetStatusReplaysTelemetryWhenValuesUnchanged() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(logsRoot);
    this->ensureDir_(dataProductsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->writeFile_(joinPath(logsRoot, "runtime.log"), "zzzz");
    this->writeFile_(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789ABCDEF");
    this->writeFile_(joinPath(persistentRoot, "data.bin"), "123456");
    this->writeFile_(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 8U, 15U);
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 0);
    this->clearHistory();

    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 1, Fw::CmdResponse::OK);
    ASSERT_TLM_STORAGE_WARNING_ACTIVE_SIZE(1);
    ASSERT_TLM_STORAGE_WARNING_ACTIVE(0, 1U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(0, OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_FILE_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_FILE_COUNT(0, 1U);
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(1);
    ASSERT_TLM_STORAGE_SCAN_COUNT(0, 2U);

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testSchedInCadenceScansOnFirstAndFourthTickWithSummaryOnlyLive() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(logsRoot);
    this->ensureDir_(dataProductsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->writeFile_(joinPath(logsRoot, "runtime.log"), "zzzz");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 0U);
    this->clearHistory();

    this->component.schedTickForTest();
    OBC::STORAGE::HealthState firstState = {};
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(0);
    ASSERT_TRUE(this->component.getCachedStateForRuntime(firstState));
    ASSERT_TRUE(firstState.hasScan);
    ASSERT_EQ(firstState.scanCount, 1U);
    ASSERT_TLM_STORAGE_SCHED_TICKS_SIZE(1);
    ASSERT_TLM_STORAGE_SCHED_TICKS(0, 1U);
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(0);
    ASSERT_TLM_STORAGE_PERSISTENT_FILE_COUNT_SIZE(0);

    this->clearHistory();
    this->component.schedTickForTest();
    this->component.schedTickForTest();
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(0);
    ASSERT_TLM_STORAGE_SCHED_TICKS_SIZE(2);
    ASSERT_TLM_STORAGE_SCHED_TICKS(0, 2U);
    ASSERT_TLM_STORAGE_SCHED_TICKS(1, 3U);

    this->component.schedTickForTest();
    OBC::STORAGE::HealthState fourthState = {};
    ASSERT_EVENTS_STORAGE_SCAN_UPDATED_SIZE(0);
    ASSERT_TRUE(this->component.getCachedStateForRuntime(fourthState));
    ASSERT_TRUE(fourthState.hasScan);
    ASSERT_EQ(fourthState.scanCount, 2U);
    ASSERT_TLM_STORAGE_SCHED_TICKS_SIZE(3);
    ASSERT_TLM_STORAGE_SCHED_TICKS(2, 4U);
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(0);

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testSchedInPublishesOnlyChangeDrivenOperatorFields() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(logsRoot);
    this->ensureDir_(dataProductsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->writeFile_(joinPath(logsRoot, "runtime.log"), "zzzz");
    this->writeFile_(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789ABCDEF");
    this->writeFile_(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF0123456789ABCDEF");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 16U, 15U);
    this->clearHistory();

    this->component.schedTickForTest();

    ASSERT_TLM_STORAGE_WARNING_ACTIVE_SIZE(1);
    ASSERT_TLM_STORAGE_WARNING_ACTIVE(0, 1U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(0, OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_RETENTION_STATUS_SIZE(0);
    ASSERT_TLM_STORAGE_WARNING_MASK_SIZE(0);
    ASSERT_TLM_STORAGE_DEGRADED_MASK_SIZE(0);
    ASSERT_TLM_STORAGE_SCAN_COUNT_SIZE(0);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_FILE_COUNT_SIZE(0);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_BYTES_SIZE(0);

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testGetStatusReportsScanFailedEventForExistingNonDirectoryRoot() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");
    const std::string logsPath = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);
    this->ensureDir_(dataProductsRoot);
    this->writeFile_(logsPath, "not-a-directory");

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 0U);
    this->clearHistory();
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 4);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 4, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_STORAGE_ROOT_MISSING_SIZE(0);
    ASSERT_EVENTS_STORAGE_SCAN_FAILED_SIZE(1);
    ASSERT_EVENTS_STORAGE_SCAN_FAILED(0, OBC::StorageRootKind::LOGS, static_cast<U32>(ENOTDIR));

    this->cleanupTree_(root);
}

void StorageHealthBridgeTester::testGetStatusReportsDataProductsRootMissingEventAndTelemetry() {
    const std::string root = this->makeTempRoot_();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    this->ensureDir_(runtimeRoot);
    this->ensureDir_(logsRoot);
    this->ensureDir_(persistentRoot);
    this->ensureDir_(stagingRoot);

    this->component.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 0U, 100U);
    this->clearHistory();
    this->sendCmd_STORAGE_GET_STATUS(TEST_INSTANCE_ID, 5);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_STORAGE_GET_STATUS, 5, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_STORAGE_ROOT_MISSING_SIZE(1);
    ASSERT_EVENTS_STORAGE_ROOT_MISSING(0, OBC::StorageRootKind::DATA_PRODUCTS);
    ASSERT_TLM_STORAGE_DEGRADED_MASK_SIZE(1);
    ASSERT_TLM_STORAGE_DEGRADED_MASK(0, static_cast<U8>(1U << 3U));
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_EXISTS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_EXISTS(0, 0U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_SCAN_OK_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_SCAN_OK(0, 0U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_ERROR_CODE_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_ERROR_CODE(0, static_cast<U32>(ENOENT));
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_BYTES_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_BYTES(0, 100U);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(0, OBC::STORAGE::STORAGE_QUOTA_UNAVAILABLE);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_RETENTION_STATUS_SIZE(1);
    ASSERT_TLM_STORAGE_DATA_PRODUCTS_RETENTION_STATUS(0, OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY);

    this->cleanupTree_(root);
}

std::string StorageHealthBridgeTester::makeTempRoot_() const {
    char buffer[] = "/tmp/storage-health-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    if (path == nullptr) {
        ADD_FAILURE() << "mkdtemp failed";
        return {};
    }
    return std::string(path);
}

void StorageHealthBridgeTester::ensureDir_(const std::string& path) const {
    const int status = ::mkdir(path.c_str(), 0755);
    ASSERT_TRUE(status == 0 || errno == EEXIST);
}

void StorageHealthBridgeTester::writeFile_(const std::string& path, const std::string& contents) const {
    std::ofstream output(path, std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output << contents;
    ASSERT_TRUE(output.good());
}

void StorageHealthBridgeTester::cleanupTree_(const std::string& root) const {
    (void)std::remove((root + "/runtime/logs/runtime.log").c_str());
    (void)std::remove((root + "/runtime/logs").c_str());
    (void)std::remove((root + "/runtime/data-products/Dp_1.fdp").c_str());
    (void)std::remove((root + "/runtime/data-products").c_str());
    (void)std::remove((root + "/runtime").c_str());
    (void)std::remove((root + "/persistent-data/data.bin").c_str());
    (void)std::remove((root + "/persistent-data").c_str());
    (void)std::remove((root + "/staging/image.bin").c_str());
    (void)std::remove((root + "/staging").c_str());
    (void)std::remove(root.c_str());
}

}  // namespace OBC
