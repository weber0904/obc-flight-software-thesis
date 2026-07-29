#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "OBC/Components/StorageHealthBridge/StorageHealthBridge.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

std::string makeTempRoot() {
    char buffer[] = "/tmp/storage-bridge-contract-XXXXXX";
    const char* path = ::mkdtemp(buffer);
    return path == nullptr ? std::string() : std::string(path);
}

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

bool ensureDir(const std::string& path, const std::string& message) {
    if (::mkdir(path.c_str(), 0755) == 0 || errno == EEXIST) {
        return true;
    }
    std::cerr << message << ": " << std::strerror(errno) << std::endl;
    return false;
}

bool writeFile(const std::string& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "Failed to open fixture file for write: " << path << std::endl;
        return false;
    }
    output << contents;
    if (!output.good()) {
        std::cerr << "Failed to write fixture file: " << path << std::endl;
        return false;
    }
    return true;
}

void cleanupTree(const std::string& root) {
    (void)std::remove((root + "/runtime/data-products/Dp_1.fdp").c_str());
    (void)std::remove((root + "/runtime/data-products").c_str());
    (void)std::remove((root + "/runtime").c_str());
    (void)std::remove((root + "/persistent-data/data.bin").c_str());
    (void)std::remove((root + "/persistent-data").c_str());
    (void)std::remove((root + "/staging/image.bin").c_str());
    (void)std::remove((root + "/staging").c_str());
    (void)std::remove(root.c_str());
}

bool testScanRequiresConfiguration() {
    bool ok = true;

    OBC::StorageHealthBridge bridge("storageHealthBridgeContractNoConfig");
    OBC::STORAGE::HealthState state = {};

    ok = check(!bridge.scanNowForTest(), "Expected scan to fail before runtime configuration") && ok;
    ok = check(!bridge.getCachedStateForRuntime(state), "Expected no cached storage state before any configured scan") &&
         ok;

    return ok;
}

bool testConfiguredScansAccumulateCountsWarningsAndErrors() {
    bool ok = true;
    const std::string root = makeTempRoot();
    const std::string runtimeRoot = joinPath(root, "runtime");
    const std::string dataProductsRoot = joinPath(runtimeRoot, "data-products");
    const std::string persistentRoot = joinPath(root, "persistent-data");
    const std::string stagingRoot = joinPath(root, "staging");

    ok = check(!root.empty(), "Expected mkdtemp to create a unique root for storage health contract test") && ok;
    ok = check(ensureDir(runtimeRoot, "Failed to create runtime root for storage health contract test"),
               "Expected runtime root directory creation to succeed") &&
         ok;
    ok = check(ensureDir(dataProductsRoot, "Failed to create data-products root for storage health contract test"),
               "Expected data-products root directory creation to succeed") &&
         ok;
    ok = check(ensureDir(persistentRoot, "Failed to create persistent root for storage health contract test"),
               "Expected persistent root directory creation to succeed") &&
         ok;
    ok = check(ensureDir(stagingRoot, "Failed to create staging root for storage health contract test"),
               "Expected staging root directory creation to succeed") &&
         ok;

    ok = check(writeFile(joinPath(persistentRoot, "data.bin"), "1234"),
               "Expected persistent fixture write to succeed") &&
         ok;
    ok = check(writeFile(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF0123456789ABCDEF"),
               "Expected staging fixture write to succeed") &&
         ok;
    ok = check(writeFile(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789"),
               "Expected data-products fixture write to succeed") &&
         ok;

    if (!ok) {
        cleanupTree(root);
        return false;
    }

    OBC::StorageHealthBridge bridge("storageHealthBridgeContract");
    bridge.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 16U, 9U);

    ok = check(!bridge.scanNowForTest(), "Expected scan to report failure when logs root is missing") && ok;

    OBC::STORAGE::HealthState state = {};
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached state after first configured scan") && ok;
    ok = check(state.hasScan, "Expected hasScan after configured scan") && ok;
    ok = check(state.scanCount == 1U, "Expected first configured scan to set scanCount to 1") && ok;
    ok = check(state.scanErrorCount == 1U, "Expected one scan error for the missing logs root") && ok;
    ok = check(state.warningActive, "Expected warning state after staging size exceeds threshold") && ok;
    ok = check((state.warningMask & static_cast<U8>(1U << 1U)) != 0U,
               "Expected staging warning bit to be raised in warningMask") &&
         ok;
    ok = check((state.degradedMask & static_cast<U8>(1U << 2U)) != 0U,
               "Expected logs degraded bit to be raised when logs root is missing") &&
         ok;
    ok = check(state.dataProducts.quotaStatus == OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA,
               "Expected data-products quota status to report over quota") &&
         ok;

    ok = check(!bridge.scanNowForTest(), "Expected repeated scan to keep failing while logs root stays missing") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached state after repeated configured scan") && ok;
    ok = check(state.scanCount == 2U, "Expected scanCount to accumulate across configured scans") && ok;
    ok = check(state.scanErrorCount == 2U, "Expected scanErrorCount to accumulate across repeated failed scans") &&
         ok;

    cleanupTree(root);
    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testScanRequiresConfiguration() && ok;
    ok = testConfiguredScansAccumulateCountsWarningsAndErrors() && ok;
    return ok ? 0 : 1;
}
