#include "OBC/Components/CommandIngressAuthority/CommandFreshnessStore.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#include "Os/FileSystem.hpp"

namespace {

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

std::string makeTempRoot() {
    char buffer[] = "/tmp/command-freshness-store-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    return path == nullptr ? "/tmp/command-freshness-store-ut-fallback" : std::string(path);
}

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

std::vector<U8> readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    input.seekg(0, std::ios::beg);
    if (size <= 0) {
        return {};
    }
    std::vector<U8> bytes(static_cast<std::size_t>(size), 0U);
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

void corruptFile(const std::string& path) {
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    assert(file.is_open());
    char byte = 0;
    file.read(&byte, 1);
    file.seekp(0, std::ios::beg);
    byte = static_cast<char>(byte ^ 0xFF);
    file.write(&byte, 1);
    file.flush();
}

void cleanupTree(const std::string& baseRoot) {
    (void)Os::FileSystem::removeFile((baseRoot + "/command-ingress/session-floor-a.bin").c_str());
    (void)Os::FileSystem::removeFile((baseRoot + "/command-ingress/session-floor-b.bin").c_str());
    (void)Os::FileSystem::removeDirectory((baseRoot + "/command-ingress").c_str());
    (void)Os::FileSystem::removeDirectory(baseRoot.c_str());
}

OBC::CommandFreshnessStore::SourceEpochKey makeKey(FwIndexType port,
                                                   OBC::AuthorityLinkIdentity identity,
                                                   OBC::AuthorityLinkRole role) {
    OBC::CommandFreshnessStore::SourceEpochKey key = {};
    key.ingressPort = port;
    key.linkIdentity = identity;
    key.linkRole = role;
    return key;
}

}  // namespace

int main() {
    bool ok = true;

    {
        const std::string root = makeTempRoot();
        OBC::CommandFreshnessStore store(joinPath(root, "command-ingress"));
        OBC::CommandFreshnessStore::Snapshot snapshot = {};
        OBC::CommandFreshnessStoreCopy activeCopy = OBC::CommandFreshnessStoreCopy::NONE;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      0U,
                                      snapshot,
                                      activeCopy),
                   "first zero floor persist should succeed") && ok;
        ok = check(activeCopy == OBC::CommandFreshnessStoreCopy::COPY_A, "first zero floor persist should create copy A") &&
             ok;

        OBC::CommandFreshnessStore::Snapshot reloaded = {};
        OBC::CommandFreshnessStoreCopy reloadedCopy = OBC::CommandFreshnessStoreCopy::NONE;
        const OBC::CommandFreshnessStore::LoadStatus status = store.load(reloaded, reloadedCopy);
        U32 floor = 1U;
        ok = check(status == OBC::CommandFreshnessStore::LoadStatus::OK, "zero floor reload should report OK") && ok;
        ok = check(reloadedCopy == OBC::CommandFreshnessStoreCopy::COPY_A, "zero floor reload should retain copy A") &&
             ok;
        ok = check(OBC::CommandFreshnessStore::findFloor(
                       reloaded, makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY), floor),
                   "zero floor reload should retain source epoch entry") &&
             ok;
        ok = check(floor == 0U, "zero floor reload should preserve session floor zero") && ok;
        cleanupTree(root);
    }

    {
        const std::string root = makeTempRoot();
        OBC::CommandFreshnessStore store(joinPath(root, "command-ingress"));
        OBC::CommandFreshnessStore::Snapshot snapshot = {};
        OBC::CommandFreshnessStoreCopy activeCopy = OBC::CommandFreshnessStoreCopy::COPY_A;
        const OBC::CommandFreshnessStore::LoadStatus status = store.load(snapshot, activeCopy);
        ok = check(status == OBC::CommandFreshnessStore::LoadStatus::EMPTY, "empty load should report EMPTY") && ok;
        ok = check(snapshot.count == 0U, "empty load should have zero entries") && ok;
        ok = check(activeCopy == OBC::CommandFreshnessStoreCopy::NONE, "empty load should use no active copy") && ok;
        cleanupTree(root);
    }

    {
        const std::string root = makeTempRoot();
        OBC::CommandFreshnessStore store(joinPath(root, "command-ingress"));
        OBC::CommandFreshnessStore::Snapshot snapshot = {};
        OBC::CommandFreshnessStoreCopy activeCopy = OBC::CommandFreshnessStoreCopy::NONE;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      42U,
                                      snapshot,
                                      activeCopy),
                   "first persist should succeed") && ok;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      43U,
                                      snapshot,
                                      activeCopy),
                   "second persist should succeed") && ok;
        ok = check(activeCopy == OBC::CommandFreshnessStoreCopy::COPY_B, "second persist should activate copy B") && ok;
        U32 floor = 0U;
        ok = check(OBC::CommandFreshnessStore::findFloor(
                       snapshot, makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY), floor),
                   "findFloor should locate persisted source epoch") && ok;
        ok = check(floor == 43U, "snapshot should retain newest floor") && ok;

        const std::vector<U8> bytes = readFile(store.getCopyPath(activeCopy));
        ok = check(bytes.size() == 348U, "snapshot copy should keep fixed byte size") && ok;
        ok = check(bytes.size() >= 48U, "snapshot copy should include header plus first entry") && ok;
        if (bytes.size() >= 48U) {
            ok = check(bytes[0] == 'C' && bytes[1] == 'S' && bytes[2] == 'F' && bytes[3] == '1',
                       "snapshot magic should be stored as little-endian CSF1") &&
                 ok;
            ok = check(bytes[4] == 0x01U && bytes[5] == 0x00U, "snapshot version should be stored little-endian") && ok;
            ok = check(bytes[8] == 0x02U && bytes[9] == 0x00U && bytes[10] == 0x00U && bytes[11] == 0x00U,
                       "generation should be stored little-endian") &&
                 ok;
            ok = check(bytes[12] == 0x10U && bytes[13] == 0x00U && bytes[14] == 0x00U && bytes[15] == 0x00U,
                       "capacity should be stored little-endian") &&
                 ok;
            ok = check(bytes[16] == 0x01U && bytes[17] == 0x00U && bytes[18] == 0x00U && bytes[19] == 0x00U,
                       "count should be stored little-endian") &&
                 ok;
            ok = check(bytes[20] == 0x14U && bytes[21] == 0x00U && bytes[22] == 0x00U && bytes[23] == 0x00U,
                       "record width should be stored little-endian") &&
                 ok;
            ok = check(bytes[28] == 0x01U && bytes[29] == 0x00U && bytes[30] == 0x00U && bytes[31] == 0x00U,
                       "entry valid flag should be stored little-endian") &&
                 ok;
            ok = check(bytes[32] == 0x00U && bytes[33] == 0x00U && bytes[34] == 0x00U && bytes[35] == 0x00U,
                       "ingress port should be stored little-endian") &&
                 ok;
            ok = check(bytes[36] == 0x01U && bytes[37] == 0x00U && bytes[38] == 0x00U && bytes[39] == 0x00U,
                       "identity should be stored little-endian") &&
                 ok;
            ok = check(bytes[40] == 0x01U && bytes[41] == 0x00U && bytes[42] == 0x00U && bytes[43] == 0x00U,
                       "role should be stored little-endian") &&
                 ok;
            ok = check(bytes[44] == 0x2BU && bytes[45] == 0x00U && bytes[46] == 0x00U && bytes[47] == 0x00U,
                       "session floor should be stored little-endian") &&
                 ok;
        }
        cleanupTree(root);
    }

    {
        const std::string root = makeTempRoot();
        OBC::CommandFreshnessStore store(joinPath(root, "command-ingress"));
        OBC::CommandFreshnessStore::Snapshot snapshot = {};
        OBC::CommandFreshnessStoreCopy activeCopy = OBC::CommandFreshnessStoreCopy::NONE;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      11U,
                                      snapshot,
                                      activeCopy),
                   "fallback first persist should succeed") && ok;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      12U,
                                      snapshot,
                                      activeCopy),
                   "fallback second persist should succeed") && ok;
        corruptFile(store.getCopyPath(OBC::CommandFreshnessStoreCopy::COPY_B));

        OBC::CommandFreshnessStore::Snapshot loaded = {};
        OBC::CommandFreshnessStoreCopy loadedCopy = OBC::CommandFreshnessStoreCopy::NONE;
        const OBC::CommandFreshnessStore::LoadStatus status = store.load(loaded, loadedCopy);
        U32 floor = 0U;
        ok = check(status == OBC::CommandFreshnessStore::LoadStatus::OK, "fallback load should still report OK") && ok;
        ok = check(loadedCopy == OBC::CommandFreshnessStoreCopy::COPY_A, "fallback should use older valid copy") && ok;
        ok = check(OBC::CommandFreshnessStore::findFloor(
                       loaded, makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY), floor),
                   "fallback snapshot should retain source epoch entry") && ok;
        ok = check(floor == 11U, "fallback should retain older persisted floor") && ok;
        cleanupTree(root);
    }

    {
        const std::string root = makeTempRoot();
        OBC::CommandFreshnessStore store(joinPath(root, "command-ingress"));
        OBC::CommandFreshnessStore::Snapshot snapshot = {};
        OBC::CommandFreshnessStoreCopy activeCopy = OBC::CommandFreshnessStoreCopy::NONE;
        ok = check(store.persistFloor(makeKey(0U, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY),
                                      21U,
                                      snapshot,
                                      activeCopy),
                   "both-invalid first persist should succeed") && ok;
        ok = check(store.persistFloor(makeKey(1U, OBC::AuthorityLinkIdentity::UHF, OBC::AuthorityLinkRole::BACKUP),
                                      22U,
                                      snapshot,
                                      activeCopy),
                   "both-invalid second persist should succeed") && ok;
        corruptFile(store.getCopyPath(OBC::CommandFreshnessStoreCopy::COPY_A));
        corruptFile(store.getCopyPath(OBC::CommandFreshnessStoreCopy::COPY_B));

        OBC::CommandFreshnessStore::Snapshot loaded = {};
        OBC::CommandFreshnessStoreCopy loadedCopy = OBC::CommandFreshnessStoreCopy::NONE;
        const OBC::CommandFreshnessStore::LoadStatus status = store.load(loaded, loadedCopy);
        ok = check(status == OBC::CommandFreshnessStore::LoadStatus::INVALID, "both-invalid load should fail closed") &&
             ok;
        ok = check(loadedCopy == OBC::CommandFreshnessStoreCopy::NONE, "invalid load should not select active copy") &&
             ok;
        cleanupTree(root);
    }

    return ok ? 0 : 1;
}
