#ifndef OBC_COMPONENTS_STORAGEHEALTHBRIDGE_HPP
#define OBC_COMPONENTS_STORAGEHEALTHBRIDGE_HPP

#include <string>

#include "OBC/Components/StorageHealthBridge/StorageHealthBridgeComponentAc.hpp"
#include "OBC/Components/StorageHealthBridge/StorageHealthTypes.hpp"
#include "OBC/Components/StorageHealthBridge/StorageScanner.hpp"

namespace OBC {

class StorageHealthBridge final : public StorageHealthBridgeComponentBase {
  public:
    explicit StorageHealthBridge(const char* const compName);

    ~StorageHealthBridge() override;

    void configureRuntime(const std::string& runtimeRoot,
                          const std::string& persistentRoot,
                          const std::string& stagingRoot,
                          U32 warningThresholdBytes = 0U,
                          U32 dataProductsQuotaBytes = 0U);

    bool scanNowForTest();
    void schedTickForTest(U32 context = 0U);

    bool getCachedStateForRuntime(OBC::STORAGE::HealthState& state) const;

  private:
    enum class ScanMode {
        CHANGE_DRIVEN,
        EXPLICIT_REFRESH,
    };

    void schedIn_handler(const FwIndexType portNum, U32 context) override;

    void STORAGE_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    bool scan_(ScanMode scanMode, bool emitScanUpdatedEvent);

    void publishChangeDrivenState_(const OBC::STORAGE::HealthState& previous, const OBC::STORAGE::HealthState& current);

    void publishExplicitRefreshState_(const OBC::STORAGE::HealthState& state);

    void emitRootEvents_();

    OBC::StorageRootKind rootKind_(OBC::STORAGE::StorageScanner::RootKind kind) const;

  private:
    OBC::STORAGE::StorageScanner m_scanner;
    OBC::STORAGE::HealthState m_cachedState;
    bool m_configured;
    U32 m_schedCount;
};

}  // namespace OBC

#endif
