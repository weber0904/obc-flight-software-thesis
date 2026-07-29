#ifndef OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTRUNTIME_HPP
#define OBC_COMPONENTS_PERSISTENTFAULTMANAGER_PERSISTENTFAULTRUNTIME_HPP

#include <vector>

#include "Fw/Time/Time.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/PersistentFaultRecordKindEnumAc.hpp"
#include "OBC/Types/PersistentFaultStoreCopyEnumAc.hpp"
#include "OBC/Types/RecoveryActionEnumAc.hpp"
#include "OBC/Types/RecoveryIncidentSourceEnumAc.hpp"
#include "OBC/Types/RecoveryLevelEnumAc.hpp"
#include "OBC/Types/ResetCauseEnumAc.hpp"

namespace OBC {

constexpr U32 PersistentFaultStoreCapacity = 64U;
constexpr U32 PersistentFaultHistoryMaxReadback = 16U;
constexpr U32 PersistentFaultFlagBootSafeFallback = 1U << 0;

struct PersistentFaultRecord {
    OBC::PersistentFaultRecordKind kind = OBC::PersistentFaultRecordKind::BOOT_OBSERVED;
    OBC::RecoveryIncidentSource source = OBC::RecoveryIncidentSource::NONE;
    OBC::RecoveryLevel level = OBC::RecoveryLevel::R0_RECORD_ONLY;
    OBC::RecoveryAction action = OBC::RecoveryAction::NONE;
    OBC::ResetCause resetCause = OBC::ResetCause::UNKNOWN;
    U32 timestampSec = 0U;
    U32 uptimeSec = 0U;
    U32 bootCount = 0U;
    U32 consecutiveResetCount = 0U;
    U32 detail = 0U;
    U32 flags = 0U;
};

struct PersistentFaultHistoryStatus {
    U32 totalRecords = 0U;
    U32 returnedRecords = 0U;
    OBC::PersistentFaultStoreCopy activeCopy = OBC::PersistentFaultStoreCopy::NONE;
    U32 generation = 0U;
};

class IPersistentFaultRecorder {
  public:
    virtual ~IPersistentFaultRecorder() = default;

    virtual bool appendPersistentFaultRecordForRuntime(const PersistentFaultRecord& record) = 0;
};

class IPersistentFaultHistoryProvider {
  public:
    virtual ~IPersistentFaultHistoryProvider() = default;

    virtual bool getPersistentFaultHistoryForRuntime(U32 limit,
                                                     PersistentFaultHistoryStatus& status,
                                                     std::vector<PersistentFaultRecord>& records) const = 0;
};

inline bool persistentFaultTimeIsZero(const Fw::Time& time) {
    return time.getTimeBase() == TimeBase::TB_NONE && time.getSeconds() == 0U && time.getUSeconds() == 0U;
}

inline U32 persistentFaultTimestampSec(const Fw::Time& time) {
    return persistentFaultTimeIsZero(time) ? 0U : time.getSeconds();
}

inline const char* persistentFaultRecordKindName(const OBC::PersistentFaultRecordKind kind) {
    switch (kind.e) {
        case OBC::PersistentFaultRecordKind::BOOT_OBSERVED:
            return "BOOT_OBSERVED";
        case OBC::PersistentFaultRecordKind::RECOVERY_BOOT_ACK:
            return "RECOVERY_BOOT_ACK";
        case OBC::PersistentFaultRecordKind::INCIDENT_OPENED:
            return "INCIDENT_OPENED";
        case OBC::PersistentFaultRecordKind::ACTION_REQUESTED:
            return "ACTION_REQUESTED";
        case OBC::PersistentFaultRecordKind::ACTION_EXECUTED:
            return "ACTION_EXECUTED";
        case OBC::PersistentFaultRecordKind::REBOOT_PENDING:
            return "REBOOT_PENDING";
        case OBC::PersistentFaultRecordKind::REBOOT_ISSUED:
            return "REBOOT_ISSUED";
        case OBC::PersistentFaultRecordKind::INCIDENT_CLEARED:
            return "INCIDENT_CLEARED";
        default:
            return "UNKNOWN";
    }
}

inline const char* persistentFaultStoreCopyName(const OBC::PersistentFaultStoreCopy copy) {
    switch (copy.e) {
        case OBC::PersistentFaultStoreCopy::COPY_A:
            return "COPY_A";
        case OBC::PersistentFaultStoreCopy::COPY_B:
            return "COPY_B";
        default:
            return "NONE";
    }
}

}  // namespace OBC

#endif

