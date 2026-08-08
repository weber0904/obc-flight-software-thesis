#ifndef OBC_COMPONENTS_STORAGEHEALTHBRIDGE_STORAGEHEALTHTYPES_HPP
#define OBC_COMPONENTS_STORAGEHEALTHBRIDGE_STORAGEHEALTHTYPES_HPP

#include "Fw/Types/BasicTypes.hpp"

namespace OBC {
namespace STORAGE {

struct RootStats {
    bool exists = false;
    bool scanOk = false;
    U32 fileCount = 0U;
    U32 totalBytes = 0U;
    U32 errorCode = 0U;
    U32 quotaBytes = 0U;
    U32 watermarkBytes = 0U;
    U8 quotaStatus = 0U;
    U8 retentionStatus = 0U;
};

static constexpr U8 STORAGE_QUOTA_NOT_CONFIGURED = 0U;
static constexpr U8 STORAGE_QUOTA_OK = 1U;
static constexpr U8 STORAGE_QUOTA_OVER_QUOTA = 2U;
static constexpr U8 STORAGE_QUOTA_UNAVAILABLE = 3U;
static constexpr U8 STORAGE_RETENTION_OBSERVE_ONLY = 0U;

struct HealthState {
    bool hasScan = false;
    bool warningActive = false;
    U8 warningMask = 0U;
    U8 degradedMask = 0U;
    U32 scanCount = 0U;
    U32 scanErrorCount = 0U;
    RootStats persistent = {};
    RootStats staging = {};
    RootStats logs = {};
    RootStats dataProducts = {};
};

}  // namespace STORAGE
}  // namespace OBC

#endif
