#include "OBC/TopCcsds/OBCAppTopology.hpp"

#include "OBC/TopCcsds/AppTopologyAc.hpp"
#include "OBC/TopCcsds/OnboardStateSnapshotSource.hpp"
#include "OBC/TopCcsds/PayloadCspService.hpp"
#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

#include "Fw/Dp/DpContainer.hpp"
#include "Fw/Types/FileNameString.hpp"
#include "Fw/Types/MallocAllocator.hpp"
#include "Os/FileSystem.hpp"
#include "Svc/BufferManager/BufferManager.hpp"
#include "Svc/DpCatalog/DpCatalog.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <limits.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace OBCApp {

namespace {

Fw::MallocAllocator mallocator;
OnboardStateSnapshotSource onboardStateSnapshotSource;
constexpr FwSizeType SEQUENCE_BUFFER_BYTES = 4096U;
constexpr U32 SEQUENCE_TIMEOUT_SECONDS = 30U;
constexpr FwSizeType DP_SMALL_BUFFER_BYTES = 10000U;
constexpr FwSizeType DP_PAYLOAD_BUFFER_BYTES = Fw::DpContainer::getPacketSizeForDataSize(OBC::PAYLOAD_DATA_PRODUCT_MAX_BYTES);
constexpr U32 DEFAULT_RG1_TIMING_PROBE_THRESHOLD_USEC = 900000U;

bool readBoolEnv(const char* name, bool fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0 || std::strcmp(value, "TRUE") == 0;
}

U32 readU32Env(const char* name, U32 fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno == ERANGE || end == value || end == nullptr || *end != '\0' ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<U32>::max())) {
        return fallback;
    }
    return static_cast<U32>(parsed);
}

class UartBeaconSink final : public OBC::StateData::IBeaconSink {
  public:
    bool sendBeacon(const U8* data, U32 size) override {
        return uartDriver.queueSendForRuntime(data, size);
    }
};

class CspBeaconSink final : public OBC::StateData::IBeaconSink {
  public:
    CspBeaconSink() : m_targetNode(0U) {}

    void configure(U16 targetNode) {
        this->m_targetNode = targetNode;
    }

    bool sendBeacon(const U8* data, U32 size) override {
        if (this->m_targetNode == 0U || data == nullptr || size == 0U || this->m_targetNode > 255U) {
            return false;
        }
        const std::string payload(reinterpret_cast<const char*>(data), static_cast<std::size_t>(size));
        return cspBridge.sendRawForRuntime(static_cast<U8>(this->m_targetNode),
                                           static_cast<U8>(OBC::COMM::CSP::ServicePort::BEACON_PUSH),
                                           payload) == Fw::CmdResponse::OK;
    }

  private:
    U16 m_targetNode;
};

UartBeaconSink uartBeaconSink;
CspBeaconSink cspBeaconSink;
PayloadCspService payloadCspService;
class PayloadEpsProxy final : public OBC::IPayloadEpsControl {
  public:
    void configure(U32 proxyPduChannel) { this->m_proxyPduChannel = static_cast<U8>(proxyPduChannel); }

    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override {
        return epsBridge.getCachedStatusForRuntime(status);
    }

    Fw::CmdResponse setPayloadProxyPower(bool enabled, OBC::EPS::StatusData& status) override {
        return epsBridge.setPduForRuntime(this->m_proxyPduChannel, enabled, status);
    }

  private:
    U8 m_proxyPduChannel = 3U;
};

PayloadEpsProxy payloadEpsProxy;
Svc::BufferManager::BufferBins dpBufferBins = {};

Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {5, 0}, {1, 0}}};

U32 rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup2Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup3Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

void configureTopology() {
    rateGroupDriverComp.configure(rateGroupDivisorsSet);
    rateGroup1Comp.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
    rateGroup2Comp.configure(rateGroup2Context, FW_NUM_ARRAY_ELEMENTS(rateGroup2Context));
    rateGroup3Comp.configure(rateGroup3Context, FW_NUM_ARRAY_ELEMENTS(rateGroup3Context));
    rateGroup1TimingProbe.configureForRuntime(readBoolEnv("RG1_TIMING_PROBE_ENABLE", false),
                                              readU32Env("RG1_TIMING_PROBE_CYCLE_THRESHOLD_USEC",
                                                         DEFAULT_RG1_TIMING_PROBE_THRESHOLD_USEC));
    static_cast<void>(mallocator);
}

std::string joinPath(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    return base.back() == '/' ? base + child : base + "/" + child;
}

bool ensureDirectoryTree(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::string current;
    if (path.front() == '/') {
        current = "/";
    }

    std::istringstream pathStream(path);
    std::string segment;
    while (std::getline(pathStream, segment, '/')) {
        if (segment.empty()) {
            continue;
        }

        if (!current.empty() && current.back() != '/') {
            current.push_back('/');
        }
        current += segment;

        const Os::FileSystem::PathType pathType = Os::FileSystem::getPathType(current.c_str());
        if (pathType == Os::FileSystem::PathType::DIRECTORY) {
            continue;
        }
        if (pathType == Os::FileSystem::PathType::FILE) {
            std::cerr << "Data product directory path is not a directory: " << current << "\n";
            return false;
        }

        const Os::FileSystem::Status status = Os::FileSystem::createDirectory(current.c_str(), false);
        if (status != Os::FileSystem::Status::OP_OK && status != Os::FileSystem::Status::ALREADY_EXISTS) {
            std::cerr << "Failed to create data product directory: " << current
                      << " status=" << static_cast<int>(status) << "\n";
            return false;
        }
    }

    return true;
}

bool configureDataProducts(const char* runtimeRoot) {
    const std::string root = runtimeRoot == nullptr || runtimeRoot[0] == '\0' ? std::string("runtime")
                                                                               : std::string(runtimeRoot);
    const std::string dpRoot = joinPath(root, "data-products");
    const std::string statePath = joinPath(dpRoot, "DpState.dat");
    if (!ensureDirectoryTree(root) || !ensureDirectoryTree(dpRoot)) {
        std::cerr << "Data product setup failed for runtime root: " << root << "\n";
        return false;
    }

    Fw::FileNameString dpDirs[Svc::DP_MAX_DIRECTORIES];
    Fw::FileNameString dpState;
    dpDirs[0].format("%s", dpRoot.c_str());
    dpState.format("%s", statePath.c_str());

    dpCatalog.configure(dpDirs, 1U, dpState, 0U, mallocator);
    dpWriter.configure(dpDirs[0]);

    std::memset(&dpBufferBins, 0, sizeof(dpBufferBins));
    dpBufferBins.bins[0].bufferSize = DP_SMALL_BUFFER_BYTES;
    dpBufferBins.bins[0].numBuffers = 10U;
    dpBufferBins.bins[1].bufferSize = DP_PAYLOAD_BUFFER_BYTES;
    dpBufferBins.bins[1].numBuffers = 2U;
    dpBufferManager.setup(300U, 0U, mallocator, dpBufferBins);
    return true;
}

std::string currentWorkingRoot() {
    char pathBuffer[PATH_MAX] = {};
    if (::getcwd(pathBuffer, sizeof(pathBuffer)) == nullptr) {
        return std::string(".");
    }
    return std::string(pathBuffer);
}

bool configureSecureAuthIngress(FwIndexType ingressPort,
                                const OBC::AuthorityConfig& authorityConfig,
                                const U8* keyBytes,
                                FwSizeType keyLength,
                                const char* label) {
    const U8 serviceId = OBC::secureServiceIdForAuthorityIdentity(authorityConfig.identity);
    if (!authorityConfig.valid || serviceId == 0U) {
        return true;
    }
    if (keyBytes == nullptr || keyLength == 0U || keyLength > OBC::CommandAuthConfig::MAX_KEY_BYTES) {
        std::cerr << "Invalid secure auth configuration for " << (label == nullptr ? "ingress" : label) << "\n";
        return false;
    }
    if (!secureLinkAuthorizer.configureIngressService(ingressPort, serviceId, keyBytes, keyLength)) {
        std::cerr << "Failed to configure secure auth ingress for " << (label == nullptr ? "ingress" : label) << "\n";
        return false;
    }
    return true;
}

bool configureSecureAuthIngressForAuthority(FwIndexType ingressPort,
                                            const OBC::AuthorityConfig& authorityConfig,
                                            const TopologyState& state,
                                            const char* label) {
    const U8* keyBytes = nullptr;
    FwSizeType keyLength = 0U;
    switch (authorityConfig.identity) {
        case OBC::AuthorityLinkIdentity::SBAND:
            keyBytes = state.sbandSecureAuthKeyBytes;
            keyLength = state.sbandSecureAuthKeyLength;
            break;
        case OBC::AuthorityLinkIdentity::UHF:
            keyBytes = state.uhfSecureAuthKeyBytes;
            keyLength = state.uhfSecureAuthKeyLength;
            break;
        default:
            break;
    }
    return configureSecureAuthIngress(ingressPort, authorityConfig, keyBytes, keyLength, label);
}

}  // namespace

bool setupTopology(const TopologyState& state) {
    initComponents(state);
    setBaseIds();
    connectComponents();
    seqDispatcher.set_seqRunOut_OutputPort(0, cmdSeqA.get_seqRunIn_InputPort(0));
    seqDispatcher.set_seqRunOut_OutputPort(1, cmdSeqB.get_seqRunIn_InputPort(0));
    seqCallbackFanoutA.set_dispatcherSeqStartOut_OutputPort(0, seqDispatcher.get_seqStartIn_InputPort(0));
    seqCallbackFanoutA.set_dispatcherSeqDoneOut_OutputPort(0, seqDispatcher.get_seqDoneIn_InputPort(0));
    seqCallbackFanoutB.set_dispatcherSeqStartOut_OutputPort(0, seqDispatcher.get_seqStartIn_InputPort(1));
    seqCallbackFanoutB.set_dispatcherSeqDoneOut_OutputPort(0, seqDispatcher.get_seqDoneIn_InputPort(1));
    regCommands();
    configComponents(state);
    commandIngressAuthority.clearIngressSources();
    commandIngressAuthority.configureIngressSource(0, state.sbandCommandAuthorityConfig);
    commandIngressAuthority.configureIngressSource(1, state.uhfCommandAuthorityConfig);
    if (!configureSecureAuthIngressForAuthority(0, state.sbandCommandAuthorityConfig, state, "sband")) {
        return false;
    }
    if (!configureSecureAuthIngressForAuthority(1, state.uhfCommandAuthorityConfig, state, "uhf")) {
        return false;
    }
    if (state.commandAuthModuleSerial != nullptr && state.commandAuthModuleSerial[0] != '\0' &&
        !secureLinkAuthorizer.configureModuleSerial(state.commandAuthModuleSerial)) {
        std::cerr << "Failed to configure secure auth module serial\n";
        return false;
    }
    const std::string runtimeRoot = state.runtimeRoot == nullptr ? std::string("runtime") : std::string(state.runtimeRoot);
    const std::string workingRoot = currentWorkingRoot();
    OBC::PayloadRuntimeConfig payloadRuntimeConfig = {};
    if (!fileIngressAuthority.configureRuntime(runtimeRoot, workingRoot)) {
        std::cerr << "Failed to configure file ingress authority runtime\n";
        return false;
    }
    if (!sequenceAdmissionController.configureRuntime(runtimeRoot, workingRoot)) {
        std::cerr << "Failed to configure sequence admission controller runtime\n";
        return false;
    }
    const std::string persistentRoot = joinPath(runtimeRoot, "persistent-data");
    if (!persistentFaultManager.configurePersistentRootForRuntime(persistentRoot)) {
        std::cerr << "Failed to configure persistent fault store runtime\n";
        return false;
    }
    bootManager.configurePersistentFaultRecorderForRuntime(&persistentFaultManager);
    gpsBridge.configureRuntime(state.runtimeRoot == nullptr ? std::string() : std::string(state.runtimeRoot));
    recoveryExecutor.configureRuntime(&modeManager, &epsBridge, &adcsBridge, &bootManager, &commController);
    recoveryExecutor.configureHardwareWatchdogModeForRuntime(state.hardwareWatchdogEnabled);
    recoveryExecutor.configurePersistentFaultRecorderForRuntime(&persistentFaultManager);
    epsFdirController.configureRuntime(&modeManager, &epsBridge, &recoveryExecutor);
    adcsFdirController.configureRuntime(&adcsBridge, &recoveryExecutor);
    modeSafetyController.configureRuntime(&modeManager, &epsBridge);
    ttcPassManager.configureRuntime(&modeManager, &gpsBridge, &commController, &adcsBridge);
    watchdogSupervisor.configureRuntime(&modeManager, &recoveryExecutor);
    payloadEpsProxy.configure(payloadRuntimeConfig.proxyPduChannel);
    if (!payloadOpsController.configureRuntime(runtimeRoot,
                                               &modeManager,
                                               &payloadEpsProxy,
                                               &bootManager,
                                               OBC::makeDefaultPiCameraDriver(),
                                               payloadRuntimeConfig)) {
        std::cerr << "Failed to configure payload ops controller runtime\n";
        return false;
    }
    payloadCspService.configure(&payloadOpsController);
    if (!linuxWatchdogSink.configureRuntime(state.hardwareWatchdogEnabled,
                                            state.hardwareWatchdogDevice == nullptr
                                                ? std::string("/dev/watchdog0")
                                                : std::string(state.hardwareWatchdogDevice),
                                            state.hardwareWatchdogTimeoutSec)) {
        std::cerr << "Failed to configure Linux hardware watchdog runtime\n";
        return false;
    }
    modeManager.configureOperatorTransitionGuard(&modeSafetyController);
    commEgressMux.setDiagnosticQuietPacketEgressForRuntime(state.diagnosticQuietPacketEgress);
    cspBridge.setRuntimeForRuntime(cspRuntimeOwner);
    epsBridge.configureCspRuntimeForRuntime(cspRuntimeOwner);
    adcsBridge.configureCspRuntimeForRuntime(cspRuntimeOwner);
    commController.configureCspRuntimeForRuntime(cspRuntimeOwner);
    const OBC::CSP::RuntimeConfig cspRuntimeConfig =
        OBC::CSP::runtimeConfigFromEnvironment(OBC::COMM::CSP::DEFAULT_OBC_NODE_ID, "OBCCSP");
    if (cspRuntimeOwner.init(cspRuntimeConfig) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize shared CSP runtime owner\n";
        return false;
    }
    onboardStateSnapshotSource.configure(
        &modeManager,
        &epsBridge,
        &adcsBridge,
        &gpsBridge,
        &storageHealthBridge,
        &commController,
        &cspBridge,
        &radioController,
        &uartDriver,
        &bootManager);
    onboardStateMonitor.configureRuntime(&onboardStateSnapshotSource);
    hkTrendProductProducer.configureRuntime(&onboardStateSnapshotSource);
    cspBeaconSink.configure(state.uhfBeaconCspNode);
    if (state.uhfBeaconSideChannelEnabled) {
        beaconPublisher.configureRuntime(&onboardStateMonitor, &cspBeaconSink, true);
    } else {
        beaconPublisher.configureRuntime(&onboardStateMonitor, &uartBeaconSink, state.beaconBroadcastEnabled);
    }
    if (!configureDataProducts(state.runtimeRoot)) {
        return false;
    }
    cmdSeqA.allocateBuffer(400U, mallocator, SEQUENCE_BUFFER_BYTES);
    cmdSeqB.allocateBuffer(401U, mallocator, SEQUENCE_BUFFER_BYTES);
    cmdSeqA.setTimeout(SEQUENCE_TIMEOUT_SECONDS);
    cmdSeqB.setTimeout(SEQUENCE_TIMEOUT_SECONDS);
    groundLinkHealthProvider.configureRuntime(&groundLinkDriver, &uhfGroundLinkDriver);
    commController.configureRuntime(&groundLinkHealthProvider,
                                    &recoveryExecutor,
                                    &commandIngressAuthority,
                                    &beaconPublisher,
                                    &commEgressMux,
                                    &cspBridge,
                                    state.sbandSubsystemHealthNode,
                                    state.uhfSubsystemHealthNode,
                                    state.commSubsystemFdirConfig,
                                    state.initialCommBand,
                                    state.sbandCommandAuthorityConfig,
                                    state.uhfCommandAuthorityConfig);
    configureTopology();
    loadParameters();
    startTasks(state);
    if (state.groundLinkMode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP &&
        state.groundLinkHost != nullptr && state.groundLinkPort != 0U) {
        groundLinkDriver.configureDirectTcp(state.groundLinkHost, state.groundLinkPort);
    } else if (state.groundLinkMode == OBC::COMM::GroundLinkBackendMode::COMM_CSP && state.enablePrimaryGroundLinkDriver) {
        groundLinkDriver.configureCommCsp(state.commCspNode, cspRuntimeOwner, state.groundLinkHealthSemantics);
    } else {
        groundLinkDriver.clearConfiguration();
    }
    uhfGroundLinkDriver.configureCommCsp(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                         cspRuntimeOwner,
                                         OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    return true;
}

void startRateGroups(const Fw::TimeInterval& interval) {
    linuxTimer.startTimer(interval);
}

void stopRateGroups() {
    linuxTimer.quit();
}

bool startPayloadCspService() {
    return payloadCspService.start();
}

void stopPayloadCspService() {
    payloadCspService.stop();
}

void teardownTopology(const TopologyState& state) {
    payloadCspService.stop();
    static_cast<void>(linuxWatchdogSink.shutdownForRuntime());
    stopTasks(state);
    freeThreads(state);
    cmdSeqA.deallocateBuffer(mallocator);
    cmdSeqB.deallocateBuffer(mallocator);
    dpCatalog.shutdown();
    dpBufferManager.cleanup();
    tearDownComponents(state);
}

}  // namespace OBCApp
