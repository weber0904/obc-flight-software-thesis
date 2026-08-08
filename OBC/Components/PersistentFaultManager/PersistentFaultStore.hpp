#ifndef OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTSTORE_HPP
#define OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTSTORE_HPP

#include <array>
#include <string>
#include <vector>

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"

namespace OBC {

class PersistentFaultStore final {
  public:
    struct Snapshot {
        U32 generation = 0U;
        U32 count = 0U;
        U32 nextIndex = 0U;
        std::array<OBC::PersistentFaultRecord, OBC::PersistentFaultStoreCapacity> records = {};
    };

    PersistentFaultStore();

    explicit PersistentFaultStore(const std::string& rootDir);

    void setRootDir(const std::string& rootDir);

    const std::string& getRootDir() const;

    std::string getCopyPath(OBC::PersistentFaultStoreCopy copy) const;

    bool ensureStorage() const;

    bool append(const OBC::PersistentFaultRecord& record);

    bool readLatest(U32 limit,
                    OBC::PersistentFaultHistoryStatus& status,
                    std::vector<OBC::PersistentFaultRecord>& records) const;

  private:
    static void initializeEmptySnapshot_(Snapshot& snapshot);

    bool ensureDirectoryTree_() const;

    void refreshPaths_();

    bool loadSnapshot_(Snapshot& snapshot, OBC::PersistentFaultStoreCopy& activeCopy) const;

    bool readCopy_(OBC::PersistentFaultStoreCopy copy, Snapshot& snapshot, bool& valid) const;

    bool writeCopy_(OBC::PersistentFaultStoreCopy copy, const Snapshot& snapshot) const;

  private:
    std::string m_rootDir;
    std::string m_copyAPath;
    std::string m_copyBPath;
};

}  // namespace OBC

#endif
