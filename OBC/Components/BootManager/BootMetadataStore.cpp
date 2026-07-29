#include "OBC/Components/BootManager/BootMetadataStore.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#include "OBC/Components/BootManager/BootManifestVerifier.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

constexpr char DEFAULT_ROOT_DIR[] = "runtime/persistent-data/boot";
constexpr FwSizeType HASH_CHUNK_SIZE = 256;
constexpr FwSizeType SHA256_DIGEST_SIZE = 32U;
constexpr FwSizeType SHA256_BLOCK_SIZE = 64U;

class Sha256Accumulator final {
  public:
    Sha256Accumulator() : m_state{0x6A09E667U,
                                  0xBB67AE85U,
                                  0x3C6EF372U,
                                  0xA54FF53AU,
                                  0x510E527FU,
                                  0x9B05688CU,
                                  0x1F83D9ABU,
                                  0x5BE0CD19U},
                          m_totalBytes(0U),
                          m_bufferSize(0U) {}

    void update(const U8* data, FwSizeType size) {
        if (data == nullptr || size == 0U) {
            return;
        }

        this->m_totalBytes += static_cast<U64>(size);

        FwSizeType offset = 0U;
        while (offset < size) {
            const FwSizeType copySize = std::min(static_cast<FwSizeType>(SHA256_BLOCK_SIZE - this->m_bufferSize),
                                                 static_cast<FwSizeType>(size - offset));
            std::copy_n(data + offset, copySize, this->m_buffer.data() + this->m_bufferSize);
            this->m_bufferSize += copySize;
            offset += copySize;

            if (this->m_bufferSize == SHA256_BLOCK_SIZE) {
                this->transform_(this->m_buffer.data());
                this->m_bufferSize = 0U;
            }
        }
    }

    void final(U8 digest[SHA256_DIGEST_SIZE]) {
        this->m_buffer[this->m_bufferSize++] = 0x80U;

        if (this->m_bufferSize > (SHA256_BLOCK_SIZE - 8U)) {
            std::fill(this->m_buffer.begin() + this->m_bufferSize, this->m_buffer.end(), 0U);
            this->transform_(this->m_buffer.data());
            this->m_bufferSize = 0U;
        }

        std::fill(this->m_buffer.begin() + this->m_bufferSize, this->m_buffer.begin() + (SHA256_BLOCK_SIZE - 8U), 0U);

        const U64 totalBits = this->m_totalBytes * 8U;
        for (FwSizeType i = 0U; i < 8U; i++) {
            this->m_buffer[SHA256_BLOCK_SIZE - 1U - i] = static_cast<U8>((totalBits >> (i * 8U)) & 0xFFU);
        }
        this->transform_(this->m_buffer.data());

        for (FwSizeType i = 0U; i < this->m_state.size(); i++) {
            const U32 value = this->m_state[i];
            digest[(i * 4U) + 0U] = static_cast<U8>((value >> 24U) & 0xFFU);
            digest[(i * 4U) + 1U] = static_cast<U8>((value >> 16U) & 0xFFU);
            digest[(i * 4U) + 2U] = static_cast<U8>((value >> 8U) & 0xFFU);
            digest[(i * 4U) + 3U] = static_cast<U8>(value & 0xFFU);
        }
    }

  private:
    static U32 rotateRight_(U32 value, U32 count) {
        return (value >> count) | (value << (32U - count));
    }

    static U32 choose_(U32 x, U32 y, U32 z) {
        return (x & y) ^ (~x & z);
    }

    static U32 majority_(U32 x, U32 y, U32 z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    static U32 bigSigma0_(U32 value) {
        return rotateRight_(value, 2U) ^ rotateRight_(value, 13U) ^ rotateRight_(value, 22U);
    }

    static U32 bigSigma1_(U32 value) {
        return rotateRight_(value, 6U) ^ rotateRight_(value, 11U) ^ rotateRight_(value, 25U);
    }

    static U32 smallSigma0_(U32 value) {
        return rotateRight_(value, 7U) ^ rotateRight_(value, 18U) ^ (value >> 3U);
    }

    static U32 smallSigma1_(U32 value) {
        return rotateRight_(value, 17U) ^ rotateRight_(value, 19U) ^ (value >> 10U);
    }

    static U32 loadBigEndian32_(const U8* data) {
        return (static_cast<U32>(data[0]) << 24U) | (static_cast<U32>(data[1]) << 16U) |
               (static_cast<U32>(data[2]) << 8U) | static_cast<U32>(data[3]);
    }

    void transform_(const U8 block[SHA256_BLOCK_SIZE]) {
        static const std::array<U32, 64> K = {0x428A2F98U,
                                              0x71374491U,
                                              0xB5C0FBCFU,
                                              0xE9B5DBA5U,
                                              0x3956C25BU,
                                              0x59F111F1U,
                                              0x923F82A4U,
                                              0xAB1C5ED5U,
                                              0xD807AA98U,
                                              0x12835B01U,
                                              0x243185BEU,
                                              0x550C7DC3U,
                                              0x72BE5D74U,
                                              0x80DEB1FEU,
                                              0x9BDC06A7U,
                                              0xC19BF174U,
                                              0xE49B69C1U,
                                              0xEFBE4786U,
                                              0x0FC19DC6U,
                                              0x240CA1CCU,
                                              0x2DE92C6FU,
                                              0x4A7484AAU,
                                              0x5CB0A9DCU,
                                              0x76F988DAU,
                                              0x983E5152U,
                                              0xA831C66DU,
                                              0xB00327C8U,
                                              0xBF597FC7U,
                                              0xC6E00BF3U,
                                              0xD5A79147U,
                                              0x06CA6351U,
                                              0x14292967U,
                                              0x27B70A85U,
                                              0x2E1B2138U,
                                              0x4D2C6DFCU,
                                              0x53380D13U,
                                              0x650A7354U,
                                              0x766A0ABBU,
                                              0x81C2C92EU,
                                              0x92722C85U,
                                              0xA2BFE8A1U,
                                              0xA81A664BU,
                                              0xC24B8B70U,
                                              0xC76C51A3U,
                                              0xD192E819U,
                                              0xD6990624U,
                                              0xF40E3585U,
                                              0x106AA070U,
                                              0x19A4C116U,
                                              0x1E376C08U,
                                              0x2748774CU,
                                              0x34B0BCB5U,
                                              0x391C0CB3U,
                                              0x4ED8AA4AU,
                                              0x5B9CCA4FU,
                                              0x682E6FF3U,
                                              0x748F82EEU,
                                              0x78A5636FU,
                                              0x84C87814U,
                                              0x8CC70208U,
                                              0x90BEFFFAU,
                                              0xA4506CEBU,
                                              0xBEF9A3F7U,
                                              0xC67178F2U};

        std::array<U32, 64> schedule = {};
        for (FwSizeType i = 0U; i < 16U; i++) {
            schedule[i] = loadBigEndian32_(block + (i * 4U));
        }
        for (FwSizeType i = 16U; i < schedule.size(); i++) {
            schedule[i] = smallSigma1_(schedule[i - 2U]) + schedule[i - 7U] + smallSigma0_(schedule[i - 15U]) +
                          schedule[i - 16U];
        }

        U32 a = this->m_state[0];
        U32 b = this->m_state[1];
        U32 c = this->m_state[2];
        U32 d = this->m_state[3];
        U32 e = this->m_state[4];
        U32 f = this->m_state[5];
        U32 g = this->m_state[6];
        U32 h = this->m_state[7];

        for (FwSizeType i = 0U; i < schedule.size(); i++) {
            const U32 temp1 = h + bigSigma1_(e) + choose_(e, f, g) + K[i] + schedule[i];
            const U32 temp2 = bigSigma0_(a) + majority_(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        this->m_state[0] += a;
        this->m_state[1] += b;
        this->m_state[2] += c;
        this->m_state[3] += d;
        this->m_state[4] += e;
        this->m_state[5] += f;
        this->m_state[6] += g;
        this->m_state[7] += h;
    }

  private:
    std::array<U32, 8> m_state;
    U64 m_totalBytes;
    std::array<U8, SHA256_BLOCK_SIZE> m_buffer = {};
    FwSizeType m_bufferSize;
};

bool parseUnsignedValue(const std::string& value, U32& output) {
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) {
        return false;
    }
    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed > static_cast<unsigned long>(std::numeric_limits<U32>::max())) {
            return false;
        }
        output = static_cast<U32>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool hasEntry(const std::map<std::string, std::string>& entries, const char* key) {
    const auto it = entries.find(key);
    return it != entries.end() && !it->second.empty();
}

std::string toLowerString(const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lowered;
}

std::string bytesToHex(const U8* data, FwSizeType size) {
    static const char* const HEX = "0123456789abcdef";
    std::string output;
    output.reserve(size * 2U);

    for (FwSizeType i = 0U; i < size; i++) {
        const U8 value = data[i];
        output.push_back(HEX[(value >> 4) & 0x0F]);
        output.push_back(HEX[value & 0x0F]);
    }

    return output;
}

}  // namespace

BootMetadataStore::BootMetadataStore() : BootMetadataStore(DEFAULT_ROOT_DIR) {}

BootMetadataStore::BootMetadataStore(const std::string& rootDir) : m_rootDir(rootDir) {
    this->refreshPaths_();
}

void BootMetadataStore::setRootDir(const std::string& rootDir) {
    this->m_rootDir = rootDir;
    this->refreshPaths_();
}

const std::string& BootMetadataStore::getRootDir() const {
    return this->m_rootDir;
}

const std::string& BootMetadataStore::getMetadataPath() const {
    return this->m_metadataPath;
}

bool BootMetadataStore::ensureStorage() const {
    return this->ensureDirectoryTree_();
}

BootMetadataStore::LoadStatus BootMetadataStore::load(BootMetadata& metadata) const {
    std::string contents;
    if (!readFile_(this->m_metadataPath, contents)) {
        const Os::FileSystem::PathType pathType = Os::FileSystem::getPathType(this->m_metadataPath.c_str());
        if (pathType == Os::FileSystem::PathType::NOT_EXIST) {
            return LoadStatus::NOT_FOUND;
        }
        return LoadStatus::IO_ERROR;
    }

    std::map<std::string, std::string> entries;
    std::istringstream stream(contents);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string trimmed = trim_(line);
        if (trimmed.empty()) {
            continue;
        }

        const std::string::size_type delimiter = trimmed.find('=');
        if (delimiter == std::string::npos || delimiter == 0U) {
            return LoadStatus::INVALID;
        }

        entries.emplace(trim_(trimmed.substr(0U, delimiter)), trim_(trimmed.substr(delimiter + 1U)));
    }

    BootMetadata parsed = {};
    if (!hasEntry(entries, "active_slot") || !hasEntry(entries, "pending_slot") ||
        !hasEntry(entries, "last_known_good_slot") || !hasEntry(entries, "confirmed") ||
        !hasEntry(entries, "expected_size") || !hasEntry(entries, "last_boot_attempt_time") ||
        !hasEntry(entries, "last_error_code") || !hasEntry(entries, "stage_verified") ||
        !parseSlot_(entries["active_slot"], parsed.activeSlot) ||
        !parseSlot_(entries["pending_slot"], parsed.pendingSlot) ||
        !parseSlot_(entries["last_known_good_slot"], parsed.lastKnownGoodSlot) ||
        !parseBool_(entries["confirmed"], parsed.confirmed) ||
        !parseUnsignedValue(entries["expected_size"], parsed.expectedSize) ||
        !parseUnsignedValue(entries["last_boot_attempt_time"], parsed.lastBootAttemptTime) ||
        !parseUnsignedValue(entries["last_error_code"], parsed.lastErrorCode) ||
        !parseBool_(entries["stage_verified"], parsed.stageVerified)) {
        return LoadStatus::INVALID;
    }

    parsed.expectedDigest = toLowerString(entries["expected_digest"]);
    parsed.stagedPath = entries["staged_path"];

    const bool hasTrustFields = hasEntry(entries, "schema_version") && hasEntry(entries, "trust_status") &&
                                hasEntry(entries, "trust_reject_reason") &&
                                hasEntry(entries, "last_accepted_version");
    if (!hasTrustFields) {
        parsed.schemaVersion = 2U;
        parsed.trustStatus = parsed.confirmed ? BOOT_TRUST_STATUS_CONFIRMED : BOOT_TRUST_STATUS_UNVERIFIED;
        parsed.trustRejectReason = BOOT_TRUST_REJECT_NONE;
        parsed.lastAcceptedVersion = 0U;
        parsed.activeSoftwareVersion = 0U;
        parsed.lastKnownGoodSoftwareVersion = 0U;
        parsed.stageVerified = false;
        parsed.stagedPath.clear();
        metadata = parsed;
        return LoadStatus::OK;
    }

    if (!parseUnsignedValue(entries["schema_version"], parsed.schemaVersion) ||
        !parseUnsignedValue(entries["trust_status"], parsed.trustStatus) ||
        !parseUnsignedValue(entries["trust_reject_reason"], parsed.trustRejectReason) ||
        !parseUnsignedValue(entries["staged_software_version"], parsed.stagedSoftwareVersion) ||
        !parseUnsignedValue(entries["active_software_version"], parsed.activeSoftwareVersion) ||
        !parseUnsignedValue(entries["pending_software_version"], parsed.pendingSoftwareVersion) ||
        !parseUnsignedValue(entries["last_known_good_software_version"], parsed.lastKnownGoodSoftwareVersion) ||
        !parseUnsignedValue(entries["last_accepted_version"], parsed.lastAcceptedVersion) ||
        !parseUnsignedValue(entries["key_slot"], parsed.keySlot)) {
        return LoadStatus::INVALID;
    }

    parsed.manifestPath = entries["manifest_path"];
    parsed.manifestImagePath = entries["manifest_image_path"];
    parsed.imageId = entries["image_id"];
    parsed.signerId = entries["signer_id"];
    parsed.signatureAlgorithm = entries["signature_algorithm"];

    U32 enumValue = 0U;
    if (hasEntry(entries, "reset_cause")) {
        if (!parseUnsignedValue(entries["reset_cause"], enumValue)) {
            return LoadStatus::INVALID;
        }
        parsed.resetCause = OBC::ResetCause(static_cast<OBC::ResetCause::T>(enumValue));
    }
    if (hasEntry(entries, "boot_count")) {
        if (!parseUnsignedValue(entries["boot_count"], parsed.bootCount)) {
            return LoadStatus::INVALID;
        }
    }
    if (hasEntry(entries, "consecutive_reset_count")) {
        if (!parseUnsignedValue(entries["consecutive_reset_count"], parsed.consecutiveResetCount)) {
            return LoadStatus::INVALID;
        }
    }
    if (hasEntry(entries, "last_recovery_source")) {
        if (!parseUnsignedValue(entries["last_recovery_source"], enumValue)) {
            return LoadStatus::INVALID;
        }
        parsed.lastRecoverySource =
            OBC::RecoveryIncidentSource(static_cast<OBC::RecoveryIncidentSource::T>(enumValue));
    }
    if (hasEntry(entries, "last_recovery_level")) {
        if (!parseUnsignedValue(entries["last_recovery_level"], enumValue)) {
            return LoadStatus::INVALID;
        }
        parsed.lastRecoveryLevel = OBC::RecoveryLevel(static_cast<OBC::RecoveryLevel::T>(enumValue));
    }
    if (hasEntry(entries, "boot_safe_fallback_required") &&
        !parseBool_(entries["boot_safe_fallback_required"], parsed.bootSafeFallbackRequired)) {
        return LoadStatus::INVALID;
    }
    if (hasEntry(entries, "recovery_reset_pending") &&
        !parseBool_(entries["recovery_reset_pending"], parsed.recoveryResetPending)) {
        return LoadStatus::INVALID;
    }

    metadata = parsed;
    return LoadStatus::OK;
}

bool BootMetadataStore::save(const BootMetadata& metadata) const {
    if (!this->ensureStorage()) {
        return false;
    }

    std::ostringstream stream;
    stream << "schema_version=" << metadata.schemaVersion << "\n";
    stream << "active_slot=" << slotToString_(metadata.activeSlot) << "\n";
    stream << "pending_slot=" << slotToString_(metadata.pendingSlot) << "\n";
    stream << "last_known_good_slot=" << slotToString_(metadata.lastKnownGoodSlot) << "\n";
    stream << "confirmed=" << (metadata.confirmed ? "1" : "0") << "\n";
    stream << "expected_digest=" << metadata.expectedDigest << "\n";
    stream << "expected_size=" << metadata.expectedSize << "\n";
    stream << "last_boot_attempt_time=" << metadata.lastBootAttemptTime << "\n";
    stream << "last_error_code=" << metadata.lastErrorCode << "\n";
    stream << "stage_verified=" << (metadata.stageVerified ? "1" : "0") << "\n";
    stream << "staged_path=" << metadata.stagedPath << "\n";
    stream << "manifest_path=" << metadata.manifestPath << "\n";
    stream << "manifest_image_path=" << metadata.manifestImagePath << "\n";
    stream << "trust_status=" << metadata.trustStatus << "\n";
    stream << "trust_reject_reason=" << metadata.trustRejectReason << "\n";
    stream << "image_id=" << metadata.imageId << "\n";
    stream << "staged_software_version=" << metadata.stagedSoftwareVersion << "\n";
    stream << "active_software_version=" << metadata.activeSoftwareVersion << "\n";
    stream << "pending_software_version=" << metadata.pendingSoftwareVersion << "\n";
    stream << "last_known_good_software_version=" << metadata.lastKnownGoodSoftwareVersion << "\n";
    stream << "last_accepted_version=" << metadata.lastAcceptedVersion << "\n";
    stream << "signer_id=" << metadata.signerId << "\n";
    stream << "key_slot=" << metadata.keySlot << "\n";
    stream << "signature_algorithm=" << metadata.signatureAlgorithm << "\n";
    stream << "reset_cause=" << static_cast<U32>(metadata.resetCause.e) << "\n";
    stream << "boot_count=" << metadata.bootCount << "\n";
    stream << "consecutive_reset_count=" << metadata.consecutiveResetCount << "\n";
    stream << "last_recovery_source=" << static_cast<U32>(metadata.lastRecoverySource.e) << "\n";
    stream << "last_recovery_level=" << static_cast<U32>(metadata.lastRecoveryLevel.e) << "\n";
    stream << "boot_safe_fallback_required=" << (metadata.bootSafeFallbackRequired ? "1" : "0") << "\n";
    stream << "recovery_reset_pending=" << (metadata.recoveryResetPending ? "1" : "0") << "\n";

    return writeFile_(this->m_metadataPath, stream.str());
}

bool BootMetadataStore::computeDigestHex(const std::string& filePath, std::string& digestHex) {
    Os::File file;
    if (file.open(filePath.c_str(), Os::File::OPEN_READ) != Os::File::Status::OP_OK) {
        return false;
    }

    Sha256Accumulator hash;

    U8 buffer[HASH_CHUNK_SIZE];
    FwSizeType readSize = HASH_CHUNK_SIZE;
    Os::File::Status status = Os::File::Status::OP_OK;

    while (true) {
        readSize = HASH_CHUNK_SIZE;
        status = file.read(buffer, readSize, Os::File::WaitType::WAIT);
        if (status != Os::File::Status::OP_OK) {
            file.close();
            return false;
        }

        if (readSize == 0U) {
            break;
        }

        hash.update(buffer, readSize);
    }

    file.close();

    U8 digest[SHA256_DIGEST_SIZE] = {};
    hash.final(digest);
    digestHex = bytesToHex(digest, SHA256_DIGEST_SIZE);
    return true;
}

bool BootMetadataStore::parseSlot_(const std::string& value, OBC::BootSlot& slot) {
    if (value == "SLOT_A") {
        slot = OBC::BootSlot::SLOT_A;
        return true;
    }

    if (value == "SLOT_B") {
        slot = OBC::BootSlot::SLOT_B;
        return true;
    }

    if (value == "NONE") {
        slot = OBC::BootSlot::NONE;
        return true;
    }

    return false;
}

const char* BootMetadataStore::slotToString_(OBC::BootSlot slot) {
    switch (slot.e) {
        case OBC::BootSlot::SLOT_A:
            return "SLOT_A";
        case OBC::BootSlot::SLOT_B:
            return "SLOT_B";
        case OBC::BootSlot::NONE:
        default:
            return "NONE";
    }
}

bool BootMetadataStore::parseBool_(const std::string& value, bool& result) {
    if (value == "1" || value == "true") {
        result = true;
        return true;
    }

    if (value == "0" || value == "false") {
        result = false;
        return true;
    }

    return false;
}

std::string BootMetadataStore::trim_(const std::string& value) {
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

bool BootMetadataStore::readFile_(const std::string& path, std::string& contents) {
    FwSizeType fileSize = 0U;
    if (Os::FileSystem::getFileSize(path.c_str(), fileSize) != Os::FileSystem::Status::OP_OK) {
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

bool BootMetadataStore::writeFile_(const std::string& path, const std::string& contents) {
    Os::File file;
    if (file.open(path.c_str(), Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE) !=
        Os::File::Status::OP_OK) {
        return false;
    }

    FwSizeType writeSize = static_cast<FwSizeType>(contents.size());
    const Os::File::Status status = file.write(reinterpret_cast<const U8*>(contents.data()), writeSize, Os::File::WaitType::WAIT);
    file.close();

    return status == Os::File::Status::OP_OK && writeSize == contents.size();
}

bool BootMetadataStore::ensureDirectoryTree_() const {
    if (this->m_rootDir.empty()) {
        return false;
    }

    std::string current;
    if (this->m_rootDir.front() == '/') {
        current = "/";
    }

    std::istringstream pathStream(this->m_rootDir);
    std::string segment;
    while (std::getline(pathStream, segment, '/')) {
        if (segment.empty()) {
            continue;
        }

        if (!current.empty() && current.back() != '/') {
            current.push_back('/');
        }
        current += segment;

        const Os::FileSystem::PathType pathType = Os::FileSystem::getPathType(current.c_str());
        if (pathType == Os::FileSystem::PathType::DIRECTORY) {
            continue;
        }
        if (pathType == Os::FileSystem::PathType::FILE) {
            return false;
        }

        const Os::FileSystem::Status createStatus = Os::FileSystem::createDirectory(current.c_str(), false);
        if (createStatus != Os::FileSystem::Status::OP_OK &&
            createStatus != Os::FileSystem::Status::ALREADY_EXISTS) {
            return false;
        }
    }

    return true;
}

void BootMetadataStore::refreshPaths_() {
    this->m_metadataPath = this->m_rootDir;
    if (!this->m_metadataPath.empty() && this->m_metadataPath.back() != '/') {
        this->m_metadataPath.push_back('/');
    }
    this->m_metadataPath += "metadata-v1.txt";
}

}  // namespace OBC
