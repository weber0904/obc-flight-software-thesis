#include "CspBridgeTester.hpp"

#include "Fw/Cmd/CmdString.hpp"

namespace OBC {

OBC::CSP::RuntimeStatus FakeCspRuntime::init(const OBC::CSP::RuntimeConfig& config) {
    if (this->initStatus == OBC::CSP::RuntimeStatus::OK) {
        this->runtimeMetrics.initialized = true;
        this->runtimeMetrics.localNodeId = config.nodeId;
        this->runtimeMetrics.txPackets = 0U;
        this->runtimeMetrics.rxPackets = 0U;
        this->runtimeMetrics.errorCount = 0U;
        this->runtimeMetrics.freeBuffers = 15U;
    } else {
        this->runtimeMetrics.errorCount++;
    }
    return this->initStatus;
}

OBC::CSP::RuntimeStatus FakeCspRuntime::ping(std::uint16_t, std::uint32_t, bool& success) {
    if (!this->runtimeMetrics.initialized) {
        this->runtimeMetrics.errorCount++;
        success = false;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    if (this->pingStatus != OBC::CSP::RuntimeStatus::OK) {
        this->runtimeMetrics.errorCount++;
        success = false;
        return this->pingStatus;
    }

    this->runtimeMetrics.txPackets++;
    success = this->pingSuccess;
    if (success) {
        this->runtimeMetrics.rxPackets++;
    } else {
        this->runtimeMetrics.errorCount++;
    }
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeCspRuntime::sendRaw(std::uint16_t, std::uint8_t, const std::string&) {
    if (!this->runtimeMetrics.initialized) {
        this->runtimeMetrics.errorCount++;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    if (this->sendStatus != OBC::CSP::RuntimeStatus::OK) {
        this->runtimeMetrics.errorCount++;
        return this->sendStatus;
    }

    this->runtimeMetrics.txPackets++;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeCspRuntime::requestReply(std::uint16_t,
                                                     std::uint8_t,
                                                     const void*,
                                                     std::size_t,
                                                     void*,
                                                     std::size_t,
                                                     std::size_t& replySize,
                                                     std::uint32_t) {
    replySize = 0U;
    if (!this->runtimeMetrics.initialized) {
        this->runtimeMetrics.errorCount++;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    this->runtimeMetrics.txPackets++;
    this->runtimeMetrics.rxPackets++;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeMetrics FakeCspRuntime::metrics() const {
    return this->runtimeMetrics;
}

void FakeCspRuntime::shutdown() {}

CspBridgeTester::CspBridgeTester()
    : CspBridgeGTestBase("CspBridgeTester", MAX_HISTORY_SIZE), component("CspBridge", this->runtime) {
    this->initComponents();
    this->connectPorts();
}

CspBridgeTester::~CspBridgeTester() = default;

void CspBridgeTester::testInitAndSuccessfulPing() {
    this->clearHistory();
    this->sendCmd_CSP_INIT(TEST_INSTANCE_ID, 0, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_CSP_INIT, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CSP_INIT_COMPLETE_SIZE(1);
    ASSERT_TLM_CSP_TX_PACKETS_SIZE(1);
    ASSERT_TLM_CSP_TX_PACKETS(0, 0);
    ASSERT_TLM_CSP_RX_PACKETS_SIZE(1);
    ASSERT_TLM_CSP_RX_PACKETS(0, 0);
    ASSERT_TLM_CSP_ERROR_COUNT_SIZE(1);
    ASSERT_TLM_CSP_ERROR_COUNT(0, 0);

    this->clearHistory();
    this->sendCmd_CSP_PING(TEST_INSTANCE_ID, 1, 2, 100);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_CSP_PING, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CSP_PING_RESULT_SIZE(0);
    ASSERT_TLM_CSP_TX_PACKETS_SIZE(1);
    ASSERT_TLM_CSP_TX_PACKETS(0, 1);
    ASSERT_TLM_CSP_RX_PACKETS_SIZE(1);
    ASSERT_TLM_CSP_RX_PACKETS(0, 1);
    ASSERT_TLM_CSP_ERROR_COUNT_SIZE(0);
}

void CspBridgeTester::testSendBeforeInitFails() {
    this->clearHistory();
    this->sendCmd_CSP_SEND_RAW(TEST_INSTANCE_ID, 0, 2, 10, Fw::CmdStringArg("abc"));

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_CSP_SEND_RAW, 0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_CSP_ERROR_SIZE(1);
    ASSERT_TLM_CSP_ERROR_COUNT_SIZE(1);
    ASSERT_TLM_CSP_ERROR_COUNT(0, 1);
}

void CspBridgeTester::testPingFailureReturnsFalseResult() {
    this->runtime.pingSuccess = false;

    this->clearHistory();
    this->sendCmd_CSP_INIT(TEST_INSTANCE_ID, 0, 1);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_CSP_INIT, 0, Fw::CmdResponse::OK);

    this->clearHistory();
    this->sendCmd_CSP_PING(TEST_INSTANCE_ID, 1, 2, 100);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_CSP_PING, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CSP_PING_RESULT_SIZE(1);
    ASSERT_EVENTS_CSP_PING_RESULT(0, 2, false, 100);
    ASSERT_EVENTS_CSP_ERROR_SIZE(1);
    ASSERT_TLM_CSP_TX_PACKETS_SIZE(1);
    ASSERT_TLM_CSP_TX_PACKETS(0, 1);
    ASSERT_TLM_CSP_RX_PACKETS_SIZE(0);
    ASSERT_TLM_CSP_ERROR_COUNT_SIZE(1);
    ASSERT_TLM_CSP_ERROR_COUNT(0, 1);
}

}  // namespace OBC
