#include "OBC/Components/BootManager/BootMetadataStore.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace {

std::string makeTempRoot() {
    char buffer[] = "/tmp/boot-metadata-store-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    return path == nullptr ? "/tmp/boot-metadata-store-ut-fallback" : std::string(path);
}

void writeFile(const std::string& path, const std::string& contents) {
    Os::File file;
    assert(file.open(path.c_str(), Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE) == Os::File::Status::OP_OK);
    FwSizeType size = static_cast<FwSizeType>(contents.size());
    assert(file.write(reinterpret_cast<const U8*>(contents.data()), size, Os::File::WaitType::WAIT) ==
           Os::File::Status::OP_OK);
    file.close();
}

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    const std::string root = makeTempRoot();
    const std::string bootRoot = root + "/boot";
    (void)Os::FileSystem::createDirectory(root.c_str());
    (void)Os::FileSystem::createDirectory(bootRoot.c_str());

    bool ok = true;

    {
        writeFile(bootRoot + "/metadata-v1.txt",
                  "schema_version=2\n"
                  "active_slot=SLOT_A\n"
                  "pending_slot=NONE\n"
                  "last_known_good_slot=SLOT_A\n"
                  "confirmed=1\n"
                  "expected_digest=\n"
                  "expected_size=0\n"
                  "last_boot_attempt_time=0\n"
                  "last_error_code=0\n"
                  "stage_verified=0\n"
                  "staged_path=\n"
                  "manifest_path=\n"
                  "trust_status=0\n"
                  "trust_reject_reason=0\n"
                  "image_id=\n"
                  "staged_software_version=0\n"
                  "active_software_version=1\n"
                  "pending_software_version=0\n"
                  "last_known_good_software_version=1\n"
                  "last_accepted_version=1\n"
                  "signer_id=repo-dev-boot-signer\n"
                  "key_slot=1\n"
                  "signature_algorithm=hmac-sha256\n");

        OBC::BootMetadataStore store(bootRoot);
        OBC::BootMetadata metadata = {};
        ok = check(store.load(metadata) == OBC::BootMetadataStore::LoadStatus::OK, "legacy schema load should succeed") && ok;
        ok = check(metadata.resetCause == OBC::ResetCause::UNKNOWN, "legacy schema should default reset cause") && ok;
        ok = check(metadata.bootCount == 0U, "legacy schema should default boot count") && ok;
        ok = check(metadata.consecutiveResetCount == 0U, "legacy schema should default consecutive reset count") && ok;
        ok = check(metadata.lastRecoverySource == OBC::RecoveryIncidentSource::NONE,
                   "legacy schema should default last recovery source") && ok;
        ok = check(metadata.lastRecoveryLevel == OBC::RecoveryLevel::R0_RECORD_ONLY,
                   "legacy schema should default last recovery level") && ok;
        ok = check(!metadata.bootSafeFallbackRequired, "legacy schema should default safe fallback flag") && ok;
    }

    {
        OBC::BootMetadataStore store(bootRoot);
        OBC::BootMetadata saved = {};
        saved.schemaVersion = 4U;
        saved.activeSlot = OBC::BootSlot::SLOT_B;
        saved.lastKnownGoodSlot = OBC::BootSlot::SLOT_B;
        saved.confirmed = true;
        saved.activeSoftwareVersion = 7U;
        saved.lastAcceptedVersion = 7U;
        saved.resetCause = OBC::ResetCause::RECOVERY_EPS_TIMEOUT;
        saved.bootCount = 4U;
        saved.consecutiveResetCount = 2U;
        saved.lastRecoverySource = OBC::RecoveryIncidentSource::EPS_TIMEOUT;
        saved.lastRecoveryLevel = OBC::RecoveryLevel::R6_OBC_REBOOT;
        saved.bootSafeFallbackRequired = true;
        saved.recoveryResetPending = true;

        ok = check(store.save(saved), "extended schema save should succeed") && ok;

        OBC::BootMetadata loaded = {};
        ok = check(store.load(loaded) == OBC::BootMetadataStore::LoadStatus::OK, "extended schema reload should succeed") && ok;
        ok = check(loaded.activeSlot == OBC::BootSlot::SLOT_B, "roundtrip active slot should persist") && ok;
        ok = check(loaded.resetCause == OBC::ResetCause::RECOVERY_EPS_TIMEOUT, "roundtrip reset cause should persist") && ok;
        ok = check(loaded.bootCount == 4U, "roundtrip boot count should persist") && ok;
        ok = check(loaded.consecutiveResetCount == 2U, "roundtrip consecutive reset count should persist") && ok;
        ok = check(loaded.lastRecoverySource == OBC::RecoveryIncidentSource::EPS_TIMEOUT,
                   "roundtrip recovery source should persist") && ok;
        ok = check(loaded.lastRecoveryLevel == OBC::RecoveryLevel::R6_OBC_REBOOT,
                   "roundtrip recovery level should persist") && ok;
        ok = check(loaded.bootSafeFallbackRequired, "roundtrip safe fallback flag should persist") && ok;
        ok = check(loaded.recoveryResetPending, "roundtrip recovery pending flag should persist") && ok;
    }

    (void)Os::FileSystem::removeFile((bootRoot + "/metadata-v1.txt").c_str());
    (void)Os::FileSystem::removeDirectory(bootRoot.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
    return ok ? 0 : 1;
}
