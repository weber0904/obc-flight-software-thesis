#include "OBC/Components/FileIngressAuthority/FileIngressPolicy.hpp"

#include <cerrno>
#include <cstdint>
#include <climits>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace OBC {

namespace {

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

std::uint32_t fnv1a32(const std::string& value) {
    std::uint32_t hash = 2166136261U;
    for (const unsigned char byte : value) {
        hash ^= static_cast<std::uint32_t>(byte);
        hash *= 16777619U;
    }
    return hash;
}

bool hasParentReference(const std::string& path) {
    std::size_t start = 0U;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part == "..") {
            return true;
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return false;
}

std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == '/') {
        return left + right;
    }
    return left + "/" + right;
}

bool pathExists(const std::string& path) {
    struct stat info = {};
    return ::lstat(path.c_str(), &info) == 0;
}

bool pathIsDirectory(const std::string& path) {
    struct stat info = {};
    return ::stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

bool makeDirectories(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path == "/") {
        return true;
    }

    std::string current;
    if (path.front() == '/') {
        current = "/";
    }

    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty()) {
            current = current.empty() || current == "/" ? current + part : current + "/" + part;
            if (::mkdir(current.c_str(), 0775) != 0) {
                if (errno != EEXIST || !pathIsDirectory(current)) {
                    return false;
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return true;
}

bool canonicalizeExistingPath(const std::string& path, std::string& canonicalPath) {
    char resolved[PATH_MAX] = {};
    if (::realpath(path.c_str(), resolved) == nullptr) {
        canonicalPath.clear();
        return false;
    }
    canonicalPath = resolved;
    return true;
}

bool getWorkingDirectory(std::string& workingDirectory) {
    char buffer[PATH_MAX] = {};
    if (::getcwd(buffer, sizeof(buffer)) == nullptr) {
        workingDirectory.clear();
        return false;
    }
    workingDirectory = buffer;
    return true;
}

std::string parentDirectory(const std::string& path) {
    const std::size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0U) {
        return "/";
    }
    return path.substr(0, pos);
}

bool readSymlinkTarget(const std::string& linkPath, std::string& targetPath) {
    char buffer[PATH_MAX] = {};
    const ssize_t bytes = ::readlink(linkPath.c_str(), buffer, sizeof(buffer) - 1);
    if (bytes < 0) {
        targetPath.clear();
        return false;
    }
    buffer[bytes] = '\0';
    targetPath = buffer;
    return true;
}

}  // namespace

bool FileIngressPolicy::configure(const std::string& runtimeRoot, const std::string& workingRoot) {
    this->m_configured = false;
    this->m_aliasPath.clear();
    this->m_logicalPrefix = LOGICAL_PREFIX;
    this->m_physicalStagingRoot.clear();
    this->m_runtimePrefix.clear();

    const std::string runtimePath = runtimeRoot.empty() ? "runtime" : runtimeRoot;
    std::string resolvedWorkingRoot = workingRoot;
    if (resolvedWorkingRoot.empty() && !getWorkingDirectory(resolvedWorkingRoot)) {
        return false;
    }

    const std::string physicalRoot = joinPath(runtimePath, PHYSICAL_STAGING_SUFFIX);
    if (!makeDirectories(physicalRoot)) {
        return false;
    }

    std::string canonicalPhysicalRoot;
    if (!canonicalizeExistingPath(physicalRoot, canonicalPhysicalRoot)) {
        return false;
    }

    char aliasName[32] = {};
    std::snprintf(aliasName, sizeof(aliasName), ".stg-%08x", fnv1a32(canonicalPhysicalRoot));
    const std::string aliasPath = joinPath(resolvedWorkingRoot, aliasName);
    struct stat aliasStatus = {};
    if (::lstat(aliasPath.c_str(), &aliasStatus) == 0) {
        if (!S_ISLNK(aliasStatus.st_mode)) {
            return false;
        }

        std::string targetPath;
        if (!readSymlinkTarget(aliasPath, targetPath)) {
            return false;
        }

        if (!targetPath.empty() && targetPath.front() != '/') {
            targetPath = joinPath(parentDirectory(aliasPath), targetPath);
        }

        std::string canonicalTargetPath;
        if (!canonicalizeExistingPath(targetPath, canonicalTargetPath)) {
            return false;
        }

        if (canonicalTargetPath != canonicalPhysicalRoot) {
            if (::unlink(aliasPath.c_str()) != 0) {
                return false;
            }
        }
    } else if (errno != ENOENT) {
        return false;
    }

    if (!pathExists(aliasPath)) {
        if (::symlink(canonicalPhysicalRoot.c_str(), aliasPath.c_str()) != 0) {
            return false;
        }
    }

    this->m_aliasPath = aliasPath;
    this->m_physicalStagingRoot = canonicalPhysicalRoot;
    this->m_runtimePrefix = std::string(aliasName) + "/";
    this->m_configured = true;
    return true;
}

bool FileIngressPolicy::validateDestinationPath(const std::string& destinationPath,
                                                FileIngressRejectReason& reason,
                                                std::string& canonicalPhysicalPath) const {
    canonicalPhysicalPath.clear();
    if (!this->m_configured) {
        reason = FileIngressRejectReason::NOT_CONFIGURED;
        return false;
    }

    if (destinationPath.empty()) {
        reason = FileIngressRejectReason::EMPTY_PATH;
        return false;
    }

    if (destinationPath.front() == '/') {
        reason = FileIngressRejectReason::ABSOLUTE_PATH;
        return false;
    }

    if (hasParentReference(destinationPath)) {
        reason = FileIngressRejectReason::PARENT_REFERENCE;
        return false;
    }

    if (!startsWith(destinationPath, this->m_logicalPrefix)) {
        reason = FileIngressRejectReason::PREFIX_MISMATCH;
        return false;
    }

    const std::string leaf = destinationPath.substr(this->m_logicalPrefix.size());
    if (leaf.empty() || leaf == "." || leaf == "..") {
        reason = FileIngressRejectReason::INVALID_FILENAME;
        return false;
    }

    if (leaf.find('/') != std::string::npos || leaf.find('\\') != std::string::npos) {
        reason = FileIngressRejectReason::SUBDIRECTORY_UNSUPPORTED;
        return false;
    }

    const std::string candidate = joinPath(this->m_physicalStagingRoot, leaf);
    struct stat candidateStatus = {};
    if (::lstat(candidate.c_str(), &candidateStatus) == 0) {
        if (S_ISLNK(candidateStatus.st_mode)) {
            reason = FileIngressRejectReason::SYMLINK_ESCAPE;
            return false;
        }
        if (S_ISDIR(candidateStatus.st_mode)) {
            reason = FileIngressRejectReason::EXISTING_PATH_INVALID;
            return false;
        }
    } else if (errno != ENOENT) {
        reason = FileIngressRejectReason::EXISTING_PATH_INVALID;
        return false;
    }

    canonicalPhysicalPath = candidate;
    reason = FileIngressRejectReason::NONE;
    return true;
}

bool FileIngressPolicy::isConfigured() const {
    return this->m_configured;
}

const std::string& FileIngressPolicy::getAliasPath() const {
    return this->m_aliasPath;
}

const std::string& FileIngressPolicy::getLogicalPrefix() const {
    return this->m_logicalPrefix;
}

const std::string& FileIngressPolicy::getPhysicalStagingRoot() const {
    return this->m_physicalStagingRoot;
}

const std::string& FileIngressPolicy::getRuntimePrefix() const {
    return this->m_runtimePrefix;
}

}  // namespace OBC
