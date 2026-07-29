#include "OBC/Components/BootManager/BootManifestVerifier.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

constexpr FwSizeType MAX_MANIFEST_BYTES = 4096U;

bool readFile(const std::string& path, std::string& contents) {
    FwSizeType fileSize = 0U;
    if (Os::FileSystem::getFileSize(path.c_str(), fileSize) != Os::FileSystem::Status::OP_OK) {
        return false;
    }
    if (fileSize == 0U || fileSize > MAX_MANIFEST_BYTES) {
        return false;
    }

    Os::File file;
    if (file.open(path.c_str(), Os::File::OPEN_READ) != Os::File::Status::OP_OK) {
        return false;
    }

    std::vector<U8> buffer(fileSize + 1U, 0U);
    FwSizeType readSize = fileSize;
    const Os::File::Status status = file.read(buffer.data(), readSize, Os::File::WaitType::WAIT);
    file.close();

    if (status != Os::File::Status::OP_OK || readSize != fileSize) {
        return false;
    }

    contents.assign(reinterpret_cast<const char*>(buffer.data()), readSize);
    return true;
}

std::string trim(const std::string& value) {
    std::string::size_type start = 0U;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        start++;
    }
    std::string::size_type end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1U])) != 0) {
        end--;
    }
    return value.substr(start, end - start);
}

bool parseUnsigned(const std::string& value, U32& output) {
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) {
        return false;
    }

    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed > std::numeric_limits<U32>::max()) {
            return false;
        }
        output = static_cast<U32>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool isLowerHex(const std::string& value, std::size_t expectedLength) {
    if (value.size() != expectedLength) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
    });
}

bool parseHexBytes(const std::string& hex, std::string& bytes) {
    bytes.clear();
    if ((hex.size() % 2U) != 0U || !std::all_of(hex.begin(), hex.end(), [](unsigned char ch) {
            return std::isxdigit(ch) != 0;
        })) {
        return false;
    }

    auto decodeNibble = [](char ch) -> U8 {
        if (ch >= '0' && ch <= '9') {
            return static_cast<U8>(ch - '0');
        }
        return static_cast<U8>(10 + static_cast<char>(std::tolower(static_cast<unsigned char>(ch))) - 'a');
    };

    bytes.reserve(hex.size() / 2U);
    for (std::size_t i = 0U; i < hex.size(); i += 2U) {
        bytes.push_back(static_cast<char>((decodeNibble(hex[i]) << 4U) | decodeNibble(hex[i + 1U])));
    }
    return true;
}

std::string bytesToHex(const U8* data, FwSizeType size) {
    static const char* const HEX = "0123456789abcdef";
    std::string output;
    output.reserve(size * 2U);
    for (FwSizeType i = 0U; i < size; i++) {
        const U8 value = data[i];
        output.push_back(HEX[(value >> 4U) & 0x0FU]);
        output.push_back(HEX[value & 0x0FU]);
    }
    return output;
}

bool parseSlot(const std::string& value, OBC::BootSlot& slot) {
    if (value == "SLOT_A") {
        slot = OBC::BootSlot::SLOT_A;
        return true;
    }
    if (value == "SLOT_B") {
        slot = OBC::BootSlot::SLOT_B;
        return true;
    }
    return false;
}

std::string basenameOf(const std::string& path) {
    const std::string::size_type pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1U);
}

bool parseManifest(const std::string& contents, BootManifest& manifest) {
    std::map<std::string, std::string> entries;
    std::istringstream stream(contents);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        const std::string::size_type delimiter = trimmed.find('=');
        if (delimiter == std::string::npos || delimiter == 0U) {
            return false;
        }

        const std::string key = trim(trimmed.substr(0U, delimiter));
        const std::string value = trim(trimmed.substr(delimiter + 1U));
        if (key.empty() || entries.find(key) != entries.end()) {
            return false;
        }
        entries.emplace(key, value);
    }

    static const char* const REQUIRED[] = {"schema",
                                           "image_path",
                                           "image_size",
                                           "image_digest_sha256",
                                           "target_slot",
                                           "image_id",
                                           "software_version",
                                           "signer_id",
                                           "key_slot",
                                           "signature_algorithm",
                                           "signature"};
    for (const char* const key : REQUIRED) {
        if (entries.find(key) == entries.end() || entries[key].empty()) {
            return false;
        }
    }

    if (entries["schema"] != "boot_manifest_v1") {
        return false;
    }
    if (!parseUnsigned(entries["image_size"], manifest.imageSize) || manifest.imageSize == 0U) {
        return false;
    }
    if (!isLowerHex(entries["image_digest_sha256"], 64U)) {
        return false;
    }
    if (!parseSlot(entries["target_slot"], manifest.targetSlot)) {
        return false;
    }
    if (!parseUnsigned(entries["software_version"], manifest.softwareVersion) || manifest.softwareVersion == 0U) {
        return false;
    }
    if (!parseUnsigned(entries["key_slot"], manifest.keySlot) || manifest.keySlot == 0U) {
        return false;
    }
    if (entries["signature_algorithm"] != "hmac-sha256") {
        return false;
    }
    if (!isLowerHex(entries["signature"], 64U)) {
        return false;
    }

    manifest.imagePath = entries["image_path"];
    manifest.imageDigest = entries["image_digest_sha256"];
    manifest.imageId = entries["image_id"];
    manifest.signerId = entries["signer_id"];
    manifest.signatureAlgorithm = entries["signature_algorithm"];
    manifest.signature = entries["signature"];
    return true;
}

std::string canonicalManifestPayload(const BootManifest& manifest) {
    std::ostringstream stream;
    stream << "schema=boot_manifest_v1\n";
    stream << "image_path=" << manifest.imagePath << "\n";
    stream << "image_size=" << manifest.imageSize << "\n";
    stream << "image_digest_sha256=" << manifest.imageDigest << "\n";
    stream << "target_slot=" << (manifest.targetSlot == OBC::BootSlot::SLOT_A ? "SLOT_A" : "SLOT_B") << "\n";
    stream << "image_id=" << manifest.imageId << "\n";
    stream << "software_version=" << manifest.softwareVersion << "\n";
    stream << "signer_id=" << manifest.signerId << "\n";
    stream << "key_slot=" << manifest.keySlot << "\n";
    stream << "signature_algorithm=" << manifest.signatureAlgorithm << "\n";
    return stream.str();
}

bool validateConfig(const BootManifest& manifest, const BootTrustConfig& config, std::string& keyBytes) {
    if (config.algorithm != "hmac-sha256" || manifest.signatureAlgorithm != "hmac-sha256") {
        return false;
    }
    if (config.trustedSignerId.empty() || config.trustedSignerId != manifest.signerId ||
        config.trustedKeySlot == 0U || config.trustedKeySlot != manifest.keySlot) {
        return false;
    }
    return parseHexBytes(config.trustedKeyHex, keyBytes) && !keyBytes.empty();
}

}  // namespace

bool computeBootManifestSignatureHex(const BootManifest& manifest,
                                     const BootTrustConfig& config,
                                     std::string& signatureHex) {
    std::string keyBytes;
    if (!validateConfig(manifest, config, keyBytes)) {
        return false;
    }

    const std::string payload = canonicalManifestPayload(manifest);
    U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
    if (!hmacSha256(reinterpret_cast<const U8*>(keyBytes.data()),
                    static_cast<FwSizeType>(keyBytes.size()),
                    reinterpret_cast<const U8*>(payload.data()),
                    static_cast<FwSizeType>(payload.size()),
                    digest)) {
        return false;
    }

    signatureHex = bytesToHex(digest, COMMAND_AUTH_SHA256_DIGEST_SIZE);
    return true;
}

BootManifestDecision verifyBootManifestFile(const std::string& manifestPath,
                                            const std::string& requestedImagePath,
                                            U32 actualImageSize,
                                            const std::string& actualImageDigest,
                                            OBC::BootSlot expectedSlot,
                                            U32 lastAcceptedVersion,
                                            const BootTrustConfig& config) {
    BootManifestDecision decision = {};

    std::string contents;
    if (!readFile(manifestPath, contents)) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_MISSING;
        return decision;
    }

    if (!parseManifest(contents, decision.manifest)) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_MALFORMED;
        return decision;
    }

    if (decision.manifest.imagePath != requestedImagePath &&
        decision.manifest.imagePath != basenameOf(requestedImagePath)) {
        decision.rejectReason = BOOT_TRUST_REJECT_IMAGE_PATH_MISMATCH;
        return decision;
    }

    if (decision.manifest.imageSize != actualImageSize) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_SIZE_MISMATCH;
        return decision;
    }

    if (decision.manifest.imageDigest != actualImageDigest) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH;
        return decision;
    }

    if (decision.manifest.targetSlot != expectedSlot) {
        decision.rejectReason = BOOT_TRUST_REJECT_TARGET_SLOT_MISMATCH;
        return decision;
    }

    if (decision.manifest.softwareVersion <= lastAcceptedVersion) {
        decision.rejectReason = BOOT_TRUST_REJECT_VERSION_DOWNGRADE;
        return decision;
    }

    std::string keyBytes;
    if (!validateConfig(decision.manifest, config, keyBytes)) {
        decision.rejectReason = BOOT_TRUST_REJECT_SIGNER_UNKNOWN;
        return decision;
    }

    std::string expectedSignature;
    if (!computeBootManifestSignatureHex(decision.manifest, config, expectedSignature)) {
        decision.rejectReason = BOOT_TRUST_REJECT_CONFIG_INVALID;
        return decision;
    }

    std::string expectedBytes;
    std::string actualBytes;
    if (!parseHexBytes(expectedSignature, expectedBytes) || !parseHexBytes(decision.manifest.signature, actualBytes) ||
        expectedBytes.size() != actualBytes.size() ||
        !constantTimeEqual(reinterpret_cast<const U8*>(expectedBytes.data()),
                           reinterpret_cast<const U8*>(actualBytes.data()),
                           static_cast<FwSizeType>(expectedBytes.size()))) {
        decision.rejectReason = BOOT_TRUST_REJECT_SIGNATURE_INVALID;
        return decision;
    }

    decision.accepted = true;
    decision.rejectReason = BOOT_TRUST_REJECT_NONE;
    return decision;
}

}  // namespace OBC
