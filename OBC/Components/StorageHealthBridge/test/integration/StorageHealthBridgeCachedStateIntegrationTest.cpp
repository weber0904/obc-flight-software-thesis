#include <cstdio>
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
    char buffer[] = "/tmp/storage-bridge-it-XXXXXX";
    const char* path = ::mkdtemp(buffer);
    return path == nullptr ? "/tmp/storage-bridge-it-fallback" : std::string(path);
}

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

void writeFile(const std::string& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary);
    output << contents;
}

void cleanupTree(const std::string& root) {
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

bool testBridgePublishesCachedStorageHealthState() {
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

    writeFile(joinPath(logsRoot, "runtime.log"), "zzzz");
    writeFile(joinPath(dataProductsRoot, "Dp_1.fdp"), "0123456789");
    writeFile(joinPath(persistentRoot, "data.bin"), "123456");
    writeFile(joinPath(stagingRoot, "image.bin"), "0123456789ABCDEF");

    OBC::StorageHealthBridge bridge("storageHealthBridgeTest");
    bridge.configureRuntime(runtimeRoot, persistentRoot, stagingRoot, 8U, 9U);

    ok = check(bridge.scanNowForTest(), "Expected storage scan to succeed") && ok;

    OBC::STORAGE::HealthState state = {};
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached storage health state") && ok;
    ok = check(state.hasScan, "Expected hasScan in cached storage state") && ok;
    ok = check(state.staging.totalBytes == 16U, "Expected staging byte count in cached storage state") && ok;
    ok = check(state.dataProducts.exists, "Expected data-products root in cached storage state") && ok;
    ok = check(state.dataProducts.totalBytes == 10U, "Expected data-products byte count in cached storage state") && ok;
    ok = check(state.dataProducts.quotaStatus == OBC::STORAGE::STORAGE_QUOTA_OVER_QUOTA,
               "Expected data-products over-quota status in cached storage state") &&
         ok;
    ok = check(state.warningActive, "Expected warningActive in cached storage state") && ok;
    ok = check(state.logs.errorCode == 0U, "Expected no logs error code for successful scan") && ok;

    cleanupTree(root);
    return ok;
}

}  // namespace

int main() {
    return testBridgePublishesCachedStorageHealthState() ? 0 : 1;
}
