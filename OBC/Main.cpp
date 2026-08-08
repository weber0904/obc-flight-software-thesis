#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "OBC/Components/CommandIngressAuthority/CommandAuthKeystore.hpp"
#include "OBC/Runtime/HostedRuntime.hpp"
#include "OBC/TopCcsds/AppTopologyAc.hpp"
#include "OBC/TopCcsds/OBCAppTopology.hpp"
#include "Os/Os.hpp"
#include "simulators/comm/CommCspProtocol.hpp"

namespace {

class DefaultRuntimeServices final : public OBC::Runtime::RuntimeServices {
  public:
    void announceBoot() override { OBCApp::modeManager.announceBoot(); }
    void setHealthEnabled(bool enabled) override { OBCApp::watchdogSupervisor.setEnabledForRuntime(enabled); }
    Fw::CmdResponse setHealthThreshold(OBC::HealthItem item, float value) override {
        return OBCApp::watchdogSupervisor.setThresholdForRuntime(item, value);
    }
    void configureCommandIngressPersistenceRoot(const std::string& persistentRoot) override {
        static_cast<void>(OBCApp::commandIngressAuthority.configurePersistentRootForRuntime(persistentRoot));
    }
    void configureBootStorageRoots(const std::string& persistentRoot, const std::string& stagingRoot) override {
        OBCApp::bootManager.configureRuntimeStorageRoots(persistentRoot, stagingRoot);
    }
    void configurePersistentFaultStorageRoot(const std::string& persistentRoot) override {
        OBCApp::persistentFaultManager.configurePersistentRootForRuntime(persistentRoot);
    }
    void configureBootTrust(const OBC::BootTrustConfig& config) override {
        OBCApp::bootManager.configureBootTrustForRuntime(config);
    }
    void configureStorageRuntime(const std::string& runtimeRoot,
                                 const std::string& persistentRoot,
                                 const std::string& stagingRoot) override {
        OBCApp::storageHealthBridge.configureRuntime(runtimeRoot, persistentRoot, stagingRoot);
    }
    void initCspForRuntime(U8 nodeId) override {
        static_cast<void>(OBCApp::cspBridge.initForRuntime(nodeId));
        if (!OBCApp::startPayloadCspService()) {
            std::cerr << "Failed to start payload CSP service\n";
        }
    }
    void shutdownCspForRuntime() override {
        OBCApp::stopPayloadCspService();
        OBCApp::cspBridge.shutdownForRuntime();
    }
    void configureCommTransport(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link,
                                std::unique_ptr<OBC::COMM::IRadioTransport> radioTransport) override {
        OBCApp::radioController.configureTransport(std::move(radioTransport));
        OBCApp::uartDriver.configureTransport(link);
    }
    bool startGroundLink() override {
        const bool primaryStarted = OBCApp::groundLinkDriver.start();
        const bool uhfStarted = OBCApp::uhfGroundLinkDriver.start();
        return primaryStarted && uhfStarted;
    }
    void stopGroundLink() override {
        OBCApp::groundLinkDriver.stop();
        OBCApp::uhfGroundLinkDriver.stop();
    }
    void joinGroundLink() override {
        OBCApp::groundLinkDriver.join();
        OBCApp::uhfGroundLinkDriver.join();
    }
    void startRateGroups(Fw::TimeInterval interval) override { OBCApp::startRateGroups(interval); }
    void stopRateGroups() override { OBCApp::stopRateGroups(); }

    bool refreshEpsStatus(OBC::EPS::StatusData& status) override { return OBCApp::epsBridge.getStatusForRuntime(status); }
    bool getCachedEpsStatus(OBC::EPS::StatusData& status) override {
        return OBCApp::epsBridge.getCachedStatusForRuntime(status);
    }
    bool refreshAdcsState(OBC::ADCS::StateData& state) override { return OBCApp::adcsBridge.getStateForRuntime(state); }
    bool getCachedAdcsState(OBC::ADCS::StateData& state) override {
        return OBCApp::adcsBridge.getCachedStateForRuntime(state);
    }
    bool getAdcsPollHealth(OBC::ADCS::PollHealthState& state) override {
        return OBCApp::adcsBridge.getPollHealthForRuntime(state);
    }
    bool getGpsCachedState(OBC::GPS::StateData& state) override {
        return OBCApp::gpsBridge.getCachedStateForRuntime(state);
    }
    bool getRadioStatus(OBC::COMM::RadioStatus& status) override {
        return OBCApp::radioController.getStatusForRuntime(status);
    }
    OBC::RadioObservationState getRadioObservation() const override {
        return OBCApp::radioController.getObservationForRuntime();
    }
    bool getStorageHealth(OBC::STORAGE::HealthState& state) override {
        return OBCApp::storageHealthBridge.getCachedStateForRuntime(state);
    }
    void updateResourceSample(float cpuPct, float rssMb) override {
        OBCApp::watchdogSupervisor.updateResourceSample(cpuPct, rssMb);
    }

    OBC::SatMode getMode() const override { return OBCApp::modeManager.getModeForRuntime(); }
    U32 getUptime() const override { return OBCApp::modeManager.getUptimeForRuntime(); }
    U32 getRebootCount() const override { return OBCApp::bootManager.getBootCountForRuntime(); }
    OBC::CommRuntimeState getCommState() const override { return OBCApp::commController.getStateForRuntime(); }
    OBC::CspRuntimeCounters getCspCounters() const override { return OBCApp::cspBridge.getCountersForRuntime(); }
    OBC::WatchdogRuntimeSnapshot getWatchdogStatus() const override {
        return OBCApp::watchdogSupervisor.getStatusForRuntime();
    }
    OBC::LinuxWatchdogRuntimeStatus getHardwareWatchdogStatus() const override {
        return OBCApp::linuxWatchdogSink.getStatusForRuntime();
    }
    OBC::RecoveryRuntimeStatus getRecoveryStatus() const override {
        return OBCApp::recoveryExecutor.getStatusForRuntime();
    }
    bool getPersistentFaultHistory(
        U32 limit, OBC::PersistentFaultHistoryStatus& status, std::vector<OBC::PersistentFaultRecord>& records) const override {
        return OBCApp::persistentFaultManager.getPersistentFaultHistoryForRuntime(limit, status, records);
    }
    OBC::TtcPassRuntimeStatus getTtcPassStatus() const override {
        return OBCApp::ttcPassManager.getStatusForRuntime();
    }
    const OBC::BootMetadata& getBootMetadata() const override {
        return OBCApp::bootManager.getMetadataForRuntime();
    }
    U8 getBootUpdateProgress() const override { return OBCApp::bootManager.getUpdateProgressForRuntime(); }
    U32 getBootRemainingConfirmSeconds() const override {
        return OBCApp::bootManager.getRemainingConfirmSecondsForRuntime();
    }
    OBC::COMM::ByteStreamStats getUartStats() const override { return OBCApp::uartDriver.getStatsForRuntime(); }
    OBC::COMM::ByteStreamStats getRadioLinkStats() const override {
        return OBCApp::radioController.getLinkStatsForRuntime();
    }
    OBC::COMM::GroundLinkStats getGroundLinkStats() const override {
        return OBCApp::groundLinkDriver.getStatsForRuntime();
    }
    OBC::COMM::GroundLinkObservationState getGroundLinkObservation(OBC::CommBand band) const override {
        return band.e == OBC::CommBand::UHF ? OBCApp::uhfGroundLinkDriver.getObservationForRuntime()
                                            : OBCApp::groundLinkDriver.getObservationForRuntime();
    }

    Fw::CmdResponse setMode(OBC::SatMode mode) override { return OBCApp::modeManager.requestModeFromOperator(mode); }
    Fw::CmdResponse pingCsp(U8 node, U32 timeoutMs, bool& success) override {
        return OBCApp::cspBridge.pingForRuntime(node, timeoutMs, success);
    }
    Fw::CmdResponse setEpsPdu(U8 channel, bool enabled, OBC::EPS::StatusData& status) override {
        return OBCApp::epsBridge.setPduForRuntime(channel, enabled, status);
    }
    Fw::CmdResponse setEpsHeater(bool enabled, OBC::EPS::StatusData& status) override {
        return OBCApp::epsBridge.setHeaterForRuntime(enabled, status);
    }
    Fw::CmdResponse setAdcsMode(OBC::AdcsMode mode, OBC::ADCS::StateData& state) override {
        return OBCApp::adcsBridge.setModeForRuntime(mode, state);
    }
    bool pollGpsState() override { return OBCApp::gpsBridge.pollStateForTest(); }
    Fw::CmdResponse setGpsSourceMode(OBC::GpsSourceMode mode) override {
        return OBCApp::gpsBridge.setSourceModeForRuntime(mode);
    }
    bool scanStorageHealth() override { return OBCApp::storageHealthBridge.scanNowForTest(); }
    Fw::CmdResponse setCommBand(OBC::CommBand band) override {
        return OBCApp::commController.setActiveBandForRuntime(band);
    }
    Fw::CmdResponse startCommPass(U32 durationSeconds) override {
        return OBCApp::commController.startPassForRuntime(durationSeconds);
    }
    Fw::CmdResponse stopCommPass() override { return OBCApp::commController.stopPassForRuntime(); }
    Fw::CmdResponse setTtcPolicy(bool enabled, U32 lossOfLockTimeoutSec) override {
        return OBCApp::ttcPassManager.setPolicyForRuntime(enabled, lossOfLockTimeoutSec);
    }
    Fw::CmdResponse setTtcPassWindow(U64 startUnixSec, U64 endUnixSec) override {
        return OBCApp::ttcPassManager.setPassWindowForRuntime(startUnixSec, endUnixSec);
    }
    Fw::CmdResponse clearTtcPassWindow() override {
        OBCApp::ttcPassManager.clearPassWindowForRuntime();
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setWatchdogConfig(OBC::WatchdogSource source,
                                      bool enabled,
                                      U32 warningTicks,
                                      U32 safeTicks,
                                      U32 suppressTicks) override {
        return OBCApp::watchdogSupervisor.setConfigForRuntime(source, enabled, warningTicks, safeTicks, suppressTicks);
    }
    bool setWatchdogProbeSuppression(OBC::WatchdogSource source, bool suppressed) override {
        return OBCApp::watchdogSupervisor.setProbeBeatSuppressionForRuntime(source, suppressed);
    }
    Fw::CmdResponse enableRadio(bool enabled, OBC::COMM::RadioStatus& status) override {
        return OBCApp::radioController.enableForRuntime(enabled, status);
    }
    Fw::CmdResponse setRadioPower(U8 powerDbm, OBC::COMM::RadioStatus& status) override {
        return OBCApp::radioController.setPowerForRuntime(powerDbm, status);
    }
    Fw::CmdResponse setRadioFrequency(U32 freqHz, OBC::COMM::RadioStatus& status) override {
        return OBCApp::radioController.setFrequencyForRuntime(freqHz, status);
    }
    bool exchangeUart(const std::string& request, std::string& response) override {
        return OBCApp::uartDriver.exchangeForRuntime(request, response);
    }
    bool exchangeDelimitedUart(const std::string& request, std::string& response, char delimiter) override {
        return OBCApp::uartDriver.exchangeDelimitedForRuntime(request, response, delimiter);
    }
    Fw::CmdResponse bootPrepareUpdate(U32 imageSize, const std::string& digest) override {
        return OBCApp::bootManager.prepareUpdateForRuntime(imageSize, digest);
    }
    Fw::CmdResponse bootVerifyStagedImage(const std::string& path) override {
        return OBCApp::bootManager.verifyStagedImageForRuntime(path);
    }
    Fw::CmdResponse bootActivateStagedImage() override {
        return OBCApp::bootManager.activateStagedImageForRuntime();
    }
    Fw::CmdResponse bootConfirm() override { return OBCApp::bootManager.confirmForRuntime(); }
    Fw::CmdResponse bootRollback() override { return OBCApp::bootManager.rollbackForRuntime(); }
    OBC::RecoveryExitRequest consumeRecoveryExitRequest() override {
        return OBCApp::recoveryExecutor.consumeRecoveryExitRequestForRuntime();
    }
};

}  // namespace

int main(int argc, char* argv[]) {
    OBC::Runtime::RuntimeConfig config;
    if (!OBC::Runtime::parseArgs(argc, argv, config, std::cout, std::cerr)) {
        return 1;
    }

    Os::init();

    OBCApp::TopologyState topologyState = {};
    topologyState.groundLinkMode = OBC::Runtime::effectiveGroundLinkMode(config);
    topologyState.groundLinkHost = config.gdsPort == 0U ? nullptr : config.gdsHost.c_str();
    topologyState.groundLinkPort = config.gdsPort;
    topologyState.commCspNode = config.commCspNode;
    topologyState.groundLinkHealthSemantics = OBC::Runtime::groundLinkHealthSemanticsForCommCspNode(config.commCspNode);
    topologyState.enablePrimaryGroundLinkDriver = config.enablePrimaryGroundLinkDriver;
    topologyState.sbandSubsystemHealthNode =
        config.enableCommSubsystemHealthDetector
            ? (config.enablePrimaryGroundLinkDriver ? OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID : 0U)
            : ((topologyState.groundLinkMode == OBC::COMM::GroundLinkBackendMode::COMM_CSP &&
                config.commCspNode == OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID)
                   ? OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID
                   : 0U);
    topologyState.uhfSubsystemHealthNode =
        config.enableCommSubsystemHealthDetector ? OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID
                                                 : ((topologyState.groundLinkMode == OBC::COMM::GroundLinkBackendMode::COMM_CSP &&
                                                     config.commCspNode == OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID)
                                                        ? OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID
                                                        : 0U);
    topologyState.commSubsystemFdirConfig.pingTimeoutMs = config.commSubsystemPingTimeoutMs;
    topologyState.commSubsystemFdirConfig.unavailableFailureThreshold =
        (topologyState.groundLinkMode == OBC::COMM::GroundLinkBackendMode::DISABLED &&
         topologyState.sbandSubsystemHealthNode == 0U &&
         topologyState.uhfSubsystemHealthNode == 0U)
            ? 0U
            : config.commPrimaryUnavailableFailureThreshold;
    topologyState.commSubsystemFdirConfig.useSubsystemResponsiveness = config.enableCommSubsystemHealthDetector;
    topologyState.initialCommBand = config.initialCommBand;
    topologyState.runtimeRoot = config.runtimeRoot.c_str();
    topologyState.hardwareWatchdogEnabled = config.hardwareWatchdogMode == "linux-device";
    topologyState.hardwareWatchdogDevice = config.hardwareWatchdogDevice.c_str();
    topologyState.hardwareWatchdogTimeoutSec = config.hardwareWatchdogTimeoutSec;
    topologyState.diagnosticQuietPacketEgress = config.diagnosticQuietPacketEgress;
    topologyState.beaconBroadcastEnabled = config.radioProtocol == "transparent-passive";
    topologyState.uhfBeaconSideChannelEnabled = config.uhfBeaconCspNode != 0U;
    topologyState.uhfBeaconCspNode = config.uhfBeaconCspNode;
    OBC::CommandAuthKeystore commandAuthKeystore = {};
    std::string commandAuthKeystoreError;
    const std::string commandAuthKeystorePath = OBC::defaultCommandAuthKeystorePath();
    if (!OBC::loadCommandAuthKeystore(commandAuthKeystorePath, commandAuthKeystore, commandAuthKeystoreError)) {
        std::cerr << "Failed to load command auth keystore from " << commandAuthKeystorePath << ": "
                  << commandAuthKeystoreError << "\n";
        return 1;
    }
    topologyState.commandAuthModuleSerial = commandAuthKeystore.moduleSerial.c_str();
    topologyState.sbandSecureAuthKeyBytes = commandAuthKeystore.sband.keyBytes;
    topologyState.sbandSecureAuthKeyLength = commandAuthKeystore.sband.keyLength;
    topologyState.uhfSecureAuthKeyBytes = commandAuthKeystore.uhf.keyBytes;
    topologyState.uhfSecureAuthKeyLength = commandAuthKeystore.uhf.keyLength;
    topologyState.sbandCommandAuthorityConfig =
        OBC::authorityConfigFromProfile(config.commandAuthorityProfile.c_str());
    topologyState.uhfCommandAuthorityConfig = OBC::authorityConfigFromProfile("uhf-backup");
    if (!OBCApp::setupTopology(topologyState)) {
        std::cerr << "OBC topology setup failed\n";
        return 1;
    }

    DefaultRuntimeServices services;
    OBC::Runtime::StartupBanner banner;
    banner.runtimeStartedLine = "OBC CCSDS S-band runtime started. Type 'help' for commands.";
    const int result = OBC::Runtime::runHostedRuntime(config, services, banner, std::cout, std::cerr);
    OBCApp::teardownTopology(topologyState);
    return result;
}
