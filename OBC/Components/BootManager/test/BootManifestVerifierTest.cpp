#include "OBC/Components/BootManager/BootManifestVerifier.hpp"

#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace {

std::string makeTempRoot() {
    char buffer[] = "/tmp/boot-manifest-verifier-ut-XXXXXX";
    const char* const path = mkdtemp(buffer);
    EXPECT_NE(path, nullptr);
    return path == nullptr ? "/tmp/boot-manifest-verifier-ut-fallback" : std::string(path);
}

void writeFile(const std::string& path, const std::string& contents) {
    Os::File file;
    ASSERT_EQ(file.open(path.c_str(), Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::Status::OP_OK);
    FwSizeType size = static_cast<FwSizeType>(contents.size());
    ASSERT_EQ(file.write(reinterpret_cast<const U8*>(contents.data()), size, Os::File::WaitType::WAIT),
              Os::File::Status::OP_OK);
    ASSERT_EQ(size, contents.size());
    file.close();
}

OBC::BootManifest makeManifest(const std::string& digest, U32 version = 7U) {
    OBC::BootManifest manifest;
    manifest.imagePath = "image.bin";
    manifest.imageSize = 11U;
    manifest.imageDigest = digest;
    manifest.targetSlot = OBC::BootSlot::SLOT_B;
    manifest.imageId = "obc-test-image";
    manifest.softwareVersion = version;
    manifest.signerId = "repo-dev-boot-signer";
    manifest.keySlot = 1U;
    manifest.signatureAlgorithm = "hmac-sha256";
    return manifest;
}

std::string slotName(OBC::BootSlot slot) {
    return slot == OBC::BootSlot::SLOT_A ? "SLOT_A" : "SLOT_B";
}

std::string manifestText(const OBC::BootManifest& manifest) {
    return "schema=boot_manifest_v1\n"
           "image_path=" +
           manifest.imagePath + "\n" + "image_size=" + std::to_string(manifest.imageSize) + "\n" +
           "image_digest_sha256=" + manifest.imageDigest + "\n" + "target_slot=" + slotName(manifest.targetSlot) +
           "\n" + "image_id=" + manifest.imageId + "\n" +
           "software_version=" + std::to_string(manifest.softwareVersion) + "\n" +
           "signer_id=" + manifest.signerId + "\n" + "key_slot=" + std::to_string(manifest.keySlot) + "\n" +
           "signature_algorithm=" + manifest.signatureAlgorithm + "\n" + "signature=" + manifest.signature + "\n";
}

std::string writeSignedManifest(const std::string& root,
                                const OBC::BootManifest& unsignedManifest,
                                bool validSignature = true) {
    OBC::BootManifest manifest = unsignedManifest;
    OBC::BootTrustConfig config;
    std::string signature;
    EXPECT_TRUE(OBC::computeBootManifestSignatureHex(manifest, config, signature));
    if (!validSignature) {
        signature[0] = signature[0] == '0' ? '1' : '0';
    }
    manifest.signature = signature;

    const std::string path = root + "/image.bin.manifest-v1";
    writeFile(path, manifestText(manifest));
    return path;
}

}  // namespace

TEST(BootManifestVerifier, AcceptsValidSignedManifest) {
    const std::string root = makeTempRoot();
    const std::string digest = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string manifestPath = writeSignedManifest(root, makeManifest(digest));

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath, "image.bin", 11U, digest, OBC::BootSlot::SLOT_B, 6U, {});

    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_NONE);
    EXPECT_EQ(decision.manifest.softwareVersion, 7U);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, RejectsInvalidSignature) {
    const std::string root = makeTempRoot();
    const std::string digest = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const std::string manifestPath = writeSignedManifest(root, makeManifest(digest), false);

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath, "image.bin", 11U, digest, OBC::BootSlot::SLOT_B, 0U, {});

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_SIGNATURE_INVALID);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, RejectsUnknownSigner) {
    const std::string root = makeTempRoot();
    const std::string digest = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    OBC::BootManifest manifest = makeManifest(digest);
    manifest.signerId = "unknown-signer";
    manifest.signature = "0000000000000000000000000000000000000000000000000000000000000000";
    const std::string manifestPath = root + "/image.bin.manifest-v1";
    writeFile(manifestPath, manifestText(manifest));

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath, "image.bin", 11U, digest, OBC::BootSlot::SLOT_B, 0U, {});

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_SIGNER_UNKNOWN);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, RejectsMalformedManifest) {
    const std::string root = makeTempRoot();
    const std::string manifestPath = root + "/image.bin.manifest-v1";
    writeFile(manifestPath, "schema=boot_manifest_v1\nimage_path=image.bin\n");

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath,
                                    "image.bin",
                                    11U,
                                    "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
                                    OBC::BootSlot::SLOT_B,
                                    0U,
                                    {});

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_MANIFEST_MALFORMED);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, RejectsVersionRollback) {
    const std::string root = makeTempRoot();
    const std::string digest = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    const std::string manifestPath = writeSignedManifest(root, makeManifest(digest, 3U));

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath, "image.bin", 11U, digest, OBC::BootSlot::SLOT_B, 3U, {});

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_VERSION_DOWNGRADE);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, RejectsManifestDigestMismatch) {
    const std::string root = makeTempRoot();
    OBC::BootManifest manifest = makeManifest("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    const std::string manifestPath = writeSignedManifest(root, manifest);

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath,
                                    "image.bin",
                                    11U,
                                    "1111111111111111111111111111111111111111111111111111111111111111",
                                    OBC::BootSlot::SLOT_B,
                                    0U,
                                    {});

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}

TEST(BootManifestVerifier, AcceptsUppercaseTrustKeyHex) {
    const std::string root = makeTempRoot();
    const std::string digest = "9999999999999999999999999999999999999999999999999999999999999999";
    OBC::BootManifest manifest = makeManifest(digest);
    OBC::BootTrustConfig config;
    config.trustedKeyHex = "424F4F545F54525553545F434841494E5F56315F4445565F4B4559";

    std::string signature;
    ASSERT_TRUE(OBC::computeBootManifestSignatureHex(manifest, config, signature));
    manifest.signature = signature;
    const std::string manifestPath = root + "/image.bin.manifest-v1";
    writeFile(manifestPath, manifestText(manifest));

    const OBC::BootManifestDecision decision =
        OBC::verifyBootManifestFile(manifestPath, "image.bin", 11U, digest, OBC::BootSlot::SLOT_B, 0U, config);

    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.rejectReason, OBC::BOOT_TRUST_REJECT_NONE);

    (void)Os::FileSystem::removeFile(manifestPath.c_str());
    (void)Os::FileSystem::removeDirectory(root.c_str());
}
