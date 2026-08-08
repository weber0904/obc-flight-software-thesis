#ifndef OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTMANAGER_HPP
#define OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTMANAGER_HPP

#include <mutex>
#include <string>
#include <vector>

#include "OBC/Components/PersistentFaultManager/PersistentFaultManagerComponentAc.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultStore.hpp"

namespace OBC {

class PersistentFaultManager final : public PersistentFaultManagerComponentBase,
                                     public OBC::IPersistentFaultRecorder,
                                     public OBC::IPersistentFaultHistoryProvider {
  public:
    explicit PersistentFaultManager(const char* const compName);

    ~PersistentFaultManager() override;

    bool configurePersistentRootForRuntime(const std::string& persistentRoot);

    bool appendPersistentFaultRecordForRuntime(const OBC::PersistentFaultRecord& record) override;

    bool getPersistentFaultHistoryForRuntime(U32 limit,
                                             OBC::PersistentFaultHistoryStatus& status,
                                             std::vector<OBC::PersistentFaultRecord>& records) const override;

  private:
    void GET_PERSISTENT_FAULT_HISTORY_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 limit) override;

    static std::string joinPath_(const std::string& base, const char* child);

    static U32 clampLimit_(U32 requested);

  private:
    mutable std::mutex m_mutex;
    OBC::PersistentFaultStore m_store;
};

}  // namespace OBC

#endif

