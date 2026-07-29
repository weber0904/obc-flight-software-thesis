#ifndef OBC_CommandFreshnessStore_HPP
#define OBC_CommandFreshnessStore_HPP

#include <array>
#include <string>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

namespace OBC {

enum class CommandFreshnessStoreCopy : U32 {
    NONE = 0,
    COPY_A = 1,
    COPY_B = 2,
};

const char* commandFreshnessStoreCopyName(CommandFreshnessStoreCopy copy);

class CommandFreshnessStore final {
  public:
    static constexpr FwSizeType MAX_SOURCE_EPOCHS = 16U;

    struct SourceEpochKey {
        FwIndexType ingressPort = 0U;
        AuthorityLinkIdentity linkIdentity = AuthorityLinkIdentity::UNKNOWN;
        AuthorityLinkRole linkRole = AuthorityLinkRole::UNKNOWN;
    };

    struct Entry {
        bool valid = false;
        SourceEpochKey key = {};
        U32 sessionFloor = 0U;
    };

    struct Snapshot {
        U32 generation = 0U;
        U32 count = 0U;
        std::array<Entry, MAX_SOURCE_EPOCHS> entries = {};
    };

    enum class LoadStatus {
        OK,
        EMPTY,
        INVALID,
    };

  public:
    CommandFreshnessStore();

    explicit CommandFreshnessStore(const std::string& rootDir);

    void setRootDir(const std::string& rootDir);

    const std::string& getRootDir() const;

    std::string getCopyPath(CommandFreshnessStoreCopy copy) const;

    bool ensureStorage() const;

    LoadStatus load(Snapshot& snapshot, CommandFreshnessStoreCopy& activeCopy) const;

    bool persistFloor(const SourceEpochKey& key,
                      U32 sessionFloor,
                      Snapshot& snapshot,
                      CommandFreshnessStoreCopy& activeCopy) const;

    static bool findFloor(const Snapshot& snapshot, const SourceEpochKey& key, U32& sessionFloor);

  private:
    static void initializeEmptySnapshot_(Snapshot& snapshot);

    static bool keysEqual_(const SourceEpochKey& lhs, const SourceEpochKey& rhs);

    static Entry* findEntry_(Snapshot& snapshot, const SourceEpochKey& key);

    static const Entry* findEntry_(const Snapshot& snapshot, const SourceEpochKey& key);

    static Entry* allocateEntry_(Snapshot& snapshot, const SourceEpochKey& key);

    bool ensureDirectoryTree_() const;

    void refreshPaths_();

    bool writeCopy_(CommandFreshnessStoreCopy copy, const Snapshot& snapshot) const;

  private:
    std::string m_rootDir;
    std::string m_copyAPath;
    std::string m_copyBPath;
};

}  // namespace OBC

#endif
