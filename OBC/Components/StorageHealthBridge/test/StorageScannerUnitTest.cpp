#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "OBC/Components/StorageHealthBridge/StorageScanner.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

std::string makeTempRoot() {
    char buffer[] = "/tmp/storage-scanner-ut-XXXXXX";
    const char* path = ::mkdtemp(buffer);
    return path == nullptr ? "/tmp/storage-scanner-ut-fallback" : std::string(path);
}

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

void writeFile(const std::string& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary);
    output << contents;
}

void cleanupTree(const std::string& root) {
    (void)std::remove((root + "/persistent-data/persist.bin").c_str());
    (void)std::remove((root + "/persistent-data").c_str());
    (void)std::remove((root + "/staging/staged.img").c_str());
    (void)std::remove((root + "/staging").c_str());
    (void)std::remove((root + "/runtime/logs/runtime.log").c_str());
    (void)std::remove((root + "/runtime/logs").c_str());
    (void)std::remove((root + "/runtime/data-products/Dp_1.fdp").c_str());
    (void)std::remove((root + "/runtime/data-products").c_str());
    (void)std::remove((root + "/runtime").c_str());
    (void)std::remove(root.c_str());
}

bool testScannerTracksCountsIndexAndWarnings() {
    bool ok = true;
    const std::string root = makeTempRoot();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    ::mkdir(runtimeRoot.c_str(), 0755);
    ::mkdir(logsRoot.c_str(), 0755);
    ::mkdir(dataProductsRoot.c_str(), 0755);
    ::mkdir(persistentRoot.c_str(), 0755);
    ::mkdir(stagingRoot.c_str(), 0755);

    writeFile(joinPath(persistentRoot, "persist.bin"), "1234");
    writeFile(joinPath(stagingRoot, "staged.img"), "0123456789");
    writeFile(joinPath(logsRoot, "runtime.log"), "hello");
    writeFile(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789ABCDEF");

    OBC::STORAGE::StorageScanner scanner;
    scanner.configure(runtimeRoot, persistentRoot, stagingRoot, 8U, 15U);

    OBC::STORAGE::HealthState state = {};
    ok = check(scanner.scan(state), "Expected scanner success with all roots present") && ok;
    ok = check(state.hasScan, "Expected hasScan after scanner run") && ok;
    ok = check(state.persistent.fileCount == 1U, "Expected persistent file count") && ok;
    ok = check(state.staging.fileCount == 1U, "Expected staging file count") && ok;
    ok = check(state.logs.fileCount == 1U, "Expected logs file count") && ok;
    ok = check(state.dataProducts.fileCount == 1U, "Expected data-products file count") && ok;
    ok = check(state.dataProducts.totalBytes == 16U, "Expected data-products byte count") && ok;
    ok = check(state.dataProducts.quotaBytes == 15U, "Expected data-products quota") && ok;
    ok = check(state.dataProducts.watermarkBytes == 8U, "Expected data-products watermark") && ok;
    ok = check(state.dataProducts.quotaStatus == OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA,
               "Expected data-products over-quota status") &&
         ok;
    ok = check(state.dataProducts.retentionStatus == OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY,
               "Expected observe-only retention status") &&
         ok;
    ok = check(state.warningActive, "Expected warning threshold to trigger") && ok;
    ok = check((state.warningMask & static_cast<U8>(1U << 1U)) != 0U, "Expected staging root warning bit") && ok;
    ok = check((state.warningMask & static_cast<U8>(1U << 3U)) != 0U,
               "Expected data-products root warning bit") &&
         ok;
    ok = check(state.degradedMask == 0U, "Expected no degraded roots when all roots exist") && ok;

    cleanupTree(root);
    return ok;
}

bool testScannerMarksMissingRootsAsDegraded() {
    bool ok = true;
    const std::string root = makeTempRoot();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    ::mkdir(runtimeRoot.c_str(), 0755);
    ::mkdir(persistentRoot.c_str(), 0755);
    ::mkdir(stagingRoot.c_str(), 0755);

    OBC::STORAGE::StorageScanner scanner;
    scanner.configure(runtimeRoot, persistentRoot, stagingRoot, 0U, 100U);

    OBC::STORAGE::HealthState state = {};
    ok = check(!scanner.scan(state), "Expected missing logs root to fail overall scan") && ok;
    ok = check(!state.logs.exists, "Expected missing logs root to remain not-present") && ok;
    ok = check(!state.dataProducts.exists, "Expected missing data-products root to remain not-present") && ok;
    ok = check((state.degradedMask & static_cast<U8>(1U << 2U)) != 0U, "Expected logs degraded bit") && ok;
    ok = check((state.degradedMask & static_cast<U8>(1U << 3U)) != 0U,
               "Expected data-products degraded bit") &&
         ok;
    ok = check(state.scanErrorCount == 2U, "Expected scan errors for missing logs and data-products roots") && ok;
    ok = check(state.logs.errorCode == static_cast<U32>(ENOENT), "Expected missing logs root errno") && ok;
    ok = check(state.dataProducts.errorCode == static_cast<U32>(ENOENT),
               "Expected missing data-products root errno") &&
         ok;
    ok = check(state.dataProducts.quotaStatus == OBC::STORAGE::STORAGE_QUOTA_UNAVAILABLE,
               "Expected unavailable quota status when configured quota cannot be evaluated") &&
         ok;

    cleanupTree(root);
    return ok;
}

bool testScannerKeepsExistingNonDirectoryRootDistinctFromMissing() {
    bool ok = true;
    const std::string root = makeTempRoot();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string logsRoot = joinPath(runtimeRoot, "logs");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    ::mkdir(runtimeRoot.c_str(), 0755);
    ::mkdir(dataProductsRoot.c_str(), 0755);
    ::mkdir(persistentRoot.c_str(), 0755);
    ::mkdir(stagingRoot.c_str(), 0755);

    writeFile(logsRoot, "not-a-directory");

    OBC::STORAGE::StorageScanner scanner;
    scanner.configure(runtimeRoot, persistentRoot, stagingRoot, 0U);

    OBC::STORAGE::HealthState state = {};
    ok = check(!scanner.scan(state), "Expected non-directory logs path to fail overall scan") && ok;
    ok = check(state.logs.exists, "Expected existing non-directory logs path not to be treated as missing") && ok;
    ok = check(!state.logs.scanOk, "Expected non-directory logs path scan to fail") && ok;
    ok = check(state.logs.errorCode == static_cast<U32>(ENOTDIR), "Expected ENOTDIR diagnostic code") && ok;
    ok = check((state.degradedMask & static_cast<U8>(1U << 2U)) != 0U, "Expected degraded bit for logs path") && ok;

    cleanupTree(root);
    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testScannerTracksCountsIndexAndWarnings() && ok;
    ok = testScannerMarksMissingRootsAsDegraded() && ok;
    ok = testScannerKeepsExistingNonDirectoryRootDistinctFromMissing() && ok;
    return ok ? 0 : 1;
}
