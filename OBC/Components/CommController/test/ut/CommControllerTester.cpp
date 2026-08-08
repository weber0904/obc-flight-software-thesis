#include "CommControllerTester.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthRevocationReasonEnumAc.hpp"
#include "simulators/comm/CommCspProtocol.hpp"

namespace OBC {

namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;

constexpr U8 SBAND_KEY_BYTES[] = {
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
};
constexpr U8 UHF_KEY_BYTES[] = {
    0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x3AU, 0x3BU, 0x3CU, 0x3DU, 0x3EU, 0x3FU,
    0x40U, 0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4AU, 0x4BU, 0x4CU, 0x4DU, 0x4EU, 0x4FU,
};

AuthorityConfig authorityConfigForProfileWithAuth(const char* profile) {
    AuthorityConfig config = authorityConfigFromProfile(profile);
    if (std::strcmp(profile, "sband-primary") == 0) {
        EXPECT_TRUE(configureAuthorityAuth(
            config, 1U, 1U, SBAND_KEY_BYTES, FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES), CommandAuthAlgorithm::HMAC_SHA256));
        return config;
    }

    EXPECT_TRUE(configureAuthorityAuth(
        config, 2U, 2U, UHF_KEY_BYTES, FW_NUM_ARRAY_ELEMENTS(UHF_KEY_BYTES), CommandAuthAlgorithm::HMAC_SHA256));
    return config;
}

std::string makeReliableTransferFile(const char* suffix, std::size_t size) {
    char path[] = "/tmp/comm-controller-rt-XXXXXX";
    const int fd = ::mkstemp(path);
    EXPECT_GE(fd, 0);
    ::close(fd);
    std::string finalPath(path);
    finalPath += suffix;
    EXPECT_EQ(std::rename(path, finalPath.c_str()), 0);
    std::ofstream out(finalPath, std::ios::binary | std::ios::trunc);
    EXPECT_TRUE(out.good());
    for (std::size_t i = 0; i < size; ++i) {
        out.put(static_cast<char>('a' + (i % 26U)));
    }
    out.close();
    return finalPath;
}

void removeIfExists(const std::string& path) {
    (void)std::remove(path.c_str());
}

class ScopedReliableTransferEnv final {
  public:
    explicit ScopedReliableTransferEnv(const char* value)
        : m_hadOriginal(false) {
        const char* original = std::getenv("COMM_RT_OUTPUT_DIR");
        if (original != nullptr) {
            this->m_hadOriginal = true;
            this->m_original = original;
        }
        if (value == nullptr) {
            (void)::unsetenv("COMM_RT_OUTPUT_DIR");
        } else {
            (void)::setenv("COMM_RT_OUTPUT_DIR", value, 1);
        }
    }

    ~ScopedReliableTransferEnv() {
        if (this->m_hadOriginal) {
            (void)::setenv("COMM_RT_OUTPUT_DIR", this->m_original.c_str(), 1);
        } else {
            (void)::unsetenv("COMM_RT_OUTPUT_DIR");
        }
    }

  private:
    bool m_hadOriginal;
    std::string m_original;
};

}  // namespace

FakeCommReliableTransferRuntime::FakeCommReliableTransferRuntime()
    : m_metrics(),
      m_dataFrameCount(0U),
      m_beginCount(0U),
      m_abortCount(0U),
      m_cancelCount(0U),
      m_lastBeginTargetNode(0U),
      m_nextAsyncHandle(1U),
      m_syncPingResponses(),
      m_pendingAsyncPingCompletions(),
      m_asyncPingCompletions() {
    this->m_metrics.initialized = true;
}

OBC::CSP::RuntimeStatus FakeCommReliableTransferRuntime::init(const OBC::CSP::RuntimeConfig&) {
    this->m_metrics.initialized = true;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeCommReliableTransferRuntime::ping(std::uint16_t targetNode, std::uint32_t, bool& success) {
    const auto it = this->m_syncPingResponses.find(targetNode);
    success = it == this->m_syncPingResponses.end() ? true : it->second;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeCommReliableTransferRuntime::sendRaw(std::uint16_t,
                                                                 std::uint8_t targetPort,
                                                                 const std::string& data) {
    if (targetPort != static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_DATA) ||
        data.size() != sizeof(OBC::COMM::CSP::ReliableTransferDataFrame)) {
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }
    this->m_dataFrameCount += 1U;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeCommReliableTransferRuntime::requestReply(std::uint16_t targetNode,
                                                                      std::uint8_t targetPort,
                                                                      const void* requestData,
                                                                      std::size_t requestSize,
                                                                      void* replyData,
                                                                      std::size_t replyCapacity,
                                                                      std::size_t& replySize,
                                                                      std::uint32_t) {
    if (targetPort != static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_CONTROL) ||
        requestSize != sizeof(OBC::COMM::CSP::ReliableTransferControlRequest) ||
        replyCapacity < sizeof(OBC::COMM::CSP::ReliableTransferControlReply)) {
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    OBC::COMM::CSP::ReliableTransferControlRequest request = {};
    std::memcpy(&request, requestData, sizeof(request));
    const auto op = static_cast<OBC::COMM::CSP::ReliableTransferOp>(request.op);
    if (op == OBC::COMM::CSP::ReliableTransferOp::BEGIN) {
        this->m_beginCount += 1U;
        this->m_lastBeginTargetNode = targetNode;
    } else if (op == OBC::COMM::CSP::ReliableTransferOp::ABORT) {
        this->m_abortCount += 1U;
    } else if (op == OBC::COMM::CSP::ReliableTransferOp::CANCEL) {
        this->m_cancelCount += 1U;
    }

    OBC::COMM::CSP::ReliableTransferControlReply reply =
        OBC::COMM::CSP::makeReliableTransferControlReply(request.header.seq, OBC::COMM::CSP::ResultCode::OK, op);
    reply.transferId = request.transferId;
    reply.transferResult = static_cast<std::uint8_t>(
        op == OBC::COMM::CSP::ReliableTransferOp::ABORT
            ? OBC::COMM::CSP::ReliableTransferResult::ABORTED
        : (op == OBC::COMM::CSP::ReliableTransferOp::CANCEL
               ? OBC::COMM::CSP::ReliableTransferResult::CANCELLED
               : OBC::COMM::CSP::ReliableTransferResult::OK));

    replySize = sizeof(reply);
    std::memcpy(replyData, &reply, sizeof(reply));
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeMetrics FakeCommReliableTransferRuntime::metrics() const {
    return this->m_metrics;
}

void FakeCommReliableTransferRuntime::shutdown() {}

std::uint32_t FakeCommReliableTransferRuntime::getDataFrameCount() const {
    return this->m_dataFrameCount;
}

std::uint32_t FakeCommReliableTransferRuntime::getAbortCount() const {
    return this->m_abortCount;
}

std::uint32_t FakeCommReliableTransferRuntime::getBeginCount() const {
    return this->m_beginCount;
}

std::uint16_t FakeCommReliableTransferRuntime::getLastBeginTargetNode() const {
    return this->m_lastBeginTargetNode;
}

void FakeCommReliableTransferRuntime::setSyncPingResponse(std::uint16_t nodeId, bool success) {
    this->m_syncPingResponses[nodeId] = success;
}

void FakeCommReliableTransferRuntime::queueAsyncPingCompletion(std::uint16_t nodeId,
                                                               OBC::CSP::RuntimeStatus status,
                                                               bool success) {
    static_cast<void>(nodeId);
    const std::uint64_t handle = this->m_nextAsyncHandle;
    this->m_pendingAsyncPingCompletions.push_back({handle, {status, success}});
}

bool FakeCommReliableTransferRuntime::completeNextAsyncPing() {
    if (this->m_pendingAsyncPingCompletions.empty()) {
        return false;
    }
    const auto queued = this->m_pendingAsyncPingCompletions.front();
    this->m_pendingAsyncPingCompletions.pop_front();
    OBC::AsyncCspPingCompletion completion = {};
    completion.status = queued.second.status;
    completion.success = queued.second.success;
    this->m_asyncPingCompletions[queued.first] = completion;
    return true;
}

bool FakeCommReliableTransferRuntime::submitAsyncPing(std::uint16_t targetNode,
                                                      std::uint32_t timeoutMs,
                                                      std::uint64_t& handle) {
    static_cast<void>(targetNode);
    static_cast<void>(timeoutMs);
    handle = this->m_nextAsyncHandle++;
    return true;
}

bool FakeCommReliableTransferRuntime::takeAsyncPingCompletion(std::uint64_t handle,
                                                              OBC::AsyncCspPingCompletion& completion) {
    const auto it = this->m_asyncPingCompletions.find(handle);
    if (it == this->m_asyncPingCompletions.end()) {
        return false;
    }
    completion = it->second;
    this->m_asyncPingCompletions.erase(it);
    return true;
}

bool FakeCommReliableTransferRuntime::submitAsyncRequestReply(std::uint16_t targetNode,
                                                              std::uint8_t targetPort,
                                                              const void* requestData,
                                                              std::size_t requestSize,
                                                              std::size_t replyCapacity,
                                                              std::uint32_t timeoutMs,
                                                              std::uint64_t& handle) {
    static_cast<void>(targetNode);
    static_cast<void>(targetPort);
    static_cast<void>(requestData);
    static_cast<void>(requestSize);
    static_cast<void>(replyCapacity);
    static_cast<void>(timeoutMs);
    static_cast<void>(handle);
    return false;
}

bool FakeCommReliableTransferRuntime::takeAsyncRequestReplyCompletion(
    std::uint64_t handle,
    OBC::AsyncCspRequestReplyCompletion& completion) {
    static_cast<void>(handle);
    static_cast<void>(completion);
    return false;
}

FakeCommControllerGroundLinkBackend::FakeCommControllerGroundLinkBackend(OBC::COMM::GroundLinkBackendMode mode)
    : m_stats(),
      m_healthSemantics(mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP
                            ? OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP
                        : (mode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP
                               ? OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK
                               : OBC::COMM::GroundLinkHealthSemantics::DISABLED)),
      m_successfulStatusObservations(0U),
      m_healthReplies() {
    this->m_stats.mode = mode;
}

bool FakeCommControllerGroundLinkBackend::start() {
    return true;
}

void FakeCommControllerGroundLinkBackend::stop() {
    this->m_stats.connected = false;
}

OBC::COMM::GroundLinkReceiveStatus FakeCommControllerGroundLinkBackend::receive(std::string& outChunk,
                                                                                std::uint32_t timeoutMs) {
    static_cast<void>(timeoutMs);
    outChunk.clear();
    return OBC::COMM::GroundLinkReceiveStatus::IDLE;
}

OBC::COMM::GroundLinkSendStatus FakeCommControllerGroundLinkBackend::send(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        this->m_stats.txErrors += 1U;
        return OBC::COMM::GroundLinkSendStatus::ERROR;
    }

    this->m_stats.txChunks += 1U;
    this->m_stats.txBytes += static_cast<std::uint32_t>(size);
    return OBC::COMM::GroundLinkSendStatus::OK;
}

OBC::COMM::GroundLinkStats FakeCommControllerGroundLinkBackend::getStats() const {
    return this->m_stats;
}

OBC::COMM::GroundLinkObservationState FakeCommControllerGroundLinkBackend::getObservationState() const {
    OBC::COMM::GroundLinkObservationState observation = {};
    observation.mode = this->m_stats.mode;
    observation.healthSemantics = this->m_healthSemantics;
    observation.connected = this->m_stats.connected;
    observation.txChunks = this->m_stats.txChunks;
    observation.rxChunks = this->m_stats.rxChunks;
    observation.txBytes = this->m_stats.txBytes;
    observation.rxBytes = this->m_stats.rxBytes;
    observation.txErrors = this->m_stats.txErrors;
    observation.rxErrors = this->m_stats.rxErrors;
    observation.successfulStatusObservations = this->m_successfulStatusObservations;
    return observation;
}

bool FakeCommControllerGroundLinkBackend::observeHealth() {
    if (!this->m_healthReplies.empty()) {
        const HealthReply reply = this->m_healthReplies.front();
        this->m_healthReplies.pop_front();
        this->m_stats.connected = reply.connected;
        if (reply.success) {
            this->m_successfulStatusObservations += 1U;
        }
        return reply.success;
    }

    if (this->m_stats.mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP && this->m_stats.connected) {
        this->m_successfulStatusObservations += 1U;
        return true;
    }

    return this->m_stats.connected;
}

void FakeCommControllerGroundLinkBackend::setConnected(bool connected) {
    this->m_stats.connected = connected;
}

void FakeCommControllerGroundLinkBackend::setMode(OBC::COMM::GroundLinkBackendMode mode) {
    this->m_stats.mode = mode;
    this->m_healthSemantics = mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP
                                  ? OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP
                              : (mode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP
                                     ? OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK
                                     : OBC::COMM::GroundLinkHealthSemantics::DISABLED);
}

void FakeCommControllerGroundLinkBackend::setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics) {
    this->m_healthSemantics = healthSemantics;
}

void FakeCommControllerGroundLinkBackend::addErrors(U32 txErrors, U32 rxErrors) {
    this->m_stats.txErrors += txErrors;
    this->m_stats.rxErrors += rxErrors;
}

void FakeCommControllerGroundLinkBackend::queueHealthObservation(bool success, bool connected) {
    this->m_healthReplies.push_back({success, connected});
}

void CommControllerTester::FakeRecoverySink::submitCommPrimaryUnavailableFault(U32 failureCount) {
    this->commUnavailableFaultCount++;
    this->lastFailureCount = failureCount;
}

void CommControllerTester::FakeRecoverySink::clearCommPrimaryUnavailableFault(U32 failureCount) {
    this->commUnavailableClearCount++;
    this->lastFailureCount = failureCount;
}

void CommControllerTester::FakeRecoverySink::submitCommPrimaryTransportFault(U32 failureCount) {
    this->commTransportFaultCount++;
    this->lastFailureCount = failureCount;
}

void CommControllerTester::FakeRecoverySink::clearCommPrimaryTransportFault(U32 failureCount) {
    this->commTransportClearCount++;
    this->lastFailureCount = failureCount;
}

bool CommControllerTester::FakeCommSubsystemHealthProbe::probeNodeResponsiveForRuntime(U16 nodeId, U32 timeoutMs) {
    static_cast<void>(timeoutMs);
    this->m_probeCounts[nodeId] += 1U;
    const auto it = this->m_nodeResponsive.find(nodeId);
    return it != this->m_nodeResponsive.end() ? it->second : false;
}

void CommControllerTester::FakeCommSubsystemHealthProbe::setNodeResponsive(U16 nodeId, bool responsive) {
    this->m_nodeResponsive[nodeId] = responsive;
}

U32 CommControllerTester::FakeCommSubsystemHealthProbe::getProbeCount(U16 nodeId) const {
    const auto it = this->m_probeCounts.find(nodeId);
    return it != this->m_probeCounts.end() ? it->second : 0U;
}

void CommControllerTester::FakeCommSubsystemHealthProbe::resetProbeCounts() {
    this->m_probeCounts.clear();
}

void CommControllerTester::FakeBeaconSuppressControl::setUhfBeaconSuppressedForRuntime(bool suppressed) {
    this->suppressed = suppressed;
    this->setCalls += 1U;
}

CommControllerTester::CommControllerTester()
    : CommControllerGTestBase("CommControllerTester", MAX_HISTORY_SIZE),
      m_reliableTransferRuntime(),
      component("CommController", this->m_reliableTransferRuntime),
      m_sbandGroundLinkDriver("SbandGroundLinkDriver"),
      m_uhfGroundLinkDriver("UhfGroundLinkDriver"),
      m_groundLinkHealthProvider("GroundLinkHealthProvider"),
      m_commandIngressAuthority("CommandIngressAuthority"),
      m_commEgressMux("CommEgressMux"),
      m_recoverySink(),
      m_sbandBackend(new FakeCommControllerGroundLinkBackend()),
      m_uhfBackend(new FakeCommControllerGroundLinkBackend()),
      m_sendFileResponses(),
      m_nextSendFileContext(1000U) {
    this->initComponents();
    this->connectPorts();
    this->m_sbandGroundLinkDriver.init(TEST_INSTANCE_ID);
    this->m_uhfGroundLinkDriver.init(TEST_INSTANCE_ID);
    this->m_groundLinkHealthProvider.init(TEST_INSTANCE_ID);
    this->m_commandIngressAuthority.init(TEST_INSTANCE_ID);
    this->m_commEgressMux.init(TEST_INSTANCE_ID);
    this->m_sbandGroundLinkDriver.setBackendForTest(this->m_sbandBackend.get());
    this->m_uhfGroundLinkDriver.setBackendForTest(this->m_uhfBackend.get());
    this->m_groundLinkHealthProvider.configureRuntime(&this->m_sbandGroundLinkDriver, &this->m_uhfGroundLinkDriver);
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));
    this->setSubsystemAvailability(false, false);
    this->m_secureAuthInvalidations.clear();
}

CommControllerTester::~CommControllerTester() {
    this->m_sbandGroundLinkDriver.clearConfiguration();
    this->m_uhfGroundLinkDriver.clearConfiguration();
}

void CommControllerTester::testPrimarySwitchRequiresAvailableLink() {
    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);

    this->setLinkAvailability(false, true);
    this->setSubsystemAvailability(false, true);
    this->refreshAvailability();
    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 1, OBC::CommBand::UHF);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_COMM_BAND_SWITCH_SIZE(1);
    ASSERT_EVENTS_COMM_BAND_SWITCH(0, OBC::CommBand::UHF);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED(0, OBC::CommBand::UHF, OBC::CommBand::UHF, OBC::CommBand::UHF, 1U);
    ASSERT_TLM_COMM_PRIMARY_COMMAND_LINK_SIZE(1);
    ASSERT_TLM_COMM_PRIMARY_COMMAND_LINK(0, OBC::CommBand::UHF);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryTelemetryLinkForRuntime(), OBC::CommBand::UHF);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryFileLinkForRuntime(), OBC::CommBand::UHF);
}

void CommControllerTester::testPassLifecycleIsObserveOnly() {
    this->setLinkAvailability(true, false);
    this->refreshAvailability();

    this->clearHistory();
    this->sendCmd_COMM_START_PASS(TEST_INSTANCE_ID, 0, 2U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_START_PASS, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_COMM_PASS_START_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    ASSERT_TLM_COMM_PASS_ACTIVE_SIZE(1);
    ASSERT_TLM_COMM_PASS_ACTIVE(0, true);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    EXPECT_EQ(this->component.getStateForRuntime().primaryFileLink, OBC::CommBand::SBAND);

    this->clearHistory();
    this->tickTimes(2U);

    ASSERT_EVENTS_COMM_PASS_END_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    ASSERT_TLM_COMM_PASS_ACTIVE_SIZE(1);
    ASSERT_TLM_COMM_PASS_ACTIVE(0, false);
    ASSERT_TLM_COMM_PASS_REMAINING_SIZE(1);
    ASSERT_TLM_COMM_PASS_REMAINING(0, 0U);
    EXPECT_EQ(this->component.getStateForRuntime().primaryTelemetryLink, OBC::CommBand::SBAND);
}

void CommControllerTester::testZeroLengthPassRejected() {
    this->clearHistory();
    this->sendCmd_COMM_START_PASS(TEST_INSTANCE_ID, 0, 0U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_START_PASS, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_COMM_PASS_START_SIZE(0);
}

void CommControllerTester::testGatewayDetachDoesNotFaultHealthyPrimarySubsystem() {
    this->setLinkAvailability(false, true);
    this->setSubsystemAvailability(true, true);
    this->clearHistory();
    this->tickTimes(3U);

    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);
    EXPECT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    EXPECT_EQ(this->component.getStateForRuntime().fdirFaultKind, OBC::CommFdirFaultKind::NONE);
    EXPECT_EQ(this->component.getStateForRuntime().consecutivePrimaryUnavailable, 0U);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
}

void CommControllerTester::testSbandLossFailsOverAndRestoreReturnsPrimary() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->tickTimes(8U);

    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);
    EXPECT_EQ(this->m_recoverySink.lastFailureCount, 3U);

    this->clearHistory();
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, true);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_EQ(this->m_recoverySink.commUnavailableClearCount, 1U);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryFileLinkForRuntime(), OBC::CommBand::SBAND);
}

void CommControllerTester::testSimultaneousSbandLossAndUhfRestoreFailsOver() {
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    EXPECT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfAvailable);

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->refreshAvailability();

    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfAvailable);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);

    this->tickTimes(7U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
    EXPECT_FALSE(this->component.getStateForRuntime().sbandAvailable);
    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);

    this->clearHistory();
    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();
    ASSERT_TRUE(result.switched);
    ASSERT_FALSE(result.noHealthyBackup);
    ASSERT_EQ(result.finalPrimaryCommandLink, OBC::CommBand::UHF);
    ASSERT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::UHF);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::UHF, true);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED(0, OBC::CommBand::UHF, OBC::CommBand::UHF, OBC::CommBand::UHF, 2U);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryTelemetryLinkForRuntime(), OBC::CommBand::UHF);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryFileLinkForRuntime(), OBC::CommBand::UHF);
}

void CommControllerTester::testOperatorSelectedUhfPrimaryDoesNotAutoRestoreOnSbandRecovery() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::UHF);

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->refreshAvailability();
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::UHF);

    this->clearHistory();
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::UHF);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryTelemetryLinkForRuntime(), OBC::CommBand::UHF);
    EXPECT_EQ(this->m_commEgressMux.getPrimaryFileLinkForRuntime(), OBC::CommBand::UHF);
}

void CommControllerTester::testUnavailableFaultClearsOnHealthyReconnectDespiteTransportNoise() {
    this->m_sbandBackend->setMode(OBC::COMM::GroundLinkBackendMode::DIRECT_TCP);
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->setLinkAvailability(false, true);
    this->tickTimes(3U);

    ASSERT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);
    ASSERT_EQ(this->component.getStateForRuntime().fdirFaultKind, OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE);

    this->clearHistory();
    this->setLinkAvailability(true, true);
    this->m_sbandBackend->addErrors(1U, 0U);
    this->refreshAvailability();

    ASSERT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.commUnavailableClearCount, 1U);
    ASSERT_EQ(this->component.getStateForRuntime().fdirFaultKind, OBC::CommFdirFaultKind::NONE);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(0);
}

void CommControllerTester::testUhfPrimaryDemotesSbandIngressAuthority() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    this->m_secureAuthInvalidations.clear();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);

    const AuthorityConfig sbandBackup = this->m_commandIngressAuthority.getIngressConfigForRuntime(0);
    const AuthorityConfig uhfPrimary = this->m_commandIngressAuthority.getIngressConfigForRuntime(1);
    EXPECT_EQ(sbandBackup.identity, AuthorityLinkIdentity::SBAND);
    EXPECT_EQ(sbandBackup.role, AuthorityLinkRole::BACKUP);
    EXPECT_EQ(uhfPrimary.identity, AuthorityLinkIdentity::UHF);
    EXPECT_EQ(uhfPrimary.role, AuthorityLinkRole::PRIMARY_AFTER_FAILOVER);

    const AuthorityDecision sbandDenied = evaluateCommandAuthority(sbandBackup, OPCODE_MODE_SET);
    EXPECT_FALSE(sbandDenied.allow);
    EXPECT_EQ(sbandDenied.reason, AuthorityRejectReason::POLICY_DENIED);
    EXPECT_TRUE(evaluateCommandAuthority(sbandBackup, OPCODE_MODE_GET).allow);
    EXPECT_TRUE(evaluateCommandAuthority(uhfPrimary, OPCODE_MODE_SET).allow);
    ASSERT_EQ(this->m_secureAuthInvalidations.size(), 2U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_ingressPort(), 0U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_serviceId(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[1].get_ingressPort(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[1].get_serviceId(), 2U);

    this->clearHistory();
    this->m_secureAuthInvalidations.clear();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 1, OBC::CommBand::SBAND);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 1, Fw::CmdResponse::OK);

    const AuthorityConfig sbandPrimary = this->m_commandIngressAuthority.getIngressConfigForRuntime(0);
    const AuthorityConfig uhfBackup = this->m_commandIngressAuthority.getIngressConfigForRuntime(1);
    EXPECT_EQ(sbandPrimary.identity, AuthorityLinkIdentity::SBAND);
    EXPECT_EQ(sbandPrimary.role, AuthorityLinkRole::PRIMARY);
    EXPECT_EQ(uhfBackup.identity, AuthorityLinkIdentity::UHF);
    EXPECT_EQ(uhfBackup.role, AuthorityLinkRole::BACKUP);
    EXPECT_TRUE(evaluateCommandAuthority(sbandPrimary, OPCODE_MODE_SET).allow);
    EXPECT_FALSE(evaluateCommandAuthority(uhfBackup, OPCODE_MODE_SET).allow);
    ASSERT_EQ(this->m_secureAuthInvalidations.size(), 2U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_serviceId(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[1].get_serviceId(), 2U);
}

void CommControllerTester::testDpActiveRejectsAdditionalDp() {
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 41U));
    this->clearHistory();
    const Svc::SendFileResponse first =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp-1.bin"), Fw::FileNameString("dp-1.bin"), 0U, 0U);
    ASSERT_EQ(first.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_NE(first.get_context(), 0U);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString("dp-1.bin"), Fw::FileNameString("dp-1.bin"), 0U, 0U);

    const Svc::SendFileResponse second =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp-2.bin"), Fw::FileNameString("dp-2.bin"), 0U, 0U);
    ASSERT_EQ(second.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL_SIZE(1);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL(0, 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkPendingOwner, static_cast<U32>(CommDownlinkOwner::NONE));
}

void CommControllerTester::testInlineDpLaunchFailureDoesNotEmitCompletion() {
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, 601U));
    this->clearHistory();
    const Svc::SendFileResponse dpResponse =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);

    ASSERT_EQ(dpResponse.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    ASSERT_NE(dpResponse.get_context(), 0U);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    ASSERT_from_dpFileCompleteOut_SIZE(0);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL_SIZE(1);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL(0, 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkPendingOwner, static_cast<U32>(CommDownlinkOwner::NONE));
}

void CommControllerTester::testDpCompletionClearsOwner() {
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 81U));
    this->clearHistory();
    const Svc::SendFileResponse dpRequest =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    ASSERT_EQ(dpRequest.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_NE(dpRequest.get_context(), 0U);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkPendingOwner, static_cast<U32>(CommDownlinkOwner::NONE));

    this->clearHistory();
    this->invoke_to_fileCompleteIn(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 81U));
    ASSERT_from_dpFileCompleteOut_SIZE(1);
    ASSERT_from_dpFileCompleteOut(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, dpRequest.get_context()));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkPendingOwner, static_cast<U32>(CommDownlinkOwner::NONE));
}

void CommControllerTester::testPrimarySwitchDropsActiveDownlinkAndNotifiesDp() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 111U));
    const Svc::SendFileResponse dpRequest =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    ASSERT_EQ(dpRequest.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_NE(dpRequest.get_context(), 0U);

    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 1, OBC::CommBand::UHF);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_COMM_DOWNLINK_STATE_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_DOWNLINK_STATE_CHANGED(
        0, static_cast<U32>(CommDownlinkOwner::NONE), static_cast<U32>(CommDownlinkOwner::NONE), 7U);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED(0, OBC::CommBand::UHF, OBC::CommBand::UHF, OBC::CommBand::UHF, 1U);
    ASSERT_from_dpFileCompleteOut_SIZE(1);
    ASSERT_from_dpFileCompleteOut(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, dpRequest.get_context()));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkPendingOwner, static_cast<U32>(CommDownlinkOwner::NONE));
}

void CommControllerTester::testUhfPrimaryActiveDownlinkSuspendsPacketEgress() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    EXPECT_FALSE(this->m_commEgressMux.getUhfFileTransferActiveForRuntime());

    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 123U));
    this->clearHistory();
    const Svc::SendFileResponse dpResponse =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    ASSERT_EQ(dpResponse.get_status(), Svc::SendFileStatus::STATUS_OK);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));
    EXPECT_TRUE(this->m_commEgressMux.getUhfFileTransferActiveForRuntime());
    ASSERT_from_sendFileOut_SIZE(0);

    this->tickTimes(13U);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);

    this->invoke_to_fileCompleteIn(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 123U));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));
    EXPECT_FALSE(this->m_commEgressMux.getUhfFileTransferActiveForRuntime());
}

void CommControllerTester::testUhfPrimaryDeferredLaunchDropsOnLinkLoss() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    this->clearHistory();
    const Svc::SendFileResponse dpResponse =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString("dp.bin"), Fw::FileNameString("dp.bin"), 0U, 0U);
    ASSERT_EQ(dpResponse.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));

    this->setSubsystemAvailability(false, false);
    this->clearHistory();
    this->tickTimes(8U);

    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::UHF, false);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);
    ASSERT_from_sendFileOut_SIZE(0);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));

    this->clearHistory();
    this->m_secureAuthInvalidations.clear();
    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();
    ASSERT_TRUE(result.noHealthyBackup);
    ASSERT_EQ(result.ownersCleared, 1U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
    ASSERT_EVENTS_COMM_DOWNLINK_STATE_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_DOWNLINK_STATE_CHANGED(
        0, static_cast<U32>(CommDownlinkOwner::NONE), static_cast<U32>(CommDownlinkOwner::NONE), 6U);
    ASSERT_EQ(this->m_secureAuthInvalidations.size(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_ingressPort(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_serviceId(), 2U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_reason(), static_cast<U32>(SecureAuthRevocationReason::INVALIDATED));
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));
}

void CommControllerTester::testReliableTransferAdmissionUsesHelper() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    this->clearHistory();
    const Svc::SendFileResponse response =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);

    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(1);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED(0, response.get_context(), static_cast<U32>(CommDownlinkOwner::DP_CATALOG));
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(1);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED(0, 1U, 256U, 2U);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID_SIZE(1);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID(0, 1U);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 1U);
    EXPECT_EQ(this->m_reliableTransferRuntime.getLastBeginTargetNode(), OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
    EXPECT_EQ(this->m_reliableTransferRuntime.getDataFrameCount(), 0U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferSwitchedUhfPrimaryUsesHelper() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    this->clearHistory();
    const Svc::SendFileResponse response =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);

    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(0);

    this->tickTimes(13U);

    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(1);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED(0, response.get_context(), static_cast<U32>(CommDownlinkOwner::DP_CATALOG));
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(1);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED(0, 1U, 256U, 2U);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID_SIZE(1);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID(0, 1U);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 1U);
    EXPECT_EQ(this->m_reliableTransferRuntime.getLastBeginTargetNode(), OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferFallsBackWhenReceiverDisabled() {
    const ScopedReliableTransferEnv reliableTransferDisabled(nullptr);
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    this->clearHistory();
    const Svc::SendFileResponse response =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);

    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(0);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 0U);

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferBootstrapUhfPrimaryFallsBackToStockDownlink() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::UHF,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    ASSERT_EQ(this->component.getStateForRuntime().primaryFileLink, OBC::CommBand::UHF);

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 333U));
    this->clearHistory();
    const Svc::SendFileResponse response =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);

    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);

    this->tickTimes(13U);

    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(0);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 0U);

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferFailoverToUhfFallsBackToStockDownlink() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(false, true);
    this->setSubsystemAvailability(false, true);
    this->refreshAvailability();

    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();
    ASSERT_TRUE(result.switched);
    ASSERT_EQ(this->component.getStateForRuntime().primaryFileLink, OBC::CommBand::UHF);

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    this->queueSendFileResponse(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 222U));
    this->clearHistory();
    const Svc::SendFileResponse response =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);

    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);

    this->tickTimes(13U);

    ASSERT_from_sendFileOut_SIZE(1);
    ASSERT_from_sendFileOut(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EVENTS_COMM_RT_ROUTE_SELECTED_SIZE(0);
    ASSERT_EVENTS_COMM_RT_TRANSFER_STARTED_SIZE(0);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 0U);

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferBusyRejectsWhileActive() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    const std::string firstPath = makeReliableTransferFile(".fdp", 256U);
    const std::string secondPath = makeReliableTransferFile(".fdp", 128U);
    ASSERT_EQ(this->invoke_to_dpFileRequestIn(0,
                                              Fw::FileNameString(firstPath.c_str()),
                                              Fw::FileNameString("hk-1.fdp"),
                                              0U,
                                              0U)
                  .get_status(),
              Svc::SendFileStatus::STATUS_OK);

    this->clearHistory();
    const Svc::SendFileResponse second =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(secondPath.c_str()), Fw::FileNameString("hk-2.fdp"), 0U, 0U);

    ASSERT_EQ(second.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    ASSERT_from_sendFileOut_SIZE(0);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL_SIZE(1);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL(0, 1U);
    EXPECT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::DP_CATALOG));

    removeIfExists(firstPath);
    removeIfExists(secondPath);
}

void CommControllerTester::testReliableTransferPrimarySwitchAbortsActiveTransfer() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    const Svc::SendFileResponse request =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EQ(request.get_status(), Svc::SendFileStatus::STATUS_OK);

    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    ASSERT_from_dpFileCompleteOut_SIZE(1);
    ASSERT_from_dpFileCompleteOut(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, request.get_context()));
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT_SIZE(1);
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT(0, 1U, static_cast<U32>(CommReliableTransferResult::ABORTED), 0U, 0U);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID_SIZE(1);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID(0, 0U);
    ASSERT_TLM_COMM_RT_LAST_RESULT_SIZE(1);
    ASSERT_TLM_COMM_RT_LAST_RESULT(0, static_cast<U32>(CommReliableTransferResult::ABORTED));
    EXPECT_EQ(this->m_reliableTransferRuntime.getAbortCount(), 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferFailoverAbortsActiveTransfer() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    const Svc::SendFileResponse request =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EQ(request.get_status(), Svc::SendFileStatus::STATUS_OK);

    this->m_sbandBackend->setConnected(false);
    this->m_uhfBackend->setConnected(false);
    this->setSubsystemAvailability(false, false);
    this->clearHistory();
    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();

    ASSERT_TRUE(result.noHealthyBackup);
    ASSERT_EQ(result.ownersCleared, 1U);
    ASSERT_from_dpFileCompleteOut_SIZE(1);
    ASSERT_from_dpFileCompleteOut(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, request.get_context()));
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT_SIZE(1);
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT(0, 1U, static_cast<U32>(CommReliableTransferResult::ABORTED), 0U, 0U);
    ASSERT_TLM_COMM_RT_LAST_RESULT_SIZE(1);
    ASSERT_TLM_COMM_RT_LAST_RESULT(0, static_cast<U32>(CommReliableTransferResult::ABORTED));
    EXPECT_EQ(this->m_reliableTransferRuntime.getAbortCount(), 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));

    removeIfExists(path);
}

void CommControllerTester::testReliableTransferUhfFailoverAbortsActiveTransfer() {
    const ScopedReliableTransferEnv reliableTransferEnabled("/tmp/comm-controller-rt-output");
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);

    const std::string path = makeReliableTransferFile(".fdp", 256U);
    const Svc::SendFileResponse request =
        this->invoke_to_dpFileRequestIn(0, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EQ(request.get_status(), Svc::SendFileStatus::STATUS_OK);

    this->tickTimes(13U);
    ASSERT_EQ(this->m_reliableTransferRuntime.getBeginCount(), 1U);
    ASSERT_EQ(this->m_reliableTransferRuntime.getLastBeginTargetNode(), OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID);

    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->clearHistory();
    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();

    ASSERT_TRUE(result.switched);
    ASSERT_EQ(result.finalPrimaryFileLink, OBC::CommBand::SBAND);
    ASSERT_from_dpFileCompleteOut_SIZE(1);
    ASSERT_from_dpFileCompleteOut(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, request.get_context()));
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT_SIZE(1);
    ASSERT_EVENTS_COMM_RT_FINAL_RESULT(0, 1U, static_cast<U32>(CommReliableTransferResult::ABORTED), 0U, 0U);
    ASSERT_TLM_COMM_RT_LAST_RESULT_SIZE(1);
    ASSERT_TLM_COMM_RT_LAST_RESULT(0, static_cast<U32>(CommReliableTransferResult::ABORTED));
    EXPECT_EQ(this->m_reliableTransferRuntime.getAbortCount(), 1U);
    EXPECT_EQ(this->component.getStateForRuntime().downlinkActiveOwner, static_cast<U32>(CommDownlinkOwner::NONE));

    removeIfExists(path);
}

Svc::SendFileResponse CommControllerTester::from_sendFileOut_handler(FwIndexType portNum,
                                                                     const Fw::StringBase& sourceFileName,
                                                                     const Fw::StringBase& destFileName,
                                                                     U32 offset,
                                                                     U32 length) {
    this->pushFromPortEntry_sendFileOut(sourceFileName, destFileName, offset, length);
    static_cast<void>(portNum);
    if (!this->m_sendFileResponses.empty()) {
        const Svc::SendFileResponse response = this->m_sendFileResponses.front();
        this->m_sendFileResponses.pop_front();
        return response;
    }
    return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, this->m_nextSendFileContext++);
}

void CommControllerTester::from_secureAuthInvalidateOut_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_secureAuthInvalidateOut(revocation);
    this->m_secureAuthInvalidations.push_back(revocation);
}

void CommControllerTester::from_filePolicyOut_handler(FwIndexType portNum, const FileIngressPolicyState& policy) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_filePolicyOut(policy);
    this->m_filePolicies.push_back(policy);
}

void CommControllerTester::from_commStatusRefreshTlmOut_handler(FwIndexType portNum,
                                                                FwChanIdType id,
                                                                Fw::Time& timeTag,
                                                                Fw::TlmBuffer& val) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_commStatusRefreshTlmOut(id, timeTag, val);
    this->dispatchTlm(id, timeTag, val);
}

void CommControllerTester::setLinkAvailability(bool sbandConnected, bool uhfConnected) {
    this->m_sbandBackend->setConnected(sbandConnected);
    this->m_uhfBackend->setConnected(uhfConnected);
}

void CommControllerTester::setSubsystemAvailability(bool sbandResponsive, bool uhfResponsive) {
    this->m_commSubsystemHealthProbe.setNodeResponsive(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, sbandResponsive);
    this->m_commSubsystemHealthProbe.setNodeResponsive(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID, uhfResponsive);
}

void CommControllerTester::refreshAvailability() {
    this->m_groundLinkHealthProvider.tickForTest();
    this->component.tickForTest();
}

void CommControllerTester::tickTimes(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->m_groundLinkHealthProvider.tickForTest();
        this->component.tickForTest();
    }
}

void CommControllerTester::queueSendFileResponse(const Svc::SendFileResponse& response) {
    this->m_sendFileResponses.push_back(response);
}

void CommControllerTester::testPrimarySubsystemPingFailureTriggersUnavailableFault() {
    this->m_sbandBackend->setConnected(false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();
    this->clearHistory();

    this->setSubsystemAvailability(false, false);
    this->tickTimes(8U);

    ASSERT_FALSE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
}

void CommControllerTester::testDirectTcpPrimaryDoesNotGoStale() {
    this->m_sbandBackend->setMode(OBC::COMM::GroundLinkBackendMode::DIRECT_TCP);
    this->m_sbandBackend->setConnected(true);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();
    this->tickTimes(3U);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    ASSERT_TRUE(state.sbandAvailable);
    ASSERT_EQ(state.sbandAvailabilityReason, OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);
}

void CommControllerTester::testSubsystemPrimaryIgnoresGroundTransportGrowth() {
    this->m_sbandBackend->setConnected(false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    this->m_sbandBackend->addErrors(1U, 1U);
    this->tickTimes(3U);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    ASSERT_TRUE(state.sbandAvailable);
    ASSERT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);
    ASSERT_EQ(state.fdirFaultKind, OBC::CommFdirFaultKind::NONE);
}

void CommControllerTester::testConnectedOnlyCompatibilityDoesNotTriggerTransportFault() {
    this->m_sbandBackend->setMode(OBC::COMM::GroundLinkBackendMode::COMM_CSP);
    this->m_sbandBackend->setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK);
    this->m_sbandBackend->setConnected(true);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    this->m_sbandBackend->addErrors(1U, 1U);
    this->tickTimes(3U);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    ASSERT_TRUE(state.sbandAvailable);
    ASSERT_EQ(state.sbandAvailabilityReason, OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK);
    ASSERT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    ASSERT_EQ(state.fdirFaultKind, OBC::CommFdirFaultKind::NONE);
}

void CommControllerTester::testSecondarySubsystemProbeIsOnDemandOnly() {
    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(true, true);
    this->m_commSubsystemHealthProbe.resetProbeCounts();

    this->tickTimes(2U);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID), 1U);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID), 0U);

    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID), 1U);
}

void CommControllerTester::testPrimarySubsystemProbeUsesCadence() {
    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(true, false);
    this->m_commSubsystemHealthProbe.resetProbeCounts();

    this->tickTimes(5U);

    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID), 2U);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID), 0U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
}

void CommControllerTester::testPrimarySubsystemSingleProbeFailureIsDebounced() {
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    subsystemFdirConfig.primaryProbePeriodTicks = 1U;
    subsystemFdirConfig.primaryProbeFailureThreshold = 2U;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();
    this->clearHistory();

    this->setSubsystemAvailability(false, false);
    this->tickTimes(1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);

    this->tickTimes(1U);
    ASSERT_FALSE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
}

void CommControllerTester::testPrimaryFailbackForceProbeClearsDebounceState() {
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    subsystemFdirConfig.primaryProbePeriodTicks = 1U;
    subsystemFdirConfig.primaryProbeFailureThreshold = 2U;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    EXPECT_TRUE(this->component.getStateForRuntime().sbandAvailable);

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->tickTimes(1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID), 2U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);

    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    ASSERT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::UHF);
    const U32 sbandProbesAfterSwitch =
        this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
    ASSERT_EQ(sbandProbesAfterSwitch, 2U);

    this->setSubsystemAvailability(true, true);
    this->clearHistory();
    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 1, OBC::CommBand::SBAND);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 1, Fw::CmdResponse::OK);
    ASSERT_EQ(this->component.getStateForRuntime().primaryCommandLink, OBC::CommBand::SBAND);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID),
              sbandProbesAfterSwitch + 1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_PRIMARY_LINK_CHANGED(0, OBC::CommBand::SBAND, OBC::CommBand::SBAND, OBC::CommBand::SBAND, 1U);

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->tickTimes(1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID),
              sbandProbesAfterSwitch + 2U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);

    this->tickTimes(1U);
    ASSERT_FALSE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID),
              sbandProbesAfterSwitch + 3U);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED(0, OBC::CommBand::SBAND, false);
}

void CommControllerTester::testForcedProbeDiscardsStaleAsyncCompletion() {
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    subsystemFdirConfig.primaryProbePeriodTicks = 1U;
    subsystemFdirConfig.primaryProbeFailureThreshold = 1U;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);

    this->component.configureCspRuntimeForRuntime(this->m_reliableTransferRuntime);
    this->m_reliableTransferRuntime.queueAsyncPingCompletion(
        OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, OBC::CSP::RuntimeStatus::OK, false);

    this->clearHistory();
    this->tickTimes(1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);

    const OBC::RecoveryCommActionResult recovery = this->component.performRecoveryLinkFailoverForRuntime();
    ASSERT_TRUE(recovery.alreadyOnHealthyPrimary);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);

    this->clearHistory();
    ASSERT_TRUE(this->m_reliableTransferRuntime.completeNextAsyncPing());
    this->tickTimes(1U);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_EVENTS_COMM_LINK_AVAILABILITY_CHANGED_SIZE(0);
}

void CommControllerTester::testDisabledGroundLinkConfigDoesNotLatchUnavailableFault() {
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.unavailableFailureThreshold = 0U;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     0U,
                                     0U,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(false, false);
    this->refreshAvailability();

    this->clearHistory();
    this->tickTimes(5U);

    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);
    EXPECT_EQ(this->m_recoverySink.commTransportFaultCount, 0U);
    EXPECT_FALSE(this->component.getStateForRuntime().fdirFaultLatched);
}

void CommControllerTester::testSbandLiveObservabilityStartsQuietUntilAuth() {
    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommControllerTester::testSbandLiveObservabilityOpensAfterAcceptedSession() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    const OBC::CommRuntimeState opened = this->component.getStateForRuntime();
    EXPECT_TRUE(opened.sbandLiveObservabilityActive);
    EXPECT_EQ(opened.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::ACTIVE_AUTHENTICATED_SESSION);
    EXPECT_EQ(opened.sbandLiveObservabilityIngressPort, 0U);
    EXPECT_EQ(opened.sbandLiveObservabilityRole, AuthorityLinkRole::PRIMARY);
    EXPECT_EQ(opened.sbandLiveObservabilitySessionId, 77U);
    EXPECT_EQ(opened.sbandLiveObservabilityLastAcceptedSequence, 0U);
    EXPECT_TRUE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));

    this->component.onCommandSessionActivityForRuntime(0, sbandConfig, 77U, 5U);
    const OBC::CommRuntimeState refreshed = this->component.getStateForRuntime();
    EXPECT_EQ(refreshed.sbandLiveObservabilityLastAcceptedSequence, 5U);
}

void CommControllerTester::testSbandLiveObservabilityActivityDoesNotRepublishFullCommState() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    this->clearHistory();

    this->component.onCommandSessionActivityForRuntime(0, sbandConfig, 77U, 5U);

    ASSERT_TLM_COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE_SIZE(1);
    ASSERT_TLM_COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE(0, 5U);
    ASSERT_TLM_COMM_PASS_REMAINING_SIZE(0);
    ASSERT_TLM_COMM_TOTAL_PASSES_SIZE(0);
    ASSERT_TLM_COMM_S_BAND_ACTIVITY_AGE_TICKS_SIZE(0);
    ASSERT_TLM_COMM_UHF_BEACON_SUPPRESS_LAST_SEQUENCE_SIZE(0);
}

void CommControllerTester::testSbandLiveObservabilityIgnoresLegacySessionOpen() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, false, false);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommControllerTester::testSbandLiveObservabilityClosesWhenSecureSessionIsReplacedByLegacySession() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandLiveObservabilityActive);

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 88U, 0U, false, true);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_EQ(state.sbandLiveObservabilityLastAcceptedSequence, 0U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommControllerTester::testSbandLiveObservabilityClosesOnSessionRevoke() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandLiveObservabilityActive);

    this->component.onCommandSessionRevokedForRuntime(0, sbandConfig, 77U, 6U, 9U);
    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_EQ(state.sbandLiveObservabilityLastAcceptedSequence, 6U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommControllerTester::testSbandLiveObservabilityClosesOnSilentRecoveryRevoke() {
    this->setLinkAvailability(true, false);
    this->setSubsystemAvailability(true, false);
    this->refreshAvailability();

    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");
    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandLiveObservabilityActive);

    this->setLinkAvailability(false, false);
    this->setSubsystemAvailability(false, false);
    this->clearHistory();
    this->m_secureAuthInvalidations.clear();

    const OBC::RecoveryCommActionResult result = this->component.performRecoveryLinkFailoverForRuntime();
    ASSERT_TRUE(result.noHealthyBackup);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
    ASSERT_EQ(this->m_secureAuthInvalidations.size(), 1U);
    EXPECT_EQ(this->m_secureAuthInvalidations[0].get_ingressPort(), 0U);
}

void CommControllerTester::testSbandLiveObservabilityClosesOnBandSwitch() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");
    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 77U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().sbandLiveObservabilityActive);

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.sbandLiveObservabilityActive);
    EXPECT_EQ(state.sbandLiveObservabilityReason, OBC::CommLiveObservabilityStateReason::INACTIVE_NON_PRIMARY_BAND);
    EXPECT_EQ(state.sbandLiveObservabilitySessionId, 0U);
    EXPECT_FALSE(this->m_commEgressMux.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommControllerTester::testUhfBeaconSuppressStartsAfterAcceptedSessionOpen() {
    const AuthorityConfig uhfConfig = authorityConfigForProfileWithAuth("uhf-backup");
    this->clearHistory();

    this->component.onCommandSessionActivityForRuntime(1, uhfConfig, 42U, 1U);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);

    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 42U, 0U, true, false);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_FALSE(state.uhfPrimaryPacketQuietActive);
    EXPECT_TRUE(state.uhfBeaconSuppressActive);
    EXPECT_EQ(state.uhfBeaconSuppressIngressPort, 1U);
    EXPECT_EQ(state.uhfBeaconSuppressRole, AuthorityLinkRole::BACKUP);
    EXPECT_EQ(state.uhfBeaconSuppressSessionId, 42U);
    EXPECT_EQ(state.uhfBeaconSuppressLastAcceptedSequence, 0U);
    EXPECT_EQ(state.uhfBeaconSuppressRemainingTicks, OBC::CommController::UHF_BEACON_SUPPRESS_TIMEOUT_TICKS);
    EXPECT_TRUE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfBeaconSuppressRefreshesAndTimesOut() {
    const AuthorityConfig uhfConfig = authorityConfigForProfileWithAuth("uhf-backup");
    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 42U, 0U, true, false);

    this->tickTimes(30U);
    EXPECT_EQ(this->component.getStateForRuntime().uhfBeaconSuppressRemainingTicks, 30U);

    this->component.onCommandSessionActivityForRuntime(1, uhfConfig, 42U, 5U);
    EXPECT_EQ(this->component.getStateForRuntime().uhfBeaconSuppressRemainingTicks,
              OBC::CommController::UHF_BEACON_SUPPRESS_TIMEOUT_TICKS);
    EXPECT_EQ(this->component.getStateForRuntime().uhfBeaconSuppressLastAcceptedSequence, 5U);

    this->tickTimes(59U);
    EXPECT_TRUE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    this->tickTimes(1U);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfBeaconSuppressRoleSwitchClearsImmediately() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    const AuthorityConfig uhfConfig = authorityConfigForProfileWithAuth("uhf-backup");
    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 42U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().uhfBeaconSuppressActive);

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfBeaconSuppressSessionReplaceRebindsOwner() {
    const AuthorityConfig uhfConfig = authorityConfigForProfileWithAuth("uhf-backup");
    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 42U, 0U, true, false);
    ASSERT_TRUE(this->component.getStateForRuntime().uhfBeaconSuppressActive);

    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 77U, 0U, true, true);

    const OBC::CommRuntimeState state = this->component.getStateForRuntime();
    EXPECT_TRUE(state.uhfBeaconSuppressActive);
    EXPECT_EQ(state.uhfBeaconSuppressSessionId, 77U);
    EXPECT_EQ(state.uhfBeaconSuppressLastAcceptedSequence, 0U);
    EXPECT_TRUE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfBeaconSuppressIgnoresNonQualifyingActivity() {
    const AuthorityConfig sbandConfig = authorityConfigForProfileWithAuth("sband-primary");
    const AuthorityConfig devConfig = authorityConfigFromProfile("dev-direct");

    this->component.onCommandSessionOpenedForRuntime(0, sbandConfig, 99U, 0U, true, false);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);

    this->component.onCommandSessionOpenedForRuntime(1, devConfig, 11U, 0U, true, false);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);

    this->component.onCommandSessionRevokedForRuntime(1, devConfig, 11U, 0U, 7U);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfPrimaryBandLeavesPacketEgressNonQuiet() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();

    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 0, OBC::CommBand::UHF);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_SET_ACTIVE, 0, Fw::CmdResponse::OK);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);

    AuthorityConfig uhfPrimaryConfig = authorityConfigForProfileWithAuth("uhf-backup");
    uhfPrimaryConfig.role = AuthorityLinkRole::PRIMARY_AFTER_FAILOVER;
    this->component.onCommandSessionOpenedForRuntime(1, uhfPrimaryConfig, 42U, 0U, true, false);

    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_TRUE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_TRUE(this->m_beaconSuppressControl.suppressed);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());
    EXPECT_FALSE(this->m_commEgressMux.getDiagnosticQuietPacketEgressForRuntime());

    this->sendCmd_COMM_SET_ACTIVE(TEST_INSTANCE_ID, 1, OBC::CommBand::SBAND);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_COMM_SET_ACTIVE, 1, Fw::CmdResponse::OK);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);
}

void CommControllerTester::testUhfBackupSuppressDoesNotEnablePacketQuietWhileSbandPrimary() {
    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());

    const AuthorityConfig uhfConfig = authorityConfigForProfileWithAuth("uhf-backup");
    this->component.onCommandSessionOpenedForRuntime(1, uhfConfig, 42U, 0U, true, false);

    EXPECT_FALSE(this->component.getStateForRuntime().uhfPrimaryPacketQuietActive);
    EXPECT_TRUE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_TRUE(this->m_beaconSuppressControl.suppressed);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());

    this->component.onCommandSessionRevokedForRuntime(1, uhfConfig, 42U, 0U, 7U);
    EXPECT_FALSE(this->component.getStateForRuntime().uhfBeaconSuppressActive);
    EXPECT_FALSE(this->m_beaconSuppressControl.suppressed);
    EXPECT_FALSE(this->m_commEgressMux.getUhfPrimaryPacketQuietForRuntime());
}

void CommControllerTester::testGetStatusRepublishesCorePostureWhenValuesUnchanged() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    this->clearHistory();

    this->sendCmd_COMM_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_GET_STATUS, 0, Fw::CmdResponse::OK);
    ASSERT_TLM_COMM_ACTIVE_BAND_SIZE(1);
    ASSERT_TLM_COMM_PRIMARY_COMMAND_LINK_SIZE(1);
    ASSERT_TLM_COMM_PRIMARY_TELEMETRY_LINK_SIZE(1);
    ASSERT_TLM_COMM_PRIMARY_FILE_LINK_SIZE(1);
    ASSERT_TLM_COMM_S_BAND_AVAILABLE_SIZE(1);
    ASSERT_TLM_COMM_UHF_AVAILABLE_SIZE(1);
    ASSERT_TLM_COMM_S_BAND_AVAILABILITY_REASON_SIZE(1);
    ASSERT_TLM_COMM_UHF_AVAILABILITY_REASON_SIZE(1);
    ASSERT_TLM_COMM_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_COMM_FDIR_FAULT_KIND_SIZE(1);
}

void CommControllerTester::testGetStatusDoesNotRepublishDeferredOrDiagnosticTelemetry() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    this->clearHistory();

    this->sendCmd_COMM_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_TLM_COMM_PASS_ACTIVE_SIZE(0);
    ASSERT_TLM_COMM_PASS_REMAINING_SIZE(0);
    ASSERT_TLM_COMM_TOTAL_PASSES_SIZE(0);
    ASSERT_TLM_COMM_DOWNLINK_ACTIVE_OWNER_SIZE(0);
    ASSERT_TLM_COMM_DOWNLINK_PENDING_OWNER_SIZE(0);
    ASSERT_TLM_COMM_SESSION_REVOKE_TOTAL_SIZE(0);
    ASSERT_TLM_COMM_DOWNLINK_REJECT_TOTAL_SIZE(0);
    ASSERT_TLM_COMM_RT_ACTIVE_TRANSFER_ID_SIZE(0);
    ASSERT_TLM_COMM_RT_LAST_ACK_SEGMENT_SIZE(0);
    ASSERT_TLM_COMM_RT_ACKED_BYTES_SIZE(0);
    ASSERT_TLM_COMM_RT_RESEND_TOTAL_SIZE(0);
    ASSERT_TLM_COMM_RT_LAST_RESULT_SIZE(0);
    ASSERT_TLM_COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE_SIZE(0);
    ASSERT_TLM_COMM_UHF_BEACON_SUPPRESS_ACTIVE_SIZE(0);
}

void CommControllerTester::testGetStatusDoesNotAdvancePassTimerOrForceSyncProbe() {
    this->setLinkAvailability(true, true);
    this->setSubsystemAvailability(true, true);
    this->sendCmd_COMM_START_PASS(TEST_INSTANCE_ID, 0, 9U);
    this->clearHistory();

    const U32 sbandProbeCountBefore =
        this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
    const U32 uhfProbeCountBefore =
        this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID);

    this->sendCmd_COMM_GET_STATUS(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_GET_STATUS, 1, Fw::CmdResponse::OK);
    EXPECT_EQ(this->component.getStateForRuntime().passRemainingSec, 9U);
    EXPECT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID),
              sbandProbeCountBefore);
    EXPECT_EQ(this->m_commSubsystemHealthProbe.getProbeCount(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID),
              uhfProbeCountBefore);
    ASSERT_TLM_COMM_PASS_REMAINING_SIZE(0);
}

void CommControllerTester::testGetStatusDoesNotAdvanceCommFdirDebounce() {
    OBC::CommSubsystemFdirConfig subsystemFdirConfig = {};
    subsystemFdirConfig.useSubsystemResponsiveness = true;
    subsystemFdirConfig.primaryProbePeriodTicks = 1U;
    subsystemFdirConfig.primaryProbeFailureThreshold = 1U;
    subsystemFdirConfig.unavailableFailureThreshold = 3U;
    this->component.configureRuntime(&this->m_groundLinkHealthProvider,
                                     &this->m_recoverySink,
                                     &this->m_commandIngressAuthority,
                                     &this->m_beaconSuppressControl,
                                     &this->m_commEgressMux,
                                     &this->m_commSubsystemHealthProbe,
                                     OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                     OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                     subsystemFdirConfig,
                                     OBC::CommBand::SBAND,
                                     authorityConfigForProfileWithAuth("sband-primary"),
                                     authorityConfigForProfileWithAuth("uhf-backup"));

    this->setSubsystemAvailability(true, true);
    this->refreshAvailability();
    ASSERT_TRUE(this->component.getStateForRuntime().sbandAvailable);

    this->clearHistory();
    this->setSubsystemAvailability(false, true);
    this->tickTimes(1U);

    ASSERT_FALSE(this->component.getStateForRuntime().sbandAvailable);
    ASSERT_FALSE(this->component.getStateForRuntime().fdirFaultLatched);
    ASSERT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);

    this->sendCmd_COMM_GET_STATUS(TEST_INSTANCE_ID, 0);
    this->sendCmd_COMM_GET_STATUS(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_COMM_GET_STATUS, 0, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_COMM_GET_STATUS, 1, Fw::CmdResponse::OK);
    EXPECT_FALSE(this->component.getStateForRuntime().fdirFaultLatched);
    EXPECT_EQ(this->component.getStateForRuntime().fdirFaultKind, OBC::CommFdirFaultKind::NONE);
    EXPECT_EQ(this->m_recoverySink.commUnavailableFaultCount, 0U);
}

}  // namespace OBC
