#include "OBC/TopCcsds/OnboardStateSnapshotSource.hpp"

namespace OBCApp {

OnboardStateSnapshotSource::OnboardStateSnapshotSource()
    : m_modeManager(nullptr),
      m_epsBridge(nullptr),
      m_adcsBridge(nullptr),
      m_gpsBridge(nullptr),
      m_storageHealthBridge(nullptr),
      m_commController(nullptr),
      m_cspBridge(nullptr),
      m_radioController(nullptr),
      m_uartDriver(nullptr),
      m_bootManager(nullptr) {}

void OnboardStateSnapshotSource::configure(const OBC::ModeManager* modeManager,
                                           const OBC::EpsBridge* epsBridge,
                                           const OBC::AdcsBridge* adcsBridge,
                                           const OBC::GpsBridge* gpsBridge,
                                           const OBC::StorageHealthBridge* storageHealthBridge,
                                           const OBC::CommController* commController,
                                           const OBC::CspBridge* cspBridge,
                                           const OBC::RadioController* radioController,
                                           const OBC::UartDriver* uartDriver,
                                           const OBC::BootManager* bootManager) {
    this->m_modeManager = modeManager;
    this->m_epsBridge = epsBridge;
    this->m_adcsBridge = adcsBridge;
    this->m_gpsBridge = gpsBridge;
    this->m_storageHealthBridge = storageHealthBridge;
    this->m_commController = commController;
    this->m_cspBridge = cspBridge;
    this->m_radioController = radioController;
    this->m_uartDriver = uartDriver;
    this->m_bootManager = bootManager;
}

bool OnboardStateSnapshotSource::readStateSnapshot(OBC::StateData::StateSnapshot& snapshot) const {
    if (this->m_modeManager == nullptr || this->m_commController == nullptr || this->m_cspBridge == nullptr ||
        this->m_uartDriver == nullptr || this->m_bootManager == nullptr) {
        return false;
    }

    snapshot.mode = this->m_modeManager->getModeForRuntime();
    snapshot.rebootCount = static_cast<U16>(this->m_bootManager->getBootCountForRuntime());
    snapshot.uptimeSec = this->m_modeManager->getUptimeForRuntime();

    if (this->m_epsBridge != nullptr) {
        snapshot.haveEpsStatus = this->m_epsBridge->getCachedStatusForRuntime(snapshot.eps);
    }
    if (this->m_adcsBridge != nullptr) {
        snapshot.haveAdcsState = this->m_adcsBridge->getCachedStateForRuntime(snapshot.adcs);
    }
    if (this->m_gpsBridge != nullptr) {
        snapshot.haveGpsState = this->m_gpsBridge->getCachedStateForRuntime(snapshot.gps);
    }
    if (this->m_storageHealthBridge != nullptr) {
        snapshot.haveStorageHealth = this->m_storageHealthBridge->getCachedStateForRuntime(snapshot.storage);
    }

    const OBC::CommRuntimeState comm = this->m_commController->getStateForRuntime();
    snapshot.commActiveBand = comm.activeBand;
    snapshot.commPassActive = comm.passActive;
    snapshot.commPassRemainingSec = comm.passRemainingSec;
    snapshot.commTotalPasses = comm.totalPasses;

    const OBC::CspRuntimeCounters csp = this->m_cspBridge->getCountersForRuntime();
    snapshot.cspInitialized = csp.initialized;
    snapshot.cspLocalNodeId = csp.localNodeId;
    snapshot.cspTxPackets = csp.txPackets;
    snapshot.cspRxPackets = csp.rxPackets;
    snapshot.cspErrorCount = csp.errorCount;
    snapshot.cspFreeBuffers = csp.freeBuffers;

    const OBC::COMM::ByteStreamStats uart = this->m_uartDriver->getStatsForRuntime();
    snapshot.uartConnected = uart.connected;
    snapshot.uartTxBytes = uart.txBytes;
    snapshot.uartRxBytes = uart.rxBytes;
    snapshot.uartTxErrors = uart.txErrors;
    snapshot.uartRxErrors = uart.rxErrors;

    if (this->m_radioController != nullptr) {
        OBC::COMM::RadioStatus radio = {};
        snapshot.haveRadioStatus = this->m_radioController->getCachedStatusForRuntime(radio);
        if (snapshot.haveRadioStatus) {
            snapshot.radioEnabled = radio.enabled;
            snapshot.radioPowerDbm = radio.powerDbm;
            snapshot.radioFreqHz = radio.freqHz;
            snapshot.radioTemperatureC = radio.temperatureC;
            snapshot.radioRssiDbm = radio.rssiDbm;
        }
        const OBC::COMM::ByteStreamStats radioLink = this->m_radioController->getLinkStatsForRuntime();
        snapshot.radioLinkConnected = radioLink.connected;
        snapshot.radioTxBytes = radioLink.txBytes;
        snapshot.radioRxBytes = radioLink.rxBytes;
        snapshot.radioTxErrors = radioLink.txErrors;
        snapshot.radioRxErrors = radioLink.rxErrors;
    }

    const OBC::BootMetadata& metadata = this->m_bootManager->getMetadataForRuntime();
    snapshot.activeBootSlot = metadata.activeSlot;
    snapshot.pendingBootSlot = metadata.pendingSlot;
    snapshot.lastKnownGoodBootSlot = metadata.lastKnownGoodSlot;
    snapshot.bootConfirmed = metadata.confirmed;
    snapshot.bootStageVerified = metadata.stageVerified;
    snapshot.bootExpectedSize = metadata.expectedSize;
    snapshot.bootLastBootAttemptTime = metadata.lastBootAttemptTime;
    snapshot.bootLastErrorCode = metadata.lastErrorCode;
    snapshot.bootRemainingConfirmSeconds = this->m_bootManager->getRemainingConfirmSecondsForRuntime();
    snapshot.bootUpdateProgress = this->m_bootManager->getUpdateProgressForRuntime();
    snapshot.lastResetReason = static_cast<U8>(metadata.resetCause.e);
    return true;
}

}  // namespace OBCApp
