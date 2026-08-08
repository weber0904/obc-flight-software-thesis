#ifndef OBC_COMPONENTS_TEST_TESTSUPPORT_HPP
#define OBC_COMPONENTS_TEST_TESTSUPPORT_HPP

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "Fw/FPrimeBasicTypes.hpp"

namespace OBC {
namespace TestSupport {

inline std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    return left.back() == '/' ? left + right : left + "/" + right;
}

inline bool ensureDirectory(const std::string& path) {
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
            if (::mkdir(current.c_str(), 0775) != 0 && errno != EEXIST) {
                return false;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return true;
}

inline bool writeBinaryFile(const std::string& path, const std::vector<U8>& bytes) {
    const std::size_t slash = path.find_last_of('/');
    if (slash != std::string::npos && !ensureDirectory(path.substr(0, slash))) {
        return false;
    }
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

inline std::vector<U8> readBinaryFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }
    return std::vector<U8>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

inline bool recursiveRemove(const std::string& path) {
    struct stat info = {};
    if (::lstat(path.c_str(), &info) != 0) {
        return errno == ENOENT;
    }

    if (S_ISDIR(info.st_mode) && !S_ISLNK(info.st_mode)) {
        DIR* dir = ::opendir(path.c_str());
        if (dir == nullptr) {
            return false;
        }

        bool ok = true;
        while (ok) {
            errno = 0;
            dirent* entry = ::readdir(dir);
            if (entry == nullptr) {
                ok = errno == 0;
                break;
            }
            if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            ok = recursiveRemove(joinPath(path, entry->d_name));
        }
        ::closedir(dir);
        return ok && (::rmdir(path.c_str()) == 0);
    }

    return ::unlink(path.c_str()) == 0;
}

class TempDirectory final {
  public:
    TempDirectory() {
        char pattern[] = "/tmp/obc-seq-test-XXXXXX";
        char* created = ::mkdtemp(pattern);
        if (created != nullptr) {
            this->m_path = created;
        }
    }

    ~TempDirectory() {
        if (!this->m_path.empty()) {
            static_cast<void>(recursiveRemove(this->m_path));
        }
    }

    const std::string& path() const { return this->m_path; }
    bool valid() const { return !this->m_path.empty(); }

  private:
    std::string m_path;
};

}  // namespace TestSupport
}  // namespace OBC

#endif
