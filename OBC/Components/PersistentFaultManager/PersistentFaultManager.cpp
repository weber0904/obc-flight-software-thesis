#include "OBC/Components/PersistentFaultManager/PersistentFaultManager.hpp"

#include <algorithm>

namespace OBC {

PersistentFaultManager::PersistentFaultManager(const char* const compName)
    : PersistentFaultManagerComponentBase(compName), m_mutex(), m_store() {}

PersistentFaultManager::~PersistentFaultManager() = default;

bool PersistentFaultManager::configurePersistentRootForRuntime(const std::string& persistentRoot) {
    if (persistentRoot.empty()) {
        return false;
    }

    const std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_store.setRootDir(joinPath_(persistentRoot, "recovery"));
    return this->m_store.ensureStorage();
}

bool PersistentFaultManager::appendPersistentFaultRecordForRuntime(const OBC::PersistentFaultRecord& record) {
    const std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_store.getRootDir().empty()) {
        this->log_WARNING_HI_PERSISTENT_FAULT_STORE_APPEND_FAILED(record.kind);
        return false;
    }
    const bool ok = this->m_store.append(record);
    if (!ok) {
        this->log_WARNING_HI_PERSISTENT_FAULT_STORE_APPEND_FAILED(record.kind);
    }
    return ok;
}

bool PersistentFaultManager::getPersistentFaultHistoryForRuntime(U32 limit,
                                                                 OBC::PersistentFaultHistoryStatus& status,
                                                                 std::vector<OBC::PersistentFaultRecord>& records) const {
    const std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_store.getRootDir().empty()) {
        return false;
    }
    return this->m_store.readLatest(clampLimit_(limit), status, records);
}

void PersistentFaultManager::GET_PERSISTENT_FAULT_HISTORY_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 limit) {
    OBC::PersistentFaultHistoryStatus status = {};
    std::vector<OBC::PersistentFaultRecord> records;
    if (!this->getPersistentFaultHistoryForRuntime(limit, status, records)) {
        this->log_WARNING_HI_PERSISTENT_FAULT_HISTORY_UNAVAILABLE();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->log_ACTIVITY_HI_PERSISTENT_FAULT_HISTORY_STATUS(
        status.totalRecords, status.returnedRecords, status.activeCopy, status.generation);
    for (std::size_t i = 0; i < records.size(); i++) {
        const OBC::PersistentFaultRecord& record = records[i];
        this->log_ACTIVITY_HI_PERSISTENT_FAULT_HISTORY_RECORD(static_cast<U32>(i),
                                                              record.kind,
                                                              record.source,
                                                              record.level,
                                                              record.action,
                                                              record.resetCause,
                                                              record.bootCount,
                                                              record.consecutiveResetCount,
                                                              record.uptimeSec,
                                                              record.timestampSec,
                                                              record.detail,
                                                              record.flags);
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

std::string PersistentFaultManager::joinPath_(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    return base.back() == '/' ? base + child : base + "/" + child;
}

U32 PersistentFaultManager::clampLimit_(const U32 requested) {
    return requested == 0U ? OBC::PersistentFaultHistoryMaxReadback
                           : std::min(requested, OBC::PersistentFaultHistoryMaxReadback);
}

}  // namespace OBC
