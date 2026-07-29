#ifndef OBC_COMPONENTS_STORAGEHEALTHBRIDGE_STORAGESCANNER_HPP
#define OBC_COMPONENTS_STORAGEHEALTHBRIDGE_STORAGESCANNER_HPP

#include <string>

#include "OBC/Components/StorageHealthBridge/StorageHealthTypes.hpp"

namespace OBC {
namespace STORAGE {

class StorageScanner final {
  public:
    enum class RootKind : U8 {
        PERSISTENT = 0U,
        STAGING = 1U,
        LOGS = 2U,
        DATA_PRODUCTS = 3U,
    };

    StorageScanner();

    void configure(const std::string& runtimeRoot,
                   const std::string& persistentRoot,
                   const std::string& stagingRoot,
                   U32 warningThresholdBytes,
                   U32 dataProductsQuotaBytes = 0U);

    bool scan(HealthState& state) const;

    const std::string& getPersistentRoot() const;
    const std::string& getStagingRoot() const;
    const std::string& getLogsRoot() const;
    const std::string& getDataProductsRoot() const;
    U32 getWarningThresholdBytes() const;
    U32 getDataProductsQuotaBytes() const;

  private:
    static bool isMissingErrno_(int err);
    static void setErrorCodeIfUnset_(RootStats& stats, int err);
    static std::string joinPath_(const std::string& base, const char* child);
    static bool scanRoot_(const std::string& path, RootStats& stats, bool& indexExists);
    static bool scanDirectoryRecursive_(const std::string& path, RootStats& stats);
    static void applyPolicy_(RootStats& stats, U32 quotaBytes, U32 watermarkBytes);
    static void evaluateWarnings_(const HealthState& state, U32 warningThresholdBytes, bool& warningActive, U8& warningMask);
    static void evaluateDegraded_(const HealthState& state, U8& degradedMask);

  private:
    std::string m_runtimeRoot;
    std::string m_persistentRoot;
    std::string m_stagingRoot;
    std::string m_logsRoot;
    std::string m_dataProductsRoot;
    U32 m_warningThresholdBytes;
    U32 m_dataProductsQuotaBytes;
};

}  // namespace STORAGE
}  // namespace OBC

#endif
