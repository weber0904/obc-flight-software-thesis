#include "OBC/Components/StorageHealthBridge/StorageScanner.hpp"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace OBC {
namespace STORAGE {

namespace {

bool isDotEntry(const char* name) {
    return name != nullptr &&
           ((name[0] == '.' && name[1] == '\0') || (name[0] == '.' && name[1] == '.' && name[2] == '\0'));
}

bool namesEqual(const char* lhs, const char* rhs) {
    return lhs != nullptr && rhs != nullptr && std::strcmp(lhs, rhs) == 0;
}

}  // namespace

StorageScanner::StorageScanner()
    : m_runtimeRoot(),
      m_persistentRoot(),
      m_stagingRoot(),
      m_logsRoot(),
      m_dataProductsRoot(),
      m_warningThresholdBytes(0U),
      m_dataProductsQuotaBytes(0U) {}

void StorageScanner::configure(const std::string& runtimeRoot,
                               const std::string& persistentRoot,
                               const std::string& stagingRoot,
                               U32 warningThresholdBytes,
                               U32 dataProductsQuotaBytes) {
    this->m_runtimeRoot = runtimeRoot;
    this->m_logsRoot = joinPath_(runtimeRoot, "logs");
    this->m_dataProductsRoot = joinPath_(runtimeRoot, "data-products");
    this->m_persistentRoot = persistentRoot.empty() ? joinPath_(runtimeRoot, "persistent-data") : persistentRoot;
    this->m_stagingRoot = stagingRoot.empty() ? joinPath_(runtimeRoot, "staging") : stagingRoot;
    this->m_warningThresholdBytes = warningThresholdBytes;
    this->m_dataProductsQuotaBytes = dataProductsQuotaBytes;
}

bool StorageScanner::scan(HealthState& state) const {
    state = {};

    bool ignored = false;
    const bool persistentOk = scanRoot_(this->m_persistentRoot, state.persistent, ignored);
    const bool stagingOk = scanRoot_(this->m_stagingRoot, state.staging, ignored);
    const bool logsOk = scanRoot_(this->m_logsRoot, state.logs, ignored);
    const bool dataProductsOk = scanRoot_(this->m_dataProductsRoot, state.dataProducts, ignored);

    applyPolicy_(state.persistent, 0U, this->m_warningThresholdBytes);
    applyPolicy_(state.staging, 0U, this->m_warningThresholdBytes);
    applyPolicy_(state.logs, 0U, this->m_warningThresholdBytes);
    applyPolicy_(state.dataProducts, this->m_dataProductsQuotaBytes, this->m_warningThresholdBytes);

    state.hasScan = true;
    state.scanCount = 1U;
    state.scanErrorCount = (persistentOk ? 0U : 1U) + (stagingOk ? 0U : 1U) + (logsOk ? 0U : 1U) +
                           (dataProductsOk ? 0U : 1U);
    evaluateWarnings_(state, this->m_warningThresholdBytes, state.warningActive, state.warningMask);
    evaluateDegraded_(state, state.degradedMask);
    return state.scanErrorCount == 0U;
}

const std::string& StorageScanner::getPersistentRoot() const {
    return this->m_persistentRoot;
}

const std::string& StorageScanner::getStagingRoot() const {
    return this->m_stagingRoot;
}

const std::string& StorageScanner::getLogsRoot() const {
    return this->m_logsRoot;
}

const std::string& StorageScanner::getDataProductsRoot() const {
    return this->m_dataProductsRoot;
}

U32 StorageScanner::getWarningThresholdBytes() const {
    return this->m_warningThresholdBytes;
}

U32 StorageScanner::getDataProductsQuotaBytes() const {
    return this->m_dataProductsQuotaBytes;
}

bool StorageScanner::isMissingErrno_(int err) {
    return err == ENOENT || err == ENOTDIR;
}

void StorageScanner::setErrorCodeIfUnset_(RootStats& stats, int err) {
    if (stats.errorCode == 0U) {
        const int normalized = err == 0 ? EIO : err;
        stats.errorCode = static_cast<U32>(normalized);
    }
}

std::string StorageScanner::joinPath_(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    if (base.back() == '/') {
        return base + child;
    }
    return base + "/" + child;
}

bool StorageScanner::scanRoot_(const std::string& path, RootStats& stats, bool& indexExists) {
    stats = {};
    indexExists = false;

    struct stat info = {};
    if (::stat(path.c_str(), &info) != 0) {
        const int statErrno = errno;
        stats.exists = !isMissingErrno_(statErrno);
        stats.scanOk = false;
        setErrorCodeIfUnset_(stats, statErrno);
        return false;
    }

    stats.exists = true;
    if (!S_ISDIR(info.st_mode)) {
        stats.scanOk = false;
        setErrorCodeIfUnset_(stats, ENOTDIR);
        return false;
    }

    stats.scanOk = scanDirectoryRecursive_(path, stats);
    return stats.scanOk;
}

bool StorageScanner::scanDirectoryRecursive_(const std::string& path, RootStats& stats) {
    DIR* dir = ::opendir(path.c_str());
    if (dir == nullptr) {
        setErrorCodeIfUnset_(stats, errno);
        return false;
    }

    bool success = true;
    while (true) {
        errno = 0;
        dirent* entry = ::readdir(dir);
        if (entry == nullptr) {
            if (errno != 0) {
                setErrorCodeIfUnset_(stats, errno);
                success = false;
            }
            break;
        }

        if (isDotEntry(entry->d_name)) {
            continue;
        }

        const std::string childPath = joinPath_(path, entry->d_name);
        struct stat childInfo = {};
        if (::stat(childPath.c_str(), &childInfo) != 0) {
            setErrorCodeIfUnset_(stats, errno);
            success = false;
            continue;
        }

        if (S_ISDIR(childInfo.st_mode)) {
            if (!scanDirectoryRecursive_(childPath, stats)) {
                success = false;
            }
            continue;
        }

        if (S_ISREG(childInfo.st_mode)) {
            stats.fileCount++;
            if (childInfo.st_size > 0) {
                const unsigned long long rawSize = static_cast<unsigned long long>(childInfo.st_size);
                const unsigned long long capped = rawSize > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : rawSize;
                const unsigned long long current = static_cast<unsigned long long>(stats.totalBytes);
                const unsigned long long sum = current + capped;
                stats.totalBytes = static_cast<U32>(sum > 0xFFFFFFFFULL ? 0xFFFFFFFFULL : sum);
            }
        }
    }

    ::closedir(dir);
    return success;
}

void StorageScanner::applyPolicy_(RootStats& stats, U32 quotaBytes, U32 watermarkBytes) {
    stats.quotaBytes = quotaBytes;
    stats.watermarkBytes = watermarkBytes;
    stats.retentionStatus = STORAGE_RETENTION_OBSERVE_ONLY;
    if (quotaBytes == 0U) {
        stats.quotaStatus = STORAGE_QUOTA_NOT_CONFIGURED;
    } else if (!stats.scanOk) {
        stats.quotaStatus = STORAGE_QUOTA_UNAVAILABLE;
    } else if (stats.scanOk && stats.totalBytes > quotaBytes) {
        stats.quotaStatus = STORAGE_QUOTA_OVER_QUOTA;
    } else {
        stats.quotaStatus = STORAGE_QUOTA_OK;
    }
}

void StorageScanner::evaluateWarnings_(const HealthState& state,
                                       U32 warningThresholdBytes,
                                       bool& warningActive,
                                       U8& warningMask) {
    warningActive = false;
    warningMask = 0U;
    if (warningThresholdBytes == 0U) {
        return;
    }

    const RootStats roots[] = {state.persistent, state.staging, state.logs, state.dataProducts};
    for (U8 index = 0U; index < 4U; ++index) {
        if (roots[index].scanOk && roots[index].totalBytes >= warningThresholdBytes) {
            warningActive = true;
            warningMask = static_cast<U8>(warningMask | static_cast<U8>(1U << index));
        }
    }
}

void StorageScanner::evaluateDegraded_(const HealthState& state, U8& degradedMask) {
    degradedMask = 0U;
    const RootStats roots[] = {state.persistent, state.staging, state.logs, state.dataProducts};
    for (U8 index = 0U; index < 4U; ++index) {
        if (!roots[index].scanOk) {
            degradedMask = static_cast<U8>(degradedMask | static_cast<U8>(1U << index));
        }
    }
}

}  // namespace STORAGE
}  // namespace OBC
