#include "OBC/Components/PersistentFaultManager/PersistentFaultStore.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "Os/FileSystem.hpp"

namespace {

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

std::string makeTempRoot() {
    char buffer[] = "/tmp/persistent-fault-store-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    return path == nullptr ? "/tmp/persistent-fault-store-ut-fallback" : std::string(path);
}

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
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
    (void)Os::FileSystem::removeFile((baseRoot + "/recovery/fault-ring-a.bin").c_str());
    (void)Os::FileSystem::removeFile((baseRoot + "/recovery/fault-ring-b.bin").c_str());
    (void)Os::FileSystem::removeDirectory((baseRoot + "/recovery").c_str());
    (void)Os::FileSystem::removeDirectory(baseRoot.c_str());
}

void writeOversizedFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    assert(file.is_open());
    std::vector<char> bytes(sizeof(U32) * 1024U, 0);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.flush();
}

OBC::PersistentFaultRecord makeRecord(OBC::PersistentFaultRecordKind kind, U32 bootCount, U32 detail) {
    OBC::PersistentFaultRecord record = {};
    record.kind = kind;
    record.source = OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE;
    record.level = OBC::RecoveryLevel::R6_OBC_REBOOT;
    record.action = OBC::RecoveryAction::OBC_REBOOT;
    record.resetCause = OBC::ResetCause::RECOVERY_COMM_FDIR;
    record.timestampSec = 1000U + bootCount;
    record.uptimeSec = 20U + bootCount;
    record.bootCount = bootCount;
    record.consecutiveResetCount = bootCount / 2U;
    record.detail = detail;
    record.flags = (bootCount % 2U) == 0U ? OBC::PersistentFaultFlagBootSafeFallback : 0U;
    return record;
}

}  // namespace

int main() {
    bool ok = true;

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(4U, status, records), "empty store should read successfully") && ok;
        ok = check(status.totalRecords == 0U, "empty store should report zero records") && ok;
        ok = check(status.activeCopy == OBC::PersistentFaultStoreCopy::NONE, "empty store should use no active copy") && ok;
        ok = check(records.empty(), "empty store should return no records") && ok;
        cleanupTree(baseRoot);
    }

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::BOOT_OBSERVED, 1U, 11U)),
                   "first append should succeed") && ok;
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::INCIDENT_OPENED, 2U, 22U)),
                   "second append should succeed") && ok;

        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(2U, status, records), "two-record read should succeed") && ok;
        ok = check(status.totalRecords == 2U, "status should report two total records") && ok;
        ok = check(status.returnedRecords == 2U, "status should return both records") && ok;
        ok = check(status.activeCopy == OBC::PersistentFaultStoreCopy::COPY_B, "second append should activate copy B") && ok;
        ok = check(records.size() == 2U, "history should contain two records") && ok;
        ok = check(records[0].kind == OBC::PersistentFaultRecordKind::INCIDENT_OPENED,
                   "latest-first read should return the second record first") && ok;
        ok = check(records[1].kind == OBC::PersistentFaultRecordKind::BOOT_OBSERVED,
                   "latest-first read should return the first record second") && ok;
        cleanupTree(baseRoot);
    }

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        for (U32 i = 0; i < OBC::PersistentFaultStoreCapacity + 2U; i++) {
            ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::ACTION_REQUESTED, i, i + 100U)),
                       "wraparound append should succeed") && ok;
        }

        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(OBC::PersistentFaultStoreCapacity, status, records),
                   "wraparound read should succeed") && ok;
        ok = check(status.totalRecords == OBC::PersistentFaultStoreCapacity,
                   "wraparound should clamp stored record count to capacity") && ok;
        ok = check(records.size() == OBC::PersistentFaultStoreCapacity,
                   "wraparound read should return capacity records") && ok;
        ok = check(records.front().bootCount == OBC::PersistentFaultStoreCapacity + 1U,
                   "latest record should be the newest appended record") && ok;
        ok = check(records.back().bootCount == 2U,
                   "oldest retained record should reflect wraparound eviction") && ok;
        cleanupTree(baseRoot);
    }

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::BOOT_OBSERVED, 1U, 101U)),
                   "fallback test first append should succeed") && ok;
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::INCIDENT_OPENED, 2U, 202U)),
                   "fallback test second append should succeed") && ok;
        corruptFile(store.getCopyPath(OBC::PersistentFaultStoreCopy::COPY_B));

        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(4U, status, records), "corrupt newer copy should still read successfully") && ok;
        ok = check(status.totalRecords == 1U, "fallback should reuse the older single-record snapshot") && ok;
        ok = check(status.activeCopy == OBC::PersistentFaultStoreCopy::COPY_A,
                   "fallback should promote copy A as the active snapshot") && ok;
        ok = check(records.size() == 1U && records[0].detail == 101U,
                   "fallback should return the older valid record") && ok;
        cleanupTree(baseRoot);
    }

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::BOOT_OBSERVED, 1U, 301U)),
                   "both-invalid first append should succeed") && ok;
        ok = check(store.append(makeRecord(OBC::PersistentFaultRecordKind::INCIDENT_OPENED, 2U, 302U)),
                   "both-invalid second append should succeed") && ok;
        corruptFile(store.getCopyPath(OBC::PersistentFaultStoreCopy::COPY_A));
        corruptFile(store.getCopyPath(OBC::PersistentFaultStoreCopy::COPY_B));

        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(4U, status, records), "both-invalid read should degrade to empty store") && ok;
        ok = check(status.totalRecords == 0U, "both-invalid store should report zero records") && ok;
        ok = check(status.activeCopy == OBC::PersistentFaultStoreCopy::NONE,
                   "both-invalid store should report no active copy") && ok;
        ok = check(records.empty(), "both-invalid store should return no records") && ok;
        cleanupTree(baseRoot);
    }

    {
        const std::string baseRoot = makeTempRoot();
        const std::string storeRoot = joinPath(baseRoot, "recovery");
        OBC::PersistentFaultStore store(storeRoot);
        ok = check(store.ensureStorage(), "oversized-file test should create storage tree") && ok;
        writeOversizedFile(store.getCopyPath(OBC::PersistentFaultStoreCopy::COPY_A));

        OBC::PersistentFaultHistoryStatus status = {};
        std::vector<OBC::PersistentFaultRecord> records;
        ok = check(store.readLatest(4U, status, records), "oversized copy should degrade to empty store") && ok;
        ok = check(status.totalRecords == 0U, "oversized copy should be rejected as empty store") && ok;
        ok = check(status.activeCopy == OBC::PersistentFaultStoreCopy::NONE,
                   "oversized copy should not become active") && ok;
        cleanupTree(baseRoot);
    }

    return ok ? 0 : 1;
}
