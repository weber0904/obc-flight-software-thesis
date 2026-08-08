#include "OBC/Components/PersistentFaultManager/PersistentFaultStore.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"
#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

constexpr U32 SNAPSHOT_MAGIC = 0x31524650U;  // PFR1 little-endian
constexpr U16 SNAPSHOT_VERSION = 1U;

struct SnapshotHeaderWire {
    U32 magic;
    U16 version;
    U16 reserved;
    U32 generation;
    U32 capacity;
    U32 count;
    U32 nextIndex;
    U32 recordBytes;
    U32 crc32;
};

static_assert(sizeof(SnapshotHeaderWire) == 32U, "Persistent fault snapshot header wire size must stay fixed");

struct RecordWire {
    U8 kind;
    U8 source;
    U8 level;
    U8 action;
    U8 resetCause;
    U8 reserved0;
    U16 reserved1;
    U32 timestampSec;
    U32 uptimeSec;
    U32 bootCount;
    U32 consecutiveResetCount;
    U32 detail;
    U32 flags;
};

static_assert(sizeof(RecordWire) == 32U, "Persistent fault record wire size must stay fixed");

constexpr std::size_t snapshotBytes() {
    return sizeof(SnapshotHeaderWire) + (sizeof(RecordWire) * static_cast<std::size_t>(OBC::PersistentFaultStoreCapacity));
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
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size < 0) {
        return false;
    }
    if (static_cast<std::size_t>(size) > snapshotBytes()) {
        return false;
    }
    input.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    return input.good() || input.eof();
}

bool writeFileBytes(const std::string& path, const std::vector<U8>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    if (!bytes.empty()) {
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    output.flush();
    return output.good();
}

RecordWire toWire(const OBC::PersistentFaultRecord& record) {
    RecordWire wire = {};
    wire.kind = static_cast<U8>(record.kind.e);
    wire.source = static_cast<U8>(record.source.e);
    wire.level = static_cast<U8>(record.level.e);
    wire.action = static_cast<U8>(record.action.e);
    wire.resetCause = static_cast<U8>(record.resetCause.e);
    wire.timestampSec = record.timestampSec;
    wire.uptimeSec = record.uptimeSec;
    wire.bootCount = record.bootCount;
    wire.consecutiveResetCount = record.consecutiveResetCount;
    wire.detail = record.detail;
    wire.flags = record.flags;
    return wire;
}

OBC::PersistentFaultRecord fromWire(const RecordWire& wire) {
    OBC::PersistentFaultRecord record = {};
    record.kind = OBC::PersistentFaultRecordKind(static_cast<OBC::PersistentFaultRecordKind::T>(wire.kind));
    record.source = OBC::RecoveryIncidentSource(static_cast<OBC::RecoveryIncidentSource::T>(wire.source));
    record.level = OBC::RecoveryLevel(static_cast<OBC::RecoveryLevel::T>(wire.level));
    record.action = OBC::RecoveryAction(static_cast<OBC::RecoveryAction::T>(wire.action));
    record.resetCause = OBC::ResetCause(static_cast<OBC::ResetCause::T>(wire.resetCause));
    record.timestampSec = wire.timestampSec;
    record.uptimeSec = wire.uptimeSec;
    record.bootCount = wire.bootCount;
    record.consecutiveResetCount = wire.consecutiveResetCount;
    record.detail = wire.detail;
    record.flags = wire.flags;
    return record;
}

bool parseCopy(const OBC::PersistentFaultStoreCopy copy, const std::string& path, OBC::PersistentFaultStore::Snapshot& snapshot) {
    static_cast<void>(copy);
    std::vector<U8> bytes;
    if (!readFileBytes(path, bytes)) {
        return false;
    }
    const std::size_t expectedSize = snapshotBytes();
    if (bytes.size() != expectedSize) {
        return false;
    }

    SnapshotHeaderWire header = {};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.magic != SNAPSHOT_MAGIC || header.version != SNAPSHOT_VERSION ||
        header.capacity != OBC::PersistentFaultStoreCapacity || header.recordBytes != sizeof(RecordWire) ||
        header.count > OBC::PersistentFaultStoreCapacity || header.nextIndex >= OBC::PersistentFaultStoreCapacity) {
        return false;
    }

    const U32 expectedCrc = header.crc32;
    std::memset(bytes.data() + offsetof(SnapshotHeaderWire, crc32), 0, sizeof(header.crc32));
    const U32 actualCrc = OBC::StateData::crc32(bytes.data(), static_cast<U32>(bytes.size()));
    if (expectedCrc != actualCrc) {
        return false;
    }

    snapshot.generation = header.generation;
    snapshot.count = header.count;
    snapshot.nextIndex = header.nextIndex;
    const U8* recordBytes = bytes.data() + sizeof(SnapshotHeaderWire);
    for (std::size_t i = 0; i < snapshot.records.size(); i++) {
        RecordWire wire = {};
        std::memcpy(&wire, recordBytes + (i * sizeof(RecordWire)), sizeof(RecordWire));
        snapshot.records[i] = fromWire(wire);
    }
    return true;
}

}  // namespace

PersistentFaultStore::PersistentFaultStore() : m_rootDir(), m_copyAPath(), m_copyBPath() {}

PersistentFaultStore::PersistentFaultStore(const std::string& rootDir)
    : m_rootDir(rootDir), m_copyAPath(), m_copyBPath() {
    this->refreshPaths_();
}

void PersistentFaultStore::setRootDir(const std::string& rootDir) {
    this->m_rootDir = rootDir;
    this->refreshPaths_();
}

const std::string& PersistentFaultStore::getRootDir() const {
    return this->m_rootDir;
}

std::string PersistentFaultStore::getCopyPath(const OBC::PersistentFaultStoreCopy copy) const {
    switch (copy.e) {
        case OBC::PersistentFaultStoreCopy::COPY_A:
            return this->m_copyAPath;
        case OBC::PersistentFaultStoreCopy::COPY_B:
            return this->m_copyBPath;
        default:
            return std::string();
    }
}

bool PersistentFaultStore::ensureStorage() const {
    return this->ensureDirectoryTree_();
}

bool PersistentFaultStore::append(const OBC::PersistentFaultRecord& record) {
    if (!this->ensureDirectoryTree_()) {
        return false;
    }

    Snapshot snapshot = {};
    OBC::PersistentFaultStoreCopy activeCopy = OBC::PersistentFaultStoreCopy::NONE;
    if (!this->loadSnapshot_(snapshot, activeCopy)) {
        return false;
    }

    snapshot.records[snapshot.nextIndex] = record;
    snapshot.nextIndex = (snapshot.nextIndex + 1U) % OBC::PersistentFaultStoreCapacity;
    snapshot.count = std::min(snapshot.count + 1U, OBC::PersistentFaultStoreCapacity);
    snapshot.generation += 1U;

    const OBC::PersistentFaultStoreCopy targetCopy =
        activeCopy == OBC::PersistentFaultStoreCopy::COPY_A ? OBC::PersistentFaultStoreCopy::COPY_B
                                                            : OBC::PersistentFaultStoreCopy::COPY_A;
    return this->writeCopy_(targetCopy, snapshot);
}

bool PersistentFaultStore::readLatest(U32 limit,
                                      OBC::PersistentFaultHistoryStatus& status,
                                      std::vector<OBC::PersistentFaultRecord>& records) const {
    Snapshot snapshot = {};
    OBC::PersistentFaultStoreCopy activeCopy = OBC::PersistentFaultStoreCopy::NONE;
    if (!this->loadSnapshot_(snapshot, activeCopy)) {
        return false;
    }

    const U32 requested = limit == 0U ? snapshot.count : limit;
    const U32 returned = std::min(requested, snapshot.count);
    status.totalRecords = snapshot.count;
    status.returnedRecords = returned;
    status.activeCopy = activeCopy;
    status.generation = snapshot.generation;

    records.clear();
    records.reserve(returned);
    if (returned == 0U) {
        return true;
    }

    U32 index = snapshot.nextIndex == 0U ? OBC::PersistentFaultStoreCapacity - 1U : snapshot.nextIndex - 1U;
    for (U32 i = 0; i < returned; i++) {
        records.push_back(snapshot.records[index]);
        index = index == 0U ? OBC::PersistentFaultStoreCapacity - 1U : index - 1U;
    }
    return true;
}

void PersistentFaultStore::initializeEmptySnapshot_(Snapshot& snapshot) {
    snapshot.generation = 0U;
    snapshot.count = 0U;
    snapshot.nextIndex = 0U;
    for (std::size_t i = 0; i < snapshot.records.size(); i++) {
        snapshot.records[i] = {};
    }
}

bool PersistentFaultStore::ensureDirectoryTree_() const {
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

void PersistentFaultStore::refreshPaths_() {
    this->m_copyAPath = joinPath(this->m_rootDir, "fault-ring-a.bin");
    this->m_copyBPath = joinPath(this->m_rootDir, "fault-ring-b.bin");
}

bool PersistentFaultStore::loadSnapshot_(Snapshot& snapshot, OBC::PersistentFaultStoreCopy& activeCopy) const {
    Snapshot aSnapshot = {};
    Snapshot bSnapshot = {};
    bool aValid = false;
    bool bValid = false;
    this->initializeEmptySnapshot_(aSnapshot);
    this->initializeEmptySnapshot_(bSnapshot);

    if (!this->m_copyAPath.empty() &&
        Os::FileSystem::getPathType(this->m_copyAPath.c_str()) != Os::FileSystem::PathType::NOT_EXIST) {
        aValid = parseCopy(OBC::PersistentFaultStoreCopy::COPY_A, this->m_copyAPath, aSnapshot);
    }
    if (!this->m_copyBPath.empty() &&
        Os::FileSystem::getPathType(this->m_copyBPath.c_str()) != Os::FileSystem::PathType::NOT_EXIST) {
        bValid = parseCopy(OBC::PersistentFaultStoreCopy::COPY_B, this->m_copyBPath, bSnapshot);
    }

    if (aValid && (!bValid || aSnapshot.generation >= bSnapshot.generation)) {
        snapshot = aSnapshot;
        activeCopy = OBC::PersistentFaultStoreCopy::COPY_A;
        return true;
    }
    if (bValid) {
        snapshot = bSnapshot;
        activeCopy = OBC::PersistentFaultStoreCopy::COPY_B;
        return true;
    }

    this->initializeEmptySnapshot_(snapshot);
    activeCopy = OBC::PersistentFaultStoreCopy::NONE;
    return true;
}

bool PersistentFaultStore::readCopy_(OBC::PersistentFaultStoreCopy copy, Snapshot& snapshot, bool& valid) const {
    const std::string path = this->getCopyPath(copy);
    valid = false;
    if (path.empty() || Os::FileSystem::getPathType(path.c_str()) == Os::FileSystem::PathType::NOT_EXIST) {
        return true;
    }
    valid = parseCopy(copy, path, snapshot);
    return true;
}

bool PersistentFaultStore::writeCopy_(OBC::PersistentFaultStoreCopy copy, const Snapshot& snapshot) const {
    const std::string path = this->getCopyPath(copy);
    if (path.empty()) {
        return false;
    }

    SnapshotHeaderWire header = {};
    header.magic = SNAPSHOT_MAGIC;
    header.version = SNAPSHOT_VERSION;
    header.generation = snapshot.generation;
    header.capacity = OBC::PersistentFaultStoreCapacity;
    header.count = snapshot.count;
    header.nextIndex = snapshot.nextIndex;
    header.recordBytes = sizeof(RecordWire);
    header.crc32 = 0U;

    const std::size_t totalBytes = snapshotBytes();
    std::vector<U8> bytes(totalBytes, 0U);
    std::memcpy(bytes.data(), &header, sizeof(header));
    U8* recordBytes = bytes.data() + sizeof(SnapshotHeaderWire);
    for (std::size_t i = 0; i < snapshot.records.size(); i++) {
        const RecordWire wire = toWire(snapshot.records[i]);
        std::memcpy(recordBytes + (i * sizeof(RecordWire)), &wire, sizeof(RecordWire));
    }

    header.crc32 = OBC::StateData::crc32(bytes.data(), static_cast<U32>(bytes.size()));
    std::memcpy(bytes.data(), &header, sizeof(header));
    return writeFileBytes(path, bytes);
}

}  // namespace OBC
