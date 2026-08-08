#include "OBC/Runtime/HostedRuntime.hpp"

#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <utility>
#include <unistd.h>

#include "simulators/comm/TransparentLinkFraming.hpp"

namespace {

class FakeRuntimeServices final : public OBC::Runtime::RuntimeServices {
  public:
    FakeRuntimeServices() {
        for (FwIndexType index = 0; index < static_cast<FwIndexType>(this->watchdog.sources.size()); ++index) {
            this->watchdog.sources[static_cast<std::size_t>(index)].source = OBC::watchdogSourceFromIndex(index);
        }
        this->radioObservation.haveSample = true;
        this->radioObservation.lastResult = OBC::RadioObservationResult::OK;
        this->radioObservation.lastStatus = this->radio;
        this->sbandGroundObservation.mode = OBC::COMM::GroundLinkBackendMode::DIRECT_TCP;
        this->sbandGroundObservation.healthSemantics = OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK;
    }

    void announceBoot() override { ++this->announceBootCalls; }
    void setHealthEnabled(bool enabled) override { this->healthEnabled = enabled; }
    Fw::CmdResponse setHealthThreshold(OBC::HealthItem item, float value) override {
        this->lastHealthItem = item;
        this->lastHealthThreshold = value;
        ++this->setHealthThresholdCalls;
        return Fw::CmdResponse::OK;
    }
    void configureCommandIngressPersistenceRoot(const std::string&) override {
        this->callOrder.push_back("command-ingress-persistence");
    }
    void configureBootStorageRoots(const std::string&, const std::string&) override {
        this->callOrder.push_back("boot-storage");
    }
    void configurePersistentFaultStorageRoot(const std::string&) override {
        this->callOrder.push_back("persistent-fault-storage");
    }
    void configureBootTrust(const OBC::BootTrustConfig& config) override {
        this->bootTrustConfig = config;
        ++this->configureBootTrustCalls;
        this->callOrder.push_back("boot-trust");
    }
    void configureStorageRuntime(const std::string&, const std::string&, const std::string&) override {
        this->callOrder.push_back("storage-runtime");
    }
    void initCspForRuntime(U8 nodeId) override {
        this->cspNode = nodeId;
        this->callOrder.push_back("init-csp");
    }
    void shutdownCspForRuntime() override { ++this->shutdownCalls; }
    void configureCommTransport(const std::shared_ptr<OBC::COMM::IByteStreamTransport>&,
                                std::unique_ptr<OBC::COMM::IRadioTransport>) override {
        ++this->configureCommCalls;
    }
    bool startGroundLink() override {
        ++this->startGroundLinkCalls;
        return true;
    }
    void stopGroundLink() override { ++this->stopGroundLinkCalls; }
    void joinGroundLink() override { ++this->joinGroundLinkCalls; }
    void startRateGroups(Fw::TimeInterval) override {}
    void stopRateGroups() override { ++this->stopRateGroupCalls; }

    bool refreshEpsStatus(OBC::EPS::StatusData& status) override {
        ++this->refreshEpsStatusCalls;
        status = this->eps;
        return this->haveEps;
    }
    bool getCachedEpsStatus(OBC::EPS::StatusData& status) override {
        ++this->getCachedEpsStatusCalls;
        status = this->eps;
        return this->haveEps;
    }
    bool refreshAdcsState(OBC::ADCS::StateData& state) override {
        ++this->refreshAdcsStateCalls;
        state = this->adcs;
        return this->haveAdcs;
    }
    bool getCachedAdcsState(OBC::ADCS::StateData& state) override {
        ++this->getCachedAdcsStateCalls;
        state = this->adcs;
        return this->haveAdcs;
    }
    bool getAdcsPollHealth(OBC::ADCS::PollHealthState& state) override {
        state = this->adcsPollHealth;
        return this->haveAdcsPollHealth;
    }
    bool getGpsCachedState(OBC::GPS::StateData& state) override {
        state = this->gps;
        return this->haveGps;
    }
    bool getRadioStatus(OBC::COMM::RadioStatus& status) override {
        status = this->radio;
        return this->haveRadio;
    }
    OBC::RadioObservationState getRadioObservation() const override { return this->radioObservation; }
    bool getStorageHealth(OBC::STORAGE::HealthState& state) override {
        state = this->storage;
        return this->haveStorage;
    }
    void updateResourceSample(float, float) override { ++this->healthSamples; }

    OBC::SatMode getMode() const override { return this->mode; }
    U32 getUptime() const override { return 7U; }
    U32 getRebootCount() const override { return 2U; }
    OBC::CommRuntimeState getCommState() const override { return this->comm; }
    OBC::CspRuntimeCounters getCspCounters() const override { return this->csp; }
    OBC::WatchdogRuntimeSnapshot getWatchdogStatus() const override { return this->watchdog; }
    OBC::LinuxWatchdogRuntimeStatus getHardwareWatchdogStatus() const override { return this->hardwareWatchdog; }
    OBC::RecoveryRuntimeStatus getRecoveryStatus() const override { return this->recovery; }
    bool getPersistentFaultHistory(
        U32 limit, OBC::PersistentFaultHistoryStatus& status, std::vector<OBC::PersistentFaultRecord>& records) const override {
        this->lastPersistentFaultLimit = limit;
        ++this->persistentFaultHistoryCalls;
        status = this->persistentFaultStatus;
        records = this->persistentFaultRecords;
        return this->persistentFaultHistoryAvailable;
    }
    OBC::TtcPassRuntimeStatus getTtcPassStatus() const override { return this->ttc; }
    const OBC::BootMetadata& getBootMetadata() const override { return this->boot; }
    U8 getBootUpdateProgress() const override { return 10U; }
    U32 getBootRemainingConfirmSeconds() const override { return 60U; }
    OBC::COMM::ByteStreamStats getUartStats() const override { return this->uartStats; }
    OBC::COMM::ByteStreamStats getRadioLinkStats() const override { return this->radioStats; }
    OBC::COMM::GroundLinkStats getGroundLinkStats() const override { return this->groundStats; }
    OBC::COMM::GroundLinkObservationState getGroundLinkObservation(OBC::CommBand band) const override {
        return band.e == OBC::CommBand::UHF ? this->uhfGroundObservation : this->sbandGroundObservation;
    }
    OBC::RecoveryExitRequest consumeRecoveryExitRequest() override {
        const OBC::RecoveryExitRequest request = this->recoveryExitRequest;
        this->recoveryExitRequest = OBC::RecoveryExitRequest::NONE;
        return request;
    }

    Fw::CmdResponse setMode(OBC::SatMode nextMode) override {
        ++this->setModeCalls;
        if (this->modeResponse == Fw::CmdResponse::OK) {
            this->mode = nextMode;
        }
        return this->modeResponse;
    }
    Fw::CmdResponse pingCsp(U8 node, U32 timeoutMs, bool& success) override {
        this->lastPingNode = node;
        this->lastPingTimeout = timeoutMs;
        success = true;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setEpsPdu(U8 channel, bool enabled, OBC::EPS::StatusData& status) override {
        this->lastPduChannel = channel;
        this->lastPduEnabled = enabled;
        ++this->setPduCalls;
        status = this->eps;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setEpsHeater(bool enabled, OBC::EPS::StatusData& status) override {
        this->lastHeaterEnabled = enabled;
        ++this->setHeaterCalls;
        status = this->eps;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setAdcsMode(OBC::AdcsMode nextMode, OBC::ADCS::StateData& state) override {
        this->lastAdcsMode = nextMode;
        ++this->setAdcsModeCalls;
        state = this->adcs;
        return Fw::CmdResponse::OK;
    }
    bool pollGpsState() override {
        ++this->gpsPollCalls;
        return true;
    }
    Fw::CmdResponse setGpsSourceMode(OBC::GpsSourceMode nextMode) override {
        this->lastGpsMode = nextMode;
        ++this->setGpsSourceCalls;
        return Fw::CmdResponse::OK;
    }
    bool scanStorageHealth() override {
        ++this->storageScanCalls;
        return true;
    }
    Fw::CmdResponse setCommBand(OBC::CommBand band) override {
        this->lastBand = band;
        ++this->setBandCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse startCommPass(U32 durationSeconds) override {
        this->lastPassDuration = durationSeconds;
        ++this->startPassCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse stopCommPass() override {
        ++this->stopPassCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setTtcPolicy(bool enabled, U32 lossOfLockTimeoutSec) override {
        this->ttc.enabled = enabled;
        this->ttc.lossOfLockTimeoutSec = lossOfLockTimeoutSec;
        ++this->setTtcPolicyCalls;
        return this->ttcPolicyResponse;
    }
    Fw::CmdResponse setTtcPassWindow(U64 startUnixSec, U64 endUnixSec) override {
        this->ttc.windowConfigured = true;
        this->ttc.windowStartUnixSec = startUnixSec;
        this->ttc.windowEndUnixSec = endUnixSec;
        ++this->setTtcWindowCalls;
        return this->ttcWindowResponse;
    }
    Fw::CmdResponse clearTtcPassWindow() override {
        this->ttc.windowConfigured = false;
        this->ttc.windowStartUnixSec = 0U;
        this->ttc.windowEndUnixSec = 0U;
        ++this->clearTtcWindowCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setWatchdogConfig(OBC::WatchdogSource source,
                                      bool enabled,
                                      U32 warningTicks,
                                      U32 safeTicks,
                                      U32 suppressTicks) override {
        this->lastWatchdogSource = source;
        this->lastWatchdogEnabled = enabled;
        this->lastWatchdogWarningTicks = warningTicks;
        this->lastWatchdogSafeTicks = safeTicks;
        this->lastWatchdogSuppressTicks = suppressTicks;
        ++this->setWatchdogConfigCalls;
        auto& snapshot = this->watchdog.sources[OBC::watchdogSourceIndex(source)];
        snapshot.config.enabled = enabled;
        snapshot.config.warningTicks = warningTicks;
        snapshot.config.safeTicks = safeTicks;
        snapshot.config.suppressTicks = suppressTicks;
        return Fw::CmdResponse::OK;
    }
    bool setWatchdogProbeSuppression(OBC::WatchdogSource source, bool suppressed) override {
        this->lastWatchdogSource = source;
        this->lastWatchdogProbeSuppressed = suppressed;
        ++this->setWatchdogProbeSuppressionCalls;
        this->watchdog.sources[OBC::watchdogSourceIndex(source)].probeSuppressed = suppressed;
        return true;
    }
    Fw::CmdResponse enableRadio(bool enabled, OBC::COMM::RadioStatus& status) override {
        this->lastRadioEnabled = enabled;
        ++this->enableRadioCalls;
        this->radio.enabled = enabled;
        status = this->radio;
        this->radioObservation.haveSample = true;
        this->radioObservation.statusAgeTicks = 0U;
        this->radioObservation.lastResult = OBC::RadioObservationResult::OK;
        this->radioObservation.lastStatus = this->radio;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setRadioPower(U8 powerDbm, OBC::COMM::RadioStatus& status) override {
        this->lastRadioPower = powerDbm;
        ++this->setRadioPowerCalls;
        this->radio.powerDbm = powerDbm;
        status = this->radio;
        this->radioObservation.haveSample = true;
        this->radioObservation.statusAgeTicks = 0U;
        this->radioObservation.lastResult = OBC::RadioObservationResult::OK;
        this->radioObservation.lastStatus = this->radio;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse setRadioFrequency(U32 freqHz, OBC::COMM::RadioStatus& status) override {
        this->lastRadioFreq = freqHz;
        ++this->setRadioFreqCalls;
        this->radio.freqHz = freqHz;
        status = this->radio;
        this->radioObservation.haveSample = true;
        this->radioObservation.statusAgeTicks = 0U;
        this->radioObservation.lastResult = OBC::RadioObservationResult::OK;
        this->radioObservation.lastStatus = this->radio;
        return Fw::CmdResponse::OK;
    }
    bool exchangeUart(const std::string& request, std::string& response) override {
        this->lastUartRequest = request;
        response = "PONG";
        return true;
    }
    bool exchangeDelimitedUart(const std::string& request, std::string& response, char delimiter) override {
        this->lastDelimitedRequest = request;
        this->lastDelimiter = delimiter;
        response = OBC::COMM::encodeTransparentLinkFrame("ACK");
        if (!response.empty() && response.back() == delimiter) {
            response.pop_back();
        }
        return true;
    }
    Fw::CmdResponse bootPrepareUpdate(U32 imageSize, const std::string& digest) override {
        this->lastImageSize = imageSize;
        this->lastDigest = digest;
        ++this->bootPrepareCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse bootVerifyStagedImage(const std::string& path) override {
        this->lastVerifyPath = path;
        ++this->bootVerifyCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse bootActivateStagedImage() override {
        ++this->bootActivateCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse bootConfirm() override {
        ++this->bootConfirmCalls;
        return Fw::CmdResponse::OK;
    }
    Fw::CmdResponse bootRollback() override {
        ++this->bootRollbackCalls;
        return Fw::CmdResponse::OK;
    }

    bool haveEps = true;
    bool haveAdcs = true;
    bool haveAdcsPollHealth = true;
    bool haveGps = true;
    bool haveRadio = true;
    bool haveStorage = true;
    OBC::EPS::StatusData eps = {};
    OBC::ADCS::StateData adcs = {};
    OBC::ADCS::PollHealthState adcsPollHealth = {};
    OBC::GPS::StateData gps = {};
    OBC::COMM::RadioStatus radio = {};
    OBC::RadioObservationState radioObservation = {};
    OBC::STORAGE::HealthState storage = {};
    OBC::SatMode mode = OBC::SatMode::SAFE;
    Fw::CmdResponse modeResponse = Fw::CmdResponse::OK;
    OBC::CommRuntimeState comm = {};
    OBC::CspRuntimeCounters csp = {};
    OBC::WatchdogRuntimeSnapshot watchdog = {};
    OBC::LinuxWatchdogRuntimeStatus hardwareWatchdog = {};
    OBC::RecoveryRuntimeStatus recovery = {};
    mutable OBC::PersistentFaultHistoryStatus persistentFaultStatus = {};
    mutable std::vector<OBC::PersistentFaultRecord> persistentFaultRecords = {};
    OBC::TtcPassRuntimeStatus ttc = {};
    OBC::BootMetadata boot = {};
    OBC::BootTrustConfig bootTrustConfig = {};
    OBC::COMM::ByteStreamStats uartStats = {};
    OBC::COMM::ByteStreamStats radioStats = {};
    OBC::COMM::GroundLinkStats groundStats = {};
    OBC::COMM::GroundLinkObservationState sbandGroundObservation = {};
    OBC::COMM::GroundLinkObservationState uhfGroundObservation = {};
    int announceBootCalls = 0;
    int configureBootTrustCalls = 0;
    int configureCommCalls = 0;
    int shutdownCalls = 0;
    int startGroundLinkCalls = 0;
    int stopGroundLinkCalls = 0;
    int joinGroundLinkCalls = 0;
    int stopRateGroupCalls = 0;
    int healthSamples = 0;
    mutable int persistentFaultHistoryCalls = 0;
    bool healthEnabled = false;
    int setHealthThresholdCalls = 0;
    OBC::HealthItem lastHealthItem = OBC::HealthItem::CPU_USAGE;
    float lastHealthThreshold = 0.0F;
    U8 cspNode = 0U;
    int setModeCalls = 0;
    U8 lastPingNode = 0U;
    U32 lastPingTimeout = 0U;
    int setPduCalls = 0;
    int refreshEpsStatusCalls = 0;
    int getCachedEpsStatusCalls = 0;
    U8 lastPduChannel = 0U;
    bool lastPduEnabled = false;
    int setHeaterCalls = 0;
    bool lastHeaterEnabled = false;
    int setAdcsModeCalls = 0;
    int refreshAdcsStateCalls = 0;
    int getCachedAdcsStateCalls = 0;
    OBC::AdcsMode lastAdcsMode = OBC::AdcsMode::IDLE;
    int gpsPollCalls = 0;
    int setGpsSourceCalls = 0;
    OBC::GpsSourceMode lastGpsMode = OBC::GpsSourceMode::FAKE;
    int storageScanCalls = 0;
    int setBandCalls = 0;
    OBC::CommBand lastBand = OBC::CommBand::SBAND;
    int startPassCalls = 0;
    U32 lastPassDuration = 0U;
    int stopPassCalls = 0;
    int setTtcPolicyCalls = 0;
    int setTtcWindowCalls = 0;
    int clearTtcWindowCalls = 0;
    int setWatchdogConfigCalls = 0;
    int setWatchdogProbeSuppressionCalls = 0;
    OBC::WatchdogSource lastWatchdogSource = OBC::WatchdogSource::EPS_BRIDGE;
    bool lastWatchdogEnabled = true;
    bool lastWatchdogProbeSuppressed = false;
    U32 lastWatchdogWarningTicks = 0U;
    U32 lastWatchdogSafeTicks = 0U;
    U32 lastWatchdogSuppressTicks = 0U;
    int enableRadioCalls = 0;
    bool lastRadioEnabled = false;
    int setRadioPowerCalls = 0;
    U8 lastRadioPower = 0U;
    int setRadioFreqCalls = 0;
    U32 lastRadioFreq = 0U;
    std::string lastUartRequest;
    std::string lastDelimitedRequest;
    char lastDelimiter = '\0';
    int bootPrepareCalls = 0;
    U32 lastImageSize = 0U;
    std::string lastDigest;
    int bootVerifyCalls = 0;
    std::string lastVerifyPath;
    int bootActivateCalls = 0;
    int bootConfirmCalls = 0;
    int bootRollbackCalls = 0;
    mutable U32 lastPersistentFaultLimit = 0U;
    OBC::RecoveryExitRequest recoveryExitRequest = OBC::RecoveryExitRequest::NONE;
    bool persistentFaultHistoryAvailable = true;
    Fw::CmdResponse ttcPolicyResponse = Fw::CmdResponse::OK;
    Fw::CmdResponse ttcWindowResponse = Fw::CmdResponse::OK;
    std::vector<std::string> callOrder = {};
};

std::string runCommand(FakeRuntimeServices& services, const std::string& command, bool* keepRunning = nullptr) {
    OBC::Runtime::RuntimeConfig config;
    OBC::Runtime::RuntimeState state;
    std::ostringstream out;
    const bool result = OBC::Runtime::handleCommand(command, services, state, config, out);
    if (keepRunning != nullptr) {
        *keepRunning = result;
    }
    return out.str();
}

void verifyHelpAndParseHelpers() {
    std::ostringstream help;
    OBC::Runtime::printHelp(help);
    assert(help.str().find("mode <safe|idle|hell|payload|ttc>") != std::string::npos);
    assert(help.str().find("comm pass start <sec>") != std::string::npos);
    assert(help.str().find("ttc config <on|off> <loss-timeout-sec>") != std::string::npos);
    assert(help.str().find("health threshold <cpu|rss> <value>") != std::string::npos);
    assert(help.str().find("watchdog status") != std::string::npos);
    assert(help.str().find("fault history [count]") != std::string::npos);
    assert(help.str().find("uart frame-hex <hex-payload>") != std::string::npos);
    assert(help.str().find("boot prepare <image-size> <sha256-hex>") != std::string::npos);
    assert(help.str().find("boot verify <staging-path>") != std::string::npos);

    bool enabled = false;
    assert(OBC::Runtime::parseBoolWord("on", enabled) && enabled);
    assert(OBC::Runtime::parseBoolWord("false", enabled) && !enabled);
    assert(!OBC::Runtime::parseBoolWord("maybe", enabled));

    std::string bytes;
    assert(OBC::Runtime::parseHexBytes("007E7DFF", bytes));
    assert(bytes.size() == 4U);
    assert(!OBC::Runtime::parseHexBytes("ABC", bytes));
    assert(!OBC::Runtime::parseHexBytes("GG", bytes));
}

void verifyArgumentParsing() {
    assert(OBC::Runtime::groundLinkHealthSemanticsForCommCspNode(OBC::COMM::CSP::DEFAULT_GENERIC_COMM_NODE_ID) ==
           OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK);
    assert(OBC::Runtime::groundLinkHealthSemanticsForCommCspNode(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID) ==
           OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    assert(OBC::Runtime::groundLinkHealthSemanticsForCommCspNode(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID) ==
           OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    assert(OBC::Runtime::groundLinkHealthSemanticsForCommCspNode(7U) ==
           OBC::COMM::GroundLinkHealthSemantics::DISABLED);

    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char comm[] = "--comm";
        char serial[] = "serial";
        char deviceOpt[] = "--comm-device";
        char device[] = "/tmp/tty";
        char baudOpt[] = "--comm-baudrate";
        char baud[] = "230400";
        char groundOpt[] = "--ground-link";
        char ground[] = "comm-csp";
        char nodeOpt[] = "--comm-csp-node";
        char node[] = "5";
        char runtimeOpt[] = "--runtime-root";
        char runtime[] = "/tmp/runtime";
        char quietOpt[] = "--diagnostic-quiet-packet-egress";
        char healthDetectorOpt[] = "--enable-comm-subsystem-health-detector";
        char disablePrimaryGroundLinkOpt[] = "--disable-primary-ground-link-driver";
        char initialBandOpt[] = "--initial-comm-band";
        char initialBand[] = "uhf";
        char pingTimeoutOpt[] = "--comm-subsystem-ping-timeout-ms";
        char pingTimeout[] = "500";
        char unavailableThresholdOpt[] = "--comm-primary-unavailable-failure-threshold";
        char unavailableThreshold[] = "10";
        char tickOpt[] = "--tick-ms";
        char tick[] = "250";
        char trustOpt[] = "--boot-trust";
        char trust[] = "hmac-sha256";
        char trustSignerOpt[] = "--boot-trust-signer-id";
        char trustSigner[] = "repo-dev-boot-signer";
        char trustSlotOpt[] = "--boot-trust-key-slot";
        char trustSlot[] = "1";
        char trustKeyOpt[] = "--boot-trust-key-hex";
        char trustKey[] = "424f4f545f54525553545f434841494e5f56315f4445565f4b4559";
        char* argv[] = {app, comm, serial, deviceOpt, device, baudOpt, baud, groundOpt, ground,
                        nodeOpt, node, runtimeOpt, runtime, quietOpt, healthDetectorOpt, disablePrimaryGroundLinkOpt,
                        initialBandOpt, initialBand, pingTimeoutOpt, pingTimeout,
                        unavailableThresholdOpt, unavailableThreshold, tickOpt, tick, trustOpt, trust,
                        trustSignerOpt, trustSigner, trustSlotOpt, trustSlot, trustKeyOpt, trustKey};
        assert(OBC::Runtime::parseArgs(32, argv, config, out, err));
        assert(config.commMode == "serial");
        assert(config.commDevice == "/tmp/tty");
        assert(config.commBaudrate == 230400U);
        assert(config.commCspNode == 5U);
        assert(config.persistentRoot == "/tmp/runtime/persistent-data");
        assert(config.stagingRoot == "/tmp/runtime/staging");
        assert(config.diagnosticQuietPacketEgress);
        assert(config.enableCommSubsystemHealthDetector);
        assert(!config.enablePrimaryGroundLinkDriver);
        assert(config.initialCommBand == OBC::CommBand::UHF);
        assert(config.commSubsystemPingTimeoutMs == 500U);
        assert(config.commPrimaryUnavailableFailureThreshold == 10U);
        assert(config.tickMs == 250);
        assert(config.bootTrustSignerId == "repo-dev-boot-signer");
        assert(config.bootTrustKeySlot == 1U);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char trustOpt[] = "--boot-trust";
        char trust[] = "disabled";
        char* argv[] = {app, trustOpt, trust};
        assert(!OBC::Runtime::parseArgs(3, argv, config, out, err));
        assert(err.str().find("Invalid --boot-trust mode") != std::string::npos);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char tickOpt[] = "--tick-ms";
        char tick[] = "bad";
        char* argv[] = {app, tickOpt, tick};
        assert(!OBC::Runtime::parseArgs(3, argv, config, out, err));
        assert(err.str().find("--tick-ms must be an integer") != std::string::npos);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char unknown[] = "--bogus";
        char* argv[] = {app, unknown};
        assert(!OBC::Runtime::parseArgs(2, argv, config, out, err));
        assert(err.str().find("Unknown argument: --bogus") != std::string::npos);
        assert(out.str().find("Usage: OBC") != std::string::npos);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char authOpt[] = "--command-auth";
        char auth[] = "disabled";
        char* argv[] = {app, authOpt, auth};
        assert(!OBC::Runtime::parseArgs(3, argv, config, out, err));
        assert(err.str().find("--command-auth has been removed") != std::string::npos);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char keystoreOpt[] = "--command-auth-keystore";
        char path[] = "/tmp/command-auth.ini";
        char* argv[] = {app, keystoreOpt, path};
        assert(!OBC::Runtime::parseArgs(3, argv, config, out, err));
        assert(err.str().find("--command-auth-keystore has been removed") != std::string::npos);
    }
    {
        OBC::Runtime::RuntimeConfig config;
        std::ostringstream out;
        std::ostringstream err;
        char app[] = "OBC";
        char slotOpt[] = "--command-auth-key-slot";
        char slot[] = "9";
        char* argv[] = {app, slotOpt, slot};
        assert(!OBC::Runtime::parseArgs(3, argv, config, out, err));
        assert(err.str().find("--command-auth-key-slot has been removed") != std::string::npos);
    }
}

void verifyCurrentRssTracksResidentSet() {
    const float before = OBC::Runtime::currentRssMb();
    assert(before >= 0.0F);

    static constexpr std::size_t ALLOCATION_BYTES = 64U * 1024U * 1024U;
    void* mapping = ::mmap(nullptr, ALLOCATION_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    assert(mapping != MAP_FAILED);

    const long pageSize = ::sysconf(_SC_PAGESIZE);
    assert(pageSize > 0);
    for (std::size_t offset = 0; offset < ALLOCATION_BYTES; offset += static_cast<std::size_t>(pageSize)) {
        static_cast<char*>(mapping)[offset] = 0x5A;
    }

    const float during = OBC::Runtime::currentRssMb();
    assert(during > before + 16.0F);

    const int munmapResult = ::munmap(mapping, ALLOCATION_BYTES);
    assert(munmapResult == 0);
    static_cast<void>(munmapResult);
    ::usleep(100000);

    const float after = OBC::Runtime::currentRssMb();
    assert(after + 8.0F < during);
}

void verifyCommandRouting() {
    FakeRuntimeServices services;

    bool keepRunning = true;
    assert(runCommand(services, "quit", &keepRunning).empty());
    assert(!keepRunning);
    assert(runCommand(services, "exit", &keepRunning).empty());
    assert(!keepRunning);

    assert(runCommand(services, "unknown").find("unknown command") != std::string::npos);
    assert(runCommand(services, "mode nominal").find("unknown mode") != std::string::npos);
    assert(runCommand(services, "mode PAYLOAD").find("unknown mode") != std::string::npos);
    std::string output = runCommand(services, "mode idle");
    assert(services.setModeCalls == 1);
    assert(services.mode == OBC::SatMode::IDLE);
    assert(output.find("mode response=0") != std::string::npos);
    assert(output.find("mode=IDLE") != std::string::npos);
    assert(output.find("radioObservation sample=yes ageTicks=0 result=OK") != std::string::npos);
    assert(output.find("groundLinkRaw band=SBAND mode=direct-tcp") != std::string::npos);

    services.modeResponse = Fw::CmdResponse::VALIDATION_ERROR;
    output = runCommand(services, "mode payload");
    assert(services.setModeCalls == 2);
    assert(output.find("mode response=2") != std::string::npos);

    output = runCommand(services, "csp ping 4");
    assert(services.lastPingNode == 4U);
    assert(services.lastPingTimeout == 250U);
    assert(output.find("csp ping response=0 success=yes") != std::string::npos);

    output = runCommand(services, "eps pdu 2 on");
    assert(services.setPduCalls == 1);
    assert(services.lastPduChannel == 2U);
    assert(services.lastPduEnabled);
    assert(output.find("eps pdu response=0") != std::string::npos);
    assert(runCommand(services, "eps pdu 2 maybe").find("expected on/off") != std::string::npos);
    output = runCommand(services, "eps get");
    assert(services.refreshEpsStatusCalls == 1);
    assert(services.getCachedEpsStatusCalls >= 1);
    assert(output.find("eps soc=") != std::string::npos);

    output = runCommand(services, "eps heater off");
    assert(services.setHeaterCalls == 1);
    assert(!services.lastHeaterEnabled);
    assert(output.find("eps heater response=0") != std::string::npos);

    assert(runCommand(services, "adcs mode invalid").find("unknown adcs mode") != std::string::npos);
    output = runCommand(services, "adcs mode pointing");
    assert(services.setAdcsModeCalls == 1);
    assert(services.lastAdcsMode == OBC::AdcsMode::POINTING);
    assert(output.find("adcs mode response=0") != std::string::npos);
    output = runCommand(services, "adcs get");
    assert(services.refreshAdcsStateCalls == 1);
    assert(services.getCachedAdcsStateCalls >= 1);
    assert(output.find("adcs mode=") != std::string::npos);

    assert(runCommand(services, "gps source invalid").find("unknown gps source") != std::string::npos);
    output = runCommand(services, "gps source live-uart");
    assert(services.setGpsSourceCalls == 1);
    assert(services.lastGpsMode == OBC::GpsSourceMode::LIVE_UART);

    output = runCommand(services, "storage scan");
    assert(services.storageScanCalls == 1);
    assert(output.find("storage scan success=yes") != std::string::npos);

    output = runCommand(services, "health enable on");
    assert(services.healthEnabled);
    assert(output.find("watchdog aggregate=HEALTHY") != std::string::npos);
    output = runCommand(services, "health threshold rss 12.5");
    assert(services.setHealthThresholdCalls == 1);
    assert(services.lastHealthItem == OBC::HealthItem::MEM_RSS_MB);
    assert(services.lastHealthThreshold == 12.5F);
    assert(output.find("health threshold response=0") != std::string::npos);
    assert(runCommand(services, "health threshold bogus 1").find("unknown health item") != std::string::npos);
    assert(runCommand(services, "health threshold rss foo")
               .find("health threshold value must be a numeric value: foo") != std::string::npos);
    assert(runCommand(services, "health threshold rss 12abc")
               .find("health threshold value must be a numeric value: 12abc") != std::string::npos);
    assert(runCommand(services, "health threshold rss")
               .find("health threshold value requires a non-empty numeric value") != std::string::npos);
    assert(services.setHealthThresholdCalls == 1);

    output = runCommand(services, "comm band uhf");
    assert(services.setBandCalls == 1);
    assert(services.lastBand == OBC::CommBand::UHF);
    output = runCommand(services, "comm pass start 12");
    assert(services.startPassCalls == 1);
    assert(services.lastPassDuration == 12U);
    output = runCommand(services, "comm pass stop");
    assert(services.stopPassCalls == 1);
    output = runCommand(services, "ttc config on 8");
    assert(services.setTtcPolicyCalls == 1);
    assert(services.ttc.enabled);
    assert(services.ttc.lossOfLockTimeoutSec == 8U);
    assert(output.find("ttc config response=0") != std::string::npos);
    output = runCommand(services, "ttc window set 100 220");
    assert(services.setTtcWindowCalls == 1);
    assert(services.ttc.windowConfigured);
    assert(services.ttc.windowStartUnixSec == 100U);
    assert(services.ttc.windowEndUnixSec == 220U);
    assert(output.find("ttc window response=0") != std::string::npos);
    output = runCommand(services, "ttc window set 1 18446744073709551615");
    assert(services.setTtcWindowCalls == 2);
    assert(services.ttc.windowStartUnixSec == 1U);
    assert(services.ttc.windowEndUnixSec == std::numeric_limits<U64>::max());
    assert(output.find("ttc window response=0") != std::string::npos);
    output = runCommand(services, "ttc window set 1 18446744073709551616");
    assert(services.setTtcWindowCalls == 2);
    assert(output.find("ttc window endUnixSec must be an integer in range [0, 18446744073709551615]") !=
           std::string::npos);
    output = runCommand(services, "ttc window clear");
    assert(services.clearTtcWindowCalls == 1);
    assert(!services.ttc.windowConfigured);
    assert(output.find("ttc window clear response=0") != std::string::npos);
    services.ttc.windowConfigured = true;
    services.ttc.windowStartUnixSec = 100U;
    services.ttc.windowEndUnixSec = 220U;
    output = runCommand(services, "ttc status");
    assert(output.find("ttc enabled=yes") != std::string::npos);

    output = runCommand(services, "watchdog config comm-controller off 4 7 9");
    assert(services.setWatchdogConfigCalls == 1);
    assert(services.lastWatchdogSource == OBC::WatchdogSource::COMM_CONTROLLER);
    assert(!services.lastWatchdogEnabled);
    assert(services.lastWatchdogWarningTicks == 4U);
    assert(services.lastWatchdogSafeTicks == 7U);
    assert(services.lastWatchdogSuppressTicks == 9U);
    assert(output.find("watchdog config response=0") != std::string::npos);
    assert(runCommand(services, "watchdog config bogus on 1 2 3").find("unknown watchdog source") !=
           std::string::npos);
    output = runCommand(services, "watchdog config adcs-fdir on 3 4 5");
    assert(services.setWatchdogConfigCalls == 2);
    assert(services.lastWatchdogSource == OBC::WatchdogSource::ADCS_FDIR);
    assert(services.lastWatchdogEnabled);
    assert(services.lastWatchdogWarningTicks == 3U);
    assert(services.lastWatchdogSafeTicks == 4U);
    assert(services.lastWatchdogSuppressTicks == 5U);
    assert(runCommand(services, "watchdog config comm-controller on -1 -1 -1")
               .find("watchdog warningTicks must be an integer in range [0, 4294967295]: -1") != std::string::npos);
    assert(runCommand(services, "watchdog config comm-controller on +1 2 3")
               .find("watchdog warningTicks must be an integer in range [0, 4294967295]: +1") !=
           std::string::npos);
    assert(runCommand(services, "watchdog config comm-controller on 1 2")
               .find("watchdog suppressTicks requires a non-empty numeric value") != std::string::npos);
    assert(services.setWatchdogConfigCalls == 2);
    output = runCommand(services, "watchdog suppress comm-controller on");
    assert(services.setWatchdogProbeSuppressionCalls == 1);
    assert(services.lastWatchdogProbeSuppressed);
    assert(output.find("watchdog suppress success=yes") != std::string::npos);
    output = runCommand(services, "watchdog status");
    assert(output.find("watchdog source=COMM_CONTROLLER enabled=no state=HEALTHY age=0 beatPending=no "
                       "probeSuppressed=yes warn/safe/suppress=4/7/9") != std::string::npos);

    services.recovery.activeIncidentCount = 1U;
    services.recovery.activeSource = OBC::RecoveryIncidentSource::EPS_TIMEOUT;
    services.recovery.currentLevel = OBC::RecoveryLevel::R6_OBC_REBOOT;
    services.recovery.highestLevel = OBC::RecoveryLevel::R6_OBC_REBOOT;
    services.recovery.lastAction = OBC::RecoveryAction::OBC_REBOOT;
    services.recovery.pendingProcessRestart = false;
    services.recovery.pendingReboot = true;
    services.recovery.relatchCount = 1U;
    output = runCommand(services, "recovery status");
    assert(output.find("recovery activeCount=1 activeSource=EPS_TIMEOUT currentLevel=R6_OBC_REBOOT "
                       "highestLevel=R6_OBC_REBOOT lastAction=OBC_REBOOT pendingProcessRestart=no "
                       "pendingReboot=yes relatchCount=1") !=
           std::string::npos);

    services.persistentFaultStatus.totalRecords = 2U;
    services.persistentFaultStatus.returnedRecords = 1U;
    services.persistentFaultStatus.activeCopy = OBC::PersistentFaultStoreCopy::COPY_B;
    services.persistentFaultStatus.generation = 4U;
    services.persistentFaultRecords = {OBC::PersistentFaultRecord{
        OBC::PersistentFaultRecordKind::REBOOT_ISSUED,
        OBC::RecoveryIncidentSource::EPS_TIMEOUT,
        OBC::RecoveryLevel::R6_OBC_REBOOT,
        OBC::RecoveryAction::OBC_REBOOT,
        OBC::ResetCause::RECOVERY_EPS_TIMEOUT,
        0U,
        0U,
        8U,
        2U,
        17U,
        0U,
    }};
    output = runCommand(services, "fault history 1");
    assert(services.persistentFaultHistoryCalls == 1);
    assert(services.lastPersistentFaultLimit == 1U);
    assert(output.find("fault total=2 returned=1 activeCopy=COPY_B generation=4") != std::string::npos);
    assert(output.find("fault[0] kind=REBOOT_ISSUED source=EPS_TIMEOUT level=R6_OBC_REBOOT action=OBC_REBOOT") !=
           std::string::npos);
    assert(runCommand(services, "fault history -1")
               .find("fault history count must be an integer in range [0, 4294967295]: -1") != std::string::npos);

    assert(runCommand(services, "radio enable maybe").find("expected on/off") != std::string::npos);
    output = runCommand(services, "radio enable on");
    assert(services.enableRadioCalls == 1);
    assert(services.lastRadioEnabled);
    output = runCommand(services, "radio power 17");
    assert(services.setRadioPowerCalls == 1);
    assert(services.lastRadioPower == 17U);
    output = runCommand(services, "radio freq 435000000");
    assert(services.setRadioFreqCalls == 1);
    assert(services.lastRadioFreq == 435000000U);

    output = runCommand(services, "uart raw PING");
    assert(services.lastUartRequest == "PING\n");
    assert(output.find("uart response: PONG") != std::string::npos);
    assert(runCommand(services, "uart frame-hex ABC").find("expected even-length hex payload") != std::string::npos);
    output = runCommand(services, "uart frame-hex 4142");
    assert(services.lastDelimiter == OBC::COMM::TRANSPARENT_FRAME_DELIMITER);
    assert(output.find("uart frame response hex: 41434B") != std::string::npos);

    output = runCommand(services, "boot prepare 42 abcdef");
    assert(services.bootPrepareCalls == 1);
    assert(services.lastImageSize == 42U);
    assert(services.lastDigest == "abcdef");
    output = runCommand(services, "boot verify /tmp/image.bin");
    assert(services.bootVerifyCalls == 1);
    assert(services.lastVerifyPath == "/tmp/image.bin");
    assert(runCommand(services, "boot activate").find("boot activate response=0") != std::string::npos);
    assert(runCommand(services, "boot confirm").find("boot confirm response=0") != std::string::npos);
    assert(runCommand(services, "boot rollback").find("boot rollback response=0") != std::string::npos);
}

void verifyRuntimeInitializationOrdering() {
    FakeRuntimeServices services;
    OBC::Runtime::RuntimeConfig config;
    config.runtimeRoot = "/tmp/runtime-order";
    config.persistentRoot = "/tmp/custom-persistent";
    config.stagingRoot = "/tmp/custom-staging";
    config.headless = false;
    config.tickMs = 1;

    OBC::Runtime::StartupBanner banner;
    banner.runtimeStartedLine = "runtime started";

    std::ostringstream out;
    std::ostringstream err;
    int stdinPipe[2] = {-1, -1};
    assert(::pipe(stdinPipe) == 0);
    static constexpr char QUIT_COMMAND[] = "quit\n";
    assert(::write(stdinPipe[1], QUIT_COMMAND, sizeof(QUIT_COMMAND) - 1) == static_cast<ssize_t>(sizeof(QUIT_COMMAND) - 1));
    assert(::close(stdinPipe[1]) == 0);
    stdinPipe[1] = -1;

    const int savedStdin = ::dup(STDIN_FILENO);
    assert(savedStdin >= 0);
    assert(::dup2(stdinPipe[0], STDIN_FILENO) >= 0);
    assert(::close(stdinPipe[0]) == 0);
    stdinPipe[0] = -1;
    std::cin.clear();
    const int code = OBC::Runtime::runHostedRuntime(config, services, banner, out, err);
    assert(::dup2(savedStdin, STDIN_FILENO) >= 0);
    assert(::close(savedStdin) == 0);
    std::cin.clear();

    assert(code == 0);
    const std::vector<std::string> expected = {
        "boot-trust",
        "command-ingress-persistence",
        "persistent-fault-storage",
        "boot-storage",
        "storage-runtime",
        "init-csp",
    };
    assert(services.callOrder.size() >= expected.size());
    assert(std::equal(expected.begin(), expected.end(), services.callOrder.begin()));
}

void verifyRecoveryExitCodes() {
    {
        FakeRuntimeServices services;
        services.recoveryExitRequest = OBC::RecoveryExitRequest::PROCESS_RESTART;
        OBC::Runtime::RuntimeConfig config;
        config.runtimeRoot = "/tmp/runtime-r2-exit";
        config.headless = true;
        config.tickMs = 1;
        OBC::Runtime::StartupBanner banner;
        banner.runtimeStartedLine = "runtime started";
        std::ostringstream out;
        std::ostringstream err;
        const int code = OBC::Runtime::runHostedRuntime(config, services, banner, out, err);
        assert(code == 31);
        assert(services.refreshEpsStatusCalls == 0);
        assert(services.refreshAdcsStateCalls == 0);
        assert(services.getCachedEpsStatusCalls >= 1);
        assert(services.getCachedAdcsStateCalls >= 1);
    }
    {
        FakeRuntimeServices services;
        services.recoveryExitRequest = OBC::RecoveryExitRequest::OBC_REBOOT;
        OBC::Runtime::RuntimeConfig config;
        config.runtimeRoot = "/tmp/runtime-r6-exit";
        config.headless = true;
        config.tickMs = 1;
        OBC::Runtime::StartupBanner banner;
        banner.runtimeStartedLine = "runtime started";
        std::ostringstream out;
        std::ostringstream err;
        const int code = OBC::Runtime::runHostedRuntime(config, services, banner, out, err);
        assert(code == 32);
    }
}

}  // namespace

int main() {
    verifyHelpAndParseHelpers();
    verifyArgumentParsing();
    verifyCurrentRssTracksResidentSet();
    verifyCommandRouting();
    verifyRuntimeInitializationOrdering();
    verifyRecoveryExitCodes();
    std::cout << "hosted_runtime_unit_test: PASS\n";
    return 0;
}
