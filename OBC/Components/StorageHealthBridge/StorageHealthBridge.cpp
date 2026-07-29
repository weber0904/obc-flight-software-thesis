#include "OBC/Components/StorageHealthBridge/StorageHealthBridge.hpp"

#include <cstdlib>

namespace OBC {

namespace {

U32 parseWarningThresholdEnv(const char* raw) {
    if (raw == nullptr || raw[0] == '\0') {
        return 0U;
    }
    return static_cast<U32>(std::strtoul(raw, nullptr, 10));
}

U32 parseQuotaEnv(const char* raw) {
    if (raw == nullptr || raw[0] == '\0') {
        return 0U;
    }
    return static_cast<U32>(std::strtoul(raw, nullptr, 10));
}

}  // namespace

StorageHealthBridge::StorageHealthBridge(const char* const compName)
    : StorageHealthBridgeComponentBase(compName), m_scanner(), m_cachedState(), m_configured(false), m_schedCount(0U) {}

StorageHealthBridge::~StorageHealthBridge() = default;

void StorageHealthBridge::configureRuntime(const std::string& runtimeRoot,
                                           const std::string& persistentRoot,
                                           const std::string& stagingRoot,
                                           U32 warningThresholdBytes,
                                           U32 dataProductsQuotaBytes) {
    if (warningThresholdBytes == 0U) {
        warningThresholdBytes = parseWarningThresholdEnv(std::getenv("OBC_STORAGE_WARNING_BYTES"));
    }
    if (dataProductsQuotaBytes == 0U) {
        dataProductsQuotaBytes = parseQuotaEnv(std::getenv("OBC_DATA_PRODUCTS_QUOTA_BYTES"));
    }
    this->m_scanner.configure(runtimeRoot, persistentRoot, stagingRoot, warningThresholdBytes, dataProductsQuotaBytes);
    this->m_configured = true;
}

bool StorageHealthBridge::scanNowForTest() {
    return this->scan_(ScanMode::EXPLICIT_REFRESH, true);
}

void StorageHealthBridge::schedTickForTest(U32 context) {
    this->schedIn_handler(0, context);
}

bool StorageHealthBridge::getCachedStateForRuntime(OBC::STORAGE::HealthState& state) const {
    state = this->m_cachedState;
    return this->m_cachedState.hasScan;
}

void StorageHealthBridge::schedIn_handler(const FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    this->m_schedCount++;
    this->tlmWrite_STORAGE_SCHED_TICKS(this->m_schedCount);
    if (this->m_schedCount == 1U || (this->m_schedCount % 4U) == 0U) {
        static_cast<void>(this->scan_(ScanMode::CHANGE_DRIVEN, false));
    }
}

void StorageHealthBridge::STORAGE_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(
        opCode,
        cmdSeq,
        this->scan_(ScanMode::EXPLICIT_REFRESH, true) ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR
    );
}

bool StorageHealthBridge::scan_(ScanMode scanMode, bool emitScanUpdatedEvent) {
    if (!this->m_configured) {
        return false;
    }

    const OBC::STORAGE::HealthState previousState = this->m_cachedState;
    OBC::STORAGE::HealthState nextState = {};
    const bool success = this->m_scanner.scan(nextState);
    nextState.scanCount = this->m_cachedState.scanCount + 1U;
    nextState.scanErrorCount += this->m_cachedState.scanErrorCount;
    this->m_cachedState = nextState;
    if (scanMode == ScanMode::EXPLICIT_REFRESH) {
        this->publishExplicitRefreshState_(this->m_cachedState);
    } else {
        this->publishChangeDrivenState_(previousState, this->m_cachedState);
    }
    this->emitRootEvents_();
    if (emitScanUpdatedEvent) {
        this->log_ACTIVITY_LO_STORAGE_SCAN_UPDATED(
            this->m_cachedState.warningActive ? 1U : 0U,
            this->m_cachedState.warningMask
        );
    }
    return success;
}

void StorageHealthBridge::publishChangeDrivenState_(const OBC::STORAGE::HealthState& previous,
                                                    const OBC::STORAGE::HealthState& current) {
    if (previous.warningActive != current.warningActive) {
        this->tlmWrite_STORAGE_WARNING_ACTIVE(current.warningActive ? 1U : 0U);
    }
    if (previous.dataProducts.quotaStatus != current.dataProducts.quotaStatus) {
        this->tlmWrite_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(current.dataProducts.quotaStatus);
    }
    if (previous.dataProducts.retentionStatus != current.dataProducts.retentionStatus) {
        this->tlmWrite_STORAGE_DATA_PRODUCTS_RETENTION_STATUS(current.dataProducts.retentionStatus);
    }
}

void StorageHealthBridge::publishExplicitRefreshState_(const OBC::STORAGE::HealthState& state) {
    this->tlmWrite_STORAGE_HAVE_SCAN(state.hasScan ? 1U : 0U);
    this->tlmWrite_STORAGE_WARNING_ACTIVE(state.warningActive ? 1U : 0U);
    this->tlmWrite_STORAGE_WARNING_MASK(state.warningMask);
    this->tlmWrite_STORAGE_DEGRADED_MASK(state.degradedMask);
    this->tlmWrite_STORAGE_SCAN_COUNT(state.scanCount);
    this->tlmWrite_STORAGE_SCAN_ERRORS(state.scanErrorCount);
    this->tlmWrite_STORAGE_PERSISTENT_FILE_COUNT(state.persistent.fileCount);
    this->tlmWrite_STORAGE_PERSISTENT_BYTES(state.persistent.totalBytes);
    this->tlmWrite_STORAGE_STAGING_FILE_COUNT(state.staging.fileCount);
    this->tlmWrite_STORAGE_STAGING_BYTES(state.staging.totalBytes);
    this->tlmWrite_STORAGE_LOG_FILE_COUNT(state.logs.fileCount);
    this->tlmWrite_STORAGE_LOG_BYTES(state.logs.totalBytes);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_EXISTS(state.dataProducts.exists ? 1U : 0U);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_SCAN_OK(state.dataProducts.scanOk ? 1U : 0U);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_FILE_COUNT(state.dataProducts.fileCount);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_BYTES(state.dataProducts.totalBytes);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_ERROR_CODE(state.dataProducts.errorCode);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_QUOTA_BYTES(state.dataProducts.quotaBytes);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_WATERMARK_BYTES(state.dataProducts.watermarkBytes);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_QUOTA_STATUS(state.dataProducts.quotaStatus);
    this->tlmWrite_STORAGE_DATA_PRODUCTS_RETENTION_STATUS(state.dataProducts.retentionStatus);
}

void StorageHealthBridge::emitRootEvents_() {
    const OBC::STORAGE::RootStats roots[] = {
        this->m_cachedState.persistent,
        this->m_cachedState.staging,
        this->m_cachedState.logs,
        this->m_cachedState.dataProducts,
    };

    for (U8 index = 0U; index < 4U; ++index) {
        const OBC::StorageRootKind root = this->rootKind_(static_cast<OBC::STORAGE::StorageScanner::RootKind>(index));
        if (!roots[index].exists) {
            this->log_WARNING_LO_STORAGE_ROOT_MISSING(root);
        } else if (!roots[index].scanOk) {
            const U32 errorCode = roots[index].errorCode == 0U ? 1U : roots[index].errorCode;
            this->log_WARNING_HI_STORAGE_SCAN_FAILED(root, errorCode);
        }
        if ((this->m_cachedState.warningMask & static_cast<U8>(1U << index)) != 0U) {
            this->log_WARNING_HI_STORAGE_WARNING_THRESHOLD_EXCEEDED(
                root, roots[index].totalBytes, this->m_scanner.getWarningThresholdBytes());
        }
    }
}

OBC::StorageRootKind StorageHealthBridge::rootKind_(OBC::STORAGE::StorageScanner::RootKind kind) const {
    switch (kind) {
        case OBC::STORAGE::StorageScanner::RootKind::PERSISTENT:
            return OBC::StorageRootKind::PERSISTENT;
        case OBC::STORAGE::StorageScanner::RootKind::STAGING:
            return OBC::StorageRootKind::STAGING;
        case OBC::STORAGE::StorageScanner::RootKind::LOGS:
            return OBC::StorageRootKind::LOGS;
        case OBC::STORAGE::StorageScanner::RootKind::DATA_PRODUCTS:
            return OBC::StorageRootKind::DATA_PRODUCTS;
        default:
            return OBC::StorageRootKind::PERSISTENT;
    }
}

}  // namespace OBC
