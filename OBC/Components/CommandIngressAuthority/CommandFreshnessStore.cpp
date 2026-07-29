#include "OBC/Components/CommandIngressAuthority/CommandFreshnessStore.hpp"

#include <array>
#include <cstring>
#include <sstream>
#include <vector>

#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

constexpr U32 SNAPSHOT_MAGIC = 0x31465343U;  // CSF1 little-endian
constexpr U16 SNAPSHOT_VERSION = 1U;
constexpr std::size_t SNAPSHOT_HEADER_BYTES = 28U;
constexpr std::size_t ENTRY_WIRE_BYTES = 20U;

struct SnapshotHeaderWire {
    U32 magic;
    U16 version;
    U16 reserved;
    U32 generation;
    U32 capacity;
    U32 count;
    U32 recordBytes;
    U32 crc32;
};

static_assert(sizeof(SnapshotHeaderWire) == SNAPSHOT_HEADER_BYTES,
              "Command freshness snapshot header wire size must stay fixed");

struct EntryWire {
    U32 valid;
    U32 ingressPort;
    U32 linkIdentity;
    U32 linkRole;
    U32 sessionFloor;
};

static_assert(sizeof(EntryWire) == ENTRY_WIRE_BYTES, "Command freshness snapshot entry wire size must stay fixed");

constexpr std::size_t snapshotBytes() {
    return SNAPSHOT_HEADER_BYTES + (ENTRY_WIRE_BYTES * static_cast<std::size_t>(CommandFreshnessStore::MAX_SOURCE_EPOCHS));
}

std::string joinPath(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    return base.back() == '/' ? base + child : base + "/" + child;
}

bool readFileBytes(const std::string& path, std::vector<U8>& bytes) {
    bytes.clear();

    FwSizeType fileSize = 0U;
    if (Os::FileSystem::getFileSize(path.c_str(), fileSize) != Os::FileSystem::Status::OP_OK) {
        return false;
    }

    if (fileSize > snapshotBytes()) {
        return false;
    }

    Os::File input;
    if (input.open(path.c_str(), Os::File::OPEN_READ) != Os::File::Status::OP_OK) {
        return false;
    }

    bytes.resize(fileSize);
    FwSizeType readSize = fileSize;
    const Os::File::Status status =
        fileSize == 0U ? Os::File::Status::OP_OK : input.read(bytes.data(), readSize, Os::File::WaitType::WAIT);
    input.close();

    return status == Os::File::Status::OP_OK && readSize == fileSize;
}

bool writeFileBytes(const std::string& path, const std::vector<U8>& bytes) {
    Os::File output;
    if (output.open(path.c_str(), Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE) !=
        Os::File::Status::OP_OK) {
        return false;
    }

    FwSizeType writeSize = static_cast<FwSizeType>(bytes.size());
    const Os::File::Status status =
        writeSize == 0U ? Os::File::Status::OP_OK : output.write(bytes.data(), writeSize, Os::File::WaitType::WAIT);
    output.close();

    return status == Os::File::Status::OP_OK && writeSize == bytes.size();
}

U16 loadLittleEndian16(const U8* data) {
    return static_cast<U16>(static_cast<U16>(data[0]) | (static_cast<U16>(data[1]) << 8U));
}

U32 loadLittleEndian32(const U8* data) {
    return static_cast<U32>(static_cast<U32>(data[0]) | (static_cast<U32>(data[1]) << 8U) |
                            (static_cast<U32>(data[2]) << 16U) | (static_cast<U32>(data[3]) << 24U));
}

void storeLittleEndian16(U8* data, U16 value) {
    data[0] = static_cast<U8>(value & 0xFFU);
    data[1] = static_cast<U8>((value >> 8U) & 0xFFU);
}

void storeLittleEndian32(U8* data, U32 value) {
    data[0] = static_cast<U8>(value & 0xFFU);
    data[1] = static_cast<U8>((value >> 8U) & 0xFFU);
    data[2] = static_cast<U8>((value >> 16U) & 0xFFU);
    data[3] = static_cast<U8>((value >> 24U) & 0xFFU);
}

SnapshotHeaderWire decodeHeader(const U8* data) {
    SnapshotHeaderWire header = {};
    header.magic = loadLittleEndian32(data + 0U);
    header.version = loadLittleEndian16(data + 4U);
    header.reserved = loadLittleEndian16(data + 6U);
    header.generation = loadLittleEndian32(data + 8U);
    header.capacity = loadLittleEndian32(data + 12U);
    header.count = loadLittleEndian32(data + 16U);
    header.recordBytes = loadLittleEndian32(data + 20U);
    header.crc32 = loadLittleEndian32(data + 24U);
    return header;
}

void encodeHeader(U8* data, const SnapshotHeaderWire& header) {
    storeLittleEndian32(data + 0U, header.magic);
    storeLittleEndian16(data + 4U, header.version);
    storeLittleEndian16(data + 6U, header.reserved);
    storeLittleEndian32(data + 8U, header.generation);
    storeLittleEndian32(data + 12U, header.capacity);
    storeLittleEndian32(data + 16U, header.count);
    storeLittleEndian32(data + 20U, header.recordBytes);
    storeLittleEndian32(data + 24U, header.crc32);
}

EntryWire decodeEntry(const U8* data) {
    EntryWire wire = {};
    wire.valid = loadLittleEndian32(data + 0U);
    wire.ingressPort = loadLittleEndian32(data + 4U);
    wire.linkIdentity = loadLittleEndian32(data + 8U);
    wire.linkRole = loadLittleEndian32(data + 12U);
    wire.sessionFloor = loadLittleEndian32(data + 16U);
    return wire;
}

void encodeEntry(U8* data, const EntryWire& wire) {
    storeLittleEndian32(data + 0U, wire.valid);
    storeLittleEndian32(data + 4U, wire.ingressPort);
    storeLittleEndian32(data + 8U, wire.linkIdentity);
    storeLittleEndian32(data + 12U, wire.linkRole);
    storeLittleEndian32(data + 16U, wire.sessionFloor);
}

EntryWire toWire(const CommandFreshnessStore::Entry& entry) {
    EntryWire wire = {};
    wire.valid = entry.valid ? 1U : 0U;
    wire.ingressPort = static_cast<U32>(entry.key.ingressPort);
    wire.linkIdentity = static_cast<U32>(entry.key.linkIdentity);
    wire.linkRole = static_cast<U32>(entry.key.linkRole);
    wire.sessionFloor = entry.sessionFloor;
    return wire;
}

CommandFreshnessStore::Entry fromWire(const EntryWire& wire) {
    CommandFreshnessStore::Entry entry = {};
    entry.valid = wire.valid != 0U;
    entry.key.ingressPort = wire.ingressPort;
    entry.key.linkIdentity = static_cast<AuthorityLinkIdentity>(wire.linkIdentity);
    entry.key.linkRole = static_cast<AuthorityLinkRole>(wire.linkRole);
    entry.sessionFloor = wire.sessionFloor;
    return entry;
}

bool parseCopy(const std::string& path, CommandFreshnessStore::Snapshot& snapshot) {
    std::vector<U8> bytes;
    if (!readFileBytes(path, bytes)) {
        return false;
    }
    if (bytes.size() != snapshotBytes()) {
        return false;
    }

    const SnapshotHeaderWire header = decodeHeader(bytes.data());
    if (header.magic != SNAPSHOT_MAGIC || header.version != SNAPSHOT_VERSION ||
        header.capacity != CommandFreshnessStore::MAX_SOURCE_EPOCHS || header.recordBytes != ENTRY_WIRE_BYTES ||
        header.count > CommandFreshnessStore::MAX_SOURCE_EPOCHS) {
        return false;
    }

    const U32 expectedCrc = header.crc32;
    std::memset(bytes.data() + 24U, 0, sizeof(header.crc32));
    const U32 actualCrc = OBC::StateData::crc32(bytes.data(), static_cast<U32>(bytes.size()));
    if (expectedCrc != actualCrc) {
        return false;
    }

    snapshot.generation = header.generation;
    snapshot.count = header.count;
    const U8* entryBytes = bytes.data() + SNAPSHOT_HEADER_BYTES;
    U32 counted = 0U;
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        const EntryWire wire = decodeEntry(entryBytes + (i * ENTRY_WIRE_BYTES));
        snapshot.entries[i] = fromWire(wire);
        if (snapshot.entries[i].valid) {
            counted++;
        }
    }
    return counted == snapshot.count;
}

}  // namespace

const char* commandFreshnessStoreCopyName(const CommandFreshnessStoreCopy copy) {
    switch (copy) {
        case CommandFreshnessStoreCopy::NONE:
            return "none";
        case CommandFreshnessStoreCopy::COPY_A:
            return "copy-a";
        case CommandFreshnessStoreCopy::COPY_B:
            return "copy-b";
        default:
            return "unknown";
    }
}

CommandFreshnessStore::CommandFreshnessStore() : m_rootDir(), m_copyAPath(), m_copyBPath() {}

CommandFreshnessStore::CommandFreshnessStore(const std::string& rootDir) : m_rootDir(rootDir), m_copyAPath(), m_copyBPath() {
    this->refreshPaths_();
}

void CommandFreshnessStore::setRootDir(const std::string& rootDir) {
    this->m_rootDir = rootDir;
    this->refreshPaths_();
}

const std::string& CommandFreshnessStore::getRootDir() const {
    return this->m_rootDir;
}

std::string CommandFreshnessStore::getCopyPath(const CommandFreshnessStoreCopy copy) const {
    switch (copy) {
        case CommandFreshnessStoreCopy::COPY_A:
            return this->m_copyAPath;
        case CommandFreshnessStoreCopy::COPY_B:
            return this->m_copyBPath;
        default:
            return std::string();
    }
}

bool CommandFreshnessStore::ensureStorage() const {
    return this->ensureDirectoryTree_();
}

CommandFreshnessStore::LoadStatus CommandFreshnessStore::load(Snapshot& snapshot,
                                                              CommandFreshnessStoreCopy& activeCopy) const {
    Snapshot aSnapshot = {};
    Snapshot bSnapshot = {};
    bool aPresent = false;
    bool bPresent = false;
    bool aValid = false;
    bool bValid = false;
    initializeEmptySnapshot_(aSnapshot);
    initializeEmptySnapshot_(bSnapshot);

    if (!this->m_copyAPath.empty() &&
        Os::FileSystem::getPathType(this->m_copyAPath.c_str()) != Os::FileSystem::PathType::NOT_EXIST) {
        aPresent = true;
        aValid = parseCopy(this->m_copyAPath, aSnapshot);
    }
    if (!this->m_copyBPath.empty() &&
        Os::FileSystem::getPathType(this->m_copyBPath.c_str()) != Os::FileSystem::PathType::NOT_EXIST) {
        bPresent = true;
        bValid = parseCopy(this->m_copyBPath, bSnapshot);
    }

    if (aValid && (!bValid || aSnapshot.generation >= bSnapshot.generation)) {
        snapshot = aSnapshot;
        activeCopy = CommandFreshnessStoreCopy::COPY_A;
        return LoadStatus::OK;
    }
    if (bValid) {
        snapshot = bSnapshot;
        activeCopy = CommandFreshnessStoreCopy::COPY_B;
        return LoadStatus::OK;
    }
    if (aPresent || bPresent) {
        initializeEmptySnapshot_(snapshot);
        activeCopy = CommandFreshnessStoreCopy::NONE;
        return LoadStatus::INVALID;
    }

    initializeEmptySnapshot_(snapshot);
    activeCopy = CommandFreshnessStoreCopy::NONE;
    return LoadStatus::EMPTY;
}

bool CommandFreshnessStore::persistFloor(const SourceEpochKey& key,
                                         const U32 sessionFloor,
                                         Snapshot& snapshot,
                                         CommandFreshnessStoreCopy& activeCopy) const {
    if (!this->ensureDirectoryTree_()) {
        return false;
    }

    Snapshot nextSnapshot = snapshot;
    Entry* entry = findEntry_(nextSnapshot, key);
    const bool hadExistingEntry = entry != nullptr;
    if (entry == nullptr) {
        entry = allocateEntry_(nextSnapshot, key);
        if (entry == nullptr) {
            return false;
        }
    }

    if (hadExistingEntry && entry->valid && sessionFloor <= entry->sessionFloor) {
        return true;
    }

    entry->valid = true;
    entry->key = key;
    entry->sessionFloor = sessionFloor;
    nextSnapshot.generation += 1U;

    const CommandFreshnessStoreCopy targetCopy =
        activeCopy == CommandFreshnessStoreCopy::COPY_A ? CommandFreshnessStoreCopy::COPY_B
                                                        : CommandFreshnessStoreCopy::COPY_A;
    if (!this->writeCopy_(targetCopy, nextSnapshot)) {
        return false;
    }
    snapshot = nextSnapshot;
    activeCopy = targetCopy;
    return true;
}

bool CommandFreshnessStore::findFloor(const Snapshot& snapshot, const SourceEpochKey& key, U32& sessionFloor) {
    const Entry* const entry = findEntry_(snapshot, key);
    if (entry == nullptr || !entry->valid) {
        return false;
    }
    sessionFloor = entry->sessionFloor;
    return true;
}

void CommandFreshnessStore::initializeEmptySnapshot_(Snapshot& snapshot) {
    snapshot.generation = 0U;
    snapshot.count = 0U;
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        snapshot.entries[i] = Entry();
    }
}

bool CommandFreshnessStore::keysEqual_(const SourceEpochKey& lhs, const SourceEpochKey& rhs) {
    return lhs.ingressPort == rhs.ingressPort && lhs.linkIdentity == rhs.linkIdentity && lhs.linkRole == rhs.linkRole;
}

CommandFreshnessStore::Entry* CommandFreshnessStore::findEntry_(Snapshot& snapshot, const SourceEpochKey& key) {
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        Entry& entry = snapshot.entries[i];
        if (entry.valid && keysEqual_(entry.key, key)) {
            return &entry;
        }
    }
    return nullptr;
}

const CommandFreshnessStore::Entry* CommandFreshnessStore::findEntry_(const Snapshot& snapshot, const SourceEpochKey& key) {
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        const Entry& entry = snapshot.entries[i];
        if (entry.valid && keysEqual_(entry.key, key)) {
            return &entry;
        }
    }
    return nullptr;
}

CommandFreshnessStore::Entry* CommandFreshnessStore::allocateEntry_(Snapshot& snapshot, const SourceEpochKey& key) {
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        Entry& entry = snapshot.entries[i];
        if (!entry.valid) {
            entry.valid = true;
            entry.key = key;
            entry.sessionFloor = 0U;
            snapshot.count += 1U;
            return &entry;
        }
    }
    return nullptr;
}

bool CommandFreshnessStore::ensureDirectoryTree_() const {
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
        const Os::FileSystem::Status status = Os::FileSystem::createDirectory(current.c_str(), false);
        if (status != Os::FileSystem::Status::OP_OK && status != Os::FileSystem::Status::ALREADY_EXISTS) {
            return false;
        }
    }
    return true;
}

void CommandFreshnessStore::refreshPaths_() {
    this->m_copyAPath = joinPath(this->m_rootDir, "session-floor-a.bin");
    this->m_copyBPath = joinPath(this->m_rootDir, "session-floor-b.bin");
}

bool CommandFreshnessStore::writeCopy_(const CommandFreshnessStoreCopy copy, const Snapshot& snapshot) const {
    const std::string path = this->getCopyPath(copy);
    if (path.empty()) {
        return false;
    }

    SnapshotHeaderWire header = {};
    header.magic = SNAPSHOT_MAGIC;
    header.version = SNAPSHOT_VERSION;
    header.generation = snapshot.generation;
    header.capacity = MAX_SOURCE_EPOCHS;
    header.count = snapshot.count;
    header.recordBytes = ENTRY_WIRE_BYTES;

    std::vector<U8> bytes(snapshotBytes(), 0U);
    encodeHeader(bytes.data(), header);

    U8* const entryBytes = bytes.data() + SNAPSHOT_HEADER_BYTES;
    for (std::size_t i = 0; i < snapshot.entries.size(); ++i) {
        const EntryWire wire = toWire(snapshot.entries[i]);
        encodeEntry(entryBytes + (i * ENTRY_WIRE_BYTES), wire);
    }

    const U32 crc = OBC::StateData::crc32(bytes.data(), static_cast<U32>(bytes.size()));
    storeLittleEndian32(bytes.data() + 24U, crc);
    return writeFileBytes(path, bytes);
}

}  // namespace OBC
