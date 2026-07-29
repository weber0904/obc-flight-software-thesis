#include "EpsBridgeTester.hpp"

#include <cstring>
#include <deque>
#include <limits>
#include <unordered_map>

namespace OBC {

namespace {

class FakeEpsTransport final : public OBC::EPS::IEpsTransport {
  public:
    struct Reply {
        OBC::EPS::TransportStatus status;
        OBC::EPS::StatusData data;
    };

    void pushStatusReply(OBC::EPS::TransportStatus status, const OBC::EPS::StatusData& data) {
        this->m_statusReplies.push_back({status, data});
    }

    void pushPduReply(OBC::EPS::TransportStatus status, const OBC::EPS::StatusData& data) {
        this->m_pduReplies.push_back({status, data});
    }

    OBC::EPS::TransportStatus getStatus(OBC::EPS::StatusData& outStatus) override {
        return this->popReply_(this->m_statusReplies, outStatus);
    }

    OBC::EPS::TransportStatus setPdu(std::uint8_t channel, bool enabled, OBC::EPS::StatusData& outStatus) override {
        static_cast<void>(channel);
        static_cast<void>(enabled);
        return this->popReply_(this->m_pduReplies, outStatus);
    }

    OBC::EPS::TransportStatus setHeater(bool enabled, OBC::EPS::StatusData& outStatus) override {
        static_cast<void>(enabled);
        return this->popReply_(this->m_statusReplies, outStatus);
    }

    OBC::EPS::TransportStatus reset(OBC::EPS::StatusData& outStatus) override {
        return this->popReply_(this->m_statusReplies, outStatus);
    }

  private:
    OBC::EPS::TransportStatus popReply_(std::deque<Reply>& queue, OBC::EPS::StatusData& outStatus) {
        if (queue.empty()) {
            return OBC::EPS::TransportStatus::TRANSPORT_ERROR;
        }

        const Reply reply = queue.front();
        queue.pop_front();
        outStatus = reply.data;
        return reply.status;
    }

  private:
    std::deque<Reply> m_statusReplies;
    std::deque<Reply> m_pduReplies;
};

class FakeAsyncEpsRuntime final : public OBC::CSP::ICspRuntime, public OBC::IAsyncCspRuntimeOwner {
  public:
    struct QueuedStatusCompletion {
        OBC::EPS::TransportStatus status;
        OBC::EPS::StatusData data;
    };

    void pushStatusCompletion(OBC::EPS::TransportStatus status, const OBC::EPS::StatusData& data) {
        this->m_statusCompletions.push_back({status, data});
    }

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig& config) override {
        static_cast<void>(config);
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override {
        static_cast<void>(targetNode);
        static_cast<void>(timeoutMs);
        success = false;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& data) override {
        static_cast<void>(targetNode);
        static_cast<void>(targetPort);
        static_cast<void>(data);
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    OBC::CSP::RuntimeStatus requestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t timeoutMs) override {
        static_cast<void>(targetNode);
        static_cast<void>(targetPort);
        static_cast<void>(requestData);
        static_cast<void>(requestSize);
        static_cast<void>(replyData);
        static_cast<void>(replyCapacity);
        static_cast<void>(replySize);
        static_cast<void>(timeoutMs);
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    OBC::CSP::RuntimeMetrics metrics() const override {
        OBC::CSP::RuntimeMetrics metrics = {};
        metrics.initialized = true;
        return metrics;
    }

    void shutdown() override {}

    bool submitAsyncPing(std::uint16_t targetNode, std::uint32_t timeoutMs, std::uint64_t& handle) override {
        static_cast<void>(targetNode);
        static_cast<void>(timeoutMs);
        static_cast<void>(handle);
        return false;
    }

    bool takeAsyncPingCompletion(std::uint64_t handle, OBC::AsyncCspPingCompletion& completion) override {
        static_cast<void>(handle);
        static_cast<void>(completion);
        return false;
    }

    bool submitAsyncRequestReply(std::uint16_t targetNode,
                                 std::uint8_t targetPort,
                                 const void* requestData,
                                 std::size_t requestSize,
                                 std::size_t replyCapacity,
                                 std::uint32_t timeoutMs,
                                 std::uint64_t& handle) override {
        static_cast<void>(targetNode);
        static_cast<void>(targetPort);
        static_cast<void>(requestSize);
        static_cast<void>(replyCapacity);
        static_cast<void>(timeoutMs);
        this->submitCount += 1U;
        handle = this->m_nextHandle++;
        if (this->m_statusCompletions.empty()) {
            return true;
        }

        const auto queued = this->m_statusCompletions.front();
        this->m_statusCompletions.pop_front();
        OBC::AsyncCspRequestReplyCompletion completion = {};
        if (queued.status == OBC::EPS::TransportStatus::OK) {
            const auto* request = static_cast<const OBC::EPS::CSP::Request*>(requestData);
            OBC::EPS::CSP::Reply reply = OBC::EPS::CSP::makeBlankReply(OBC::EPS::CSP::ServicePort::STATUS,
                                                                       request->header.seq);
            reply.header.result = static_cast<std::uint8_t>(OBC::EPS::ResultCode::OK);
            reply.status = queued.data;
            completion.status = OBC::CSP::RuntimeStatus::OK;
            completion.reply.resize(sizeof(reply));
            std::memcpy(completion.reply.data(), &reply, sizeof(reply));
            completion.replySize = sizeof(reply);
        } else if (queued.status == OBC::EPS::TransportStatus::TIMEOUT) {
            completion.status = OBC::CSP::RuntimeStatus::TIMEOUT;
        } else {
            completion.status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
        }
        this->m_requestReplyCompletions[handle] = completion;
        return true;
    }

    bool takeAsyncRequestReplyCompletion(std::uint64_t handle, OBC::AsyncCspRequestReplyCompletion& completion) override {
        const auto it = this->m_requestReplyCompletions.find(handle);
        if (it == this->m_requestReplyCompletions.end()) {
            return false;
        }
        completion = it->second;
        this->m_requestReplyCompletions.erase(it);
        return true;
    }

    void recordCoalescedForRuntime() override {
        this->coalescedCount += 1U;
    }

  public:
    U32 submitCount = 0U;
    U32 coalescedCount = 0U;

  private:
    std::uint64_t m_nextHandle = 1U;
    std::deque<QueuedStatusCompletion> m_statusCompletions;
    std::unordered_map<std::uint64_t, OBC::AsyncCspRequestReplyCompletion> m_requestReplyCompletions;
};

OBC::EPS::StatusData makeStatus(float soc,
                                std::uint8_t pduStatus,
                                float tempBat = 28.0F,
                                bool heaterEnabled = false,
                                std::uint8_t overcurrentFlags = 0U,
                                float powerOut = 4.0F) {
    OBC::EPS::StatusData status = {};
    status.vbat = 8.1F;
    status.ibat = -0.3F;
    status.soc = soc;
    status.vsolar = 5.3F;
    status.isolar = 0.6F;
    status.temp_bat = tempBat;
    status.power_out = powerOut;
    status.pdu_status = pduStatus;
    status.sunlight = 1U;
    status.heater_enabled = heaterEnabled ? 1U : 0U;
    status.overcurrent_flags = overcurrentFlags;
    return status;
}

}  // namespace

EpsBridgeTester::EpsBridgeTester()
    : EpsBridgeGTestBase("EpsBridgeTester", MAX_HISTORY_SIZE), component("EpsBridge") {
    this->initComponents();
    this->connectPorts();
}

EpsBridgeTester::~EpsBridgeTester() = default;

void EpsBridgeTester::from_epsStatusRefreshTlmOut_handler(FwIndexType portNum,
                                                          FwChanIdType id,
                                                          Fw::Time& timeTag,
                                                          Fw::TlmBuffer& val) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_epsStatusRefreshTlmOut(id, timeTag, val);
    this->dispatchTlm(id, timeTag, val);
}

void EpsBridgeTester::testGetStatusPublishesExplicitRefreshTelemetry() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(76.0F, 0x03U, 28.0F, true, 0x02U, 7.5F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_EPS_GET_STATUS, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_EPS_STATUS_RECEIVED_SIZE(1);
    ASSERT_TLM_EPS_VBAT_SIZE(1);
    ASSERT_TLM_EPS_IBAT_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_SOC(0, 76.0F);
    ASSERT_TLM_EPS_TEMP_BAT_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS(0, 0x03U);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED(0, true);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(1);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS(0, 0x02U);
    ASSERT_TLM_EPS_VSOLAR_SIZE(1);
    ASSERT_TLM_EPS_ISOLAR_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT(0, 7.5F);
    ASSERT_EVENTS_EPS_COMM_ERROR_SIZE(0);
}

void EpsBridgeTester::testPduCommandPublishesChangeEventAndExplicitRefresh() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(75.0F, 0x01U));
    transport.pushPduReply(OBC::EPS::TransportStatus::OK, makeStatus(75.0F, 0x05U, 29.0F, true, 0x01U, 12.5F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_EPS_SET_PDU(TEST_INSTANCE_ID, 1, 2U, true);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_EPS_SET_PDU, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_EPS_STATUS_RECEIVED_SIZE(1);
    ASSERT_EVENTS_EPS_PDU_CHANGE_SIZE(1);
    ASSERT_EVENTS_EPS_PDU_CHANGE(0, 0x05U);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS(0, 0x05U);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED(0, true);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(1);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS(0, 0x01U);
    ASSERT_TLM_EPS_VSOLAR_SIZE(1);
    ASSERT_TLM_EPS_ISOLAR_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT(0, 12.5F);
}

void EpsBridgeTester::testHeaterCommandPublishesExplicitRefreshTelemetry() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(74.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(74.0F, 0x03U, 31.0F, true, 0x00U, 6.5F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_EPS_SET_HEATER(TEST_INSTANCE_ID, 1, true);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_EPS_SET_HEATER, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_EPS_STATUS_RECEIVED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED(0, true);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(1);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS(0, 0x00U);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT(0, 6.5F);
}

void EpsBridgeTester::testResetPublishesExplicitRefreshTelemetry() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(74.0F, 0x07U, 31.0F, true, 0x01U, 12.5F));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(80.0F, 0x03U, 27.0F, false, 0x00U, 4.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_EPS_RESET(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_EPS_RESET, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_EPS_STATUS_RECEIVED_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS(0, 0x03U);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED(0, false);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(1);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS(0, 0x00U);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(1);
    ASSERT_TLM_EPS_POWER_OUT(0, 4.0F);
}

void EpsBridgeTester::testLowBatteryAndCriticalEvents() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(15.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(5.0F, 0x03U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_EVENTS_EPS_LOW_BATTERY_SIZE(1);
    ASSERT_EVENTS_EPS_LOW_BATTERY(0, 15.0F);
    ASSERT_EVENTS_EPS_CRITICAL_BATTERY_SIZE(0);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());

    ASSERT_EVENTS_EPS_LOW_BATTERY_SIZE(0);
    ASSERT_EVENTS_EPS_CRITICAL_BATTERY_SIZE(1);
    ASSERT_EVENTS_EPS_CRITICAL_BATTERY(0, 5.0F);
}

void EpsBridgeTester::testScheduledPollPublishesContinuousOnly() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());

    ASSERT_EVENTS_EPS_STATUS_RECEIVED_SIZE(0);
    ASSERT_TLM_EPS_VBAT_SIZE(1);
    ASSERT_TLM_EPS_IBAT_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_TEMP_BAT_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(0);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(0);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(0);
    ASSERT_TLM_EPS_VSOLAR_SIZE(0);
    ASSERT_TLM_EPS_ISOLAR_SIZE(0);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(0);
}

void EpsBridgeTester::testScheduledPollPublishesChangeDrivenTelemetryOnStateChange() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x07U, 28.0F, true, 0x01U, 12.5F));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    ASSERT_TRUE(this->component.pollStatusForTest());

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());

    ASSERT_EVENTS_EPS_PDU_CHANGE_SIZE(1);
    ASSERT_EVENTS_EPS_PDU_CHANGE(0, 0x07U);
    ASSERT_TLM_EPS_VBAT_SIZE(1);
    ASSERT_TLM_EPS_IBAT_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_TEMP_BAT_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(1);
    ASSERT_TLM_EPS_PDU_STATUS(0, 0x07U);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(1);
    ASSERT_TLM_EPS_HEATER_ENABLED(0, true);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(1);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS(0, 0x01U);
    ASSERT_TLM_EPS_POWER_OUT_SIZE(0);
}

void EpsBridgeTester::testScheduledPollThrottlesContinuousTelemetry() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(71.0F, 0x03U));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(3U);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());
    ASSERT_TLM_EPS_VBAT_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_SOC(0, 70.0F);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());
    ASSERT_TLM_EPS_VBAT_SIZE(0);
    ASSERT_TLM_EPS_IBAT_SIZE(0);
    ASSERT_TLM_EPS_SOC_SIZE(0);
    ASSERT_TLM_EPS_TEMP_BAT_SIZE(0);
}

void EpsBridgeTester::testSchedInUsesAsyncOwnerFlow() {
    FakeAsyncEpsRuntime runtime;
    runtime.pushStatusCompletion(OBC::EPS::TransportStatus::OK, makeStatus(68.0F, 0x07U));
    this->component.configureCspRuntimeForRuntime(runtime);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();

    ASSERT_EQ(runtime.submitCount, 1U);
    ASSERT_TLM_EPS_VBAT_SIZE(0);
    ASSERT_TLM_EPS_SOC_SIZE(0);
    ASSERT_EVENTS_EPS_COMM_ERROR_SIZE(0);

    this->clearHistory();
    this->component.tickScheduledPollForTest();

    ASSERT_EQ(runtime.submitCount, 2U);
    ASSERT_TLM_EPS_VBAT_SIZE(1);
    ASSERT_TLM_EPS_IBAT_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_SOC(0, 68.0F);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(0);
    ASSERT_TLM_EPS_HEATER_ENABLED_SIZE(0);
    ASSERT_TLM_EPS_OVERCURRENT_FLAGS_SIZE(0);
    ASSERT_EVENTS_EPS_COMM_ERROR_SIZE(0);
}

void EpsBridgeTester::testSchedInHonorsConfiguredPollPeriod() {
    FakeAsyncEpsRuntime runtime;
    runtime.pushStatusCompletion(OBC::EPS::TransportStatus::OK, makeStatus(69.0F, 0x09U));
    this->component.configureCspRuntimeForRuntime(runtime);
    this->component.configureScheduledPollPeriodForTest(3U);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);
    ASSERT_TLM_EPS_SOC_SIZE(1);
    ASSERT_TLM_EPS_SOC(0, 69.0F);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 2U);
}

void EpsBridgeTester::testTimeoutInvalidatesCachedStatusAndPreservesLastTelemetry() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, makeStatus(0.0F, 0x00U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_EPS_GET_STATUS(TEST_INSTANCE_ID, 0);
    this->component.drainExplicitRefreshForTest();

    OBC::EPS::StatusData cachedStatus = {};
    ASSERT_TRUE(this->component.getCachedStatusForRuntime(cachedStatus));
    ASSERT_EQ(cachedStatus.soc, 70.0F);

    this->clearHistory();
    ASSERT_FALSE(this->component.pollStatusForTest());

    ASSERT_FALSE(this->component.getCachedStatusForRuntime(cachedStatus));
    ASSERT_EVENTS_EPS_COMM_ERROR_SIZE(1);
    ASSERT_TLM_EPS_SOC_SIZE(0);
    ASSERT_TLM_EPS_PDU_STATUS_SIZE(0);
}

void EpsBridgeTester::testPollHealthTracksConsecutiveFailuresAndRecovery() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, makeStatus(0.0F, 0x00U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, makeStatus(0.0F, 0x00U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(71.0F, 0x03U));
    this->component.setTransportForTest(&transport);

    OBC::EPS::PollHealthState health = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 0U);

    ASSERT_TRUE(this->component.pollStatusForTest());
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_TRUE(health.cacheValid);
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 0U);

    ASSERT_FALSE(this->component.pollStatusForTest());
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_FALSE(health.cacheValid);
    ASSERT_FALSE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 1U);
    ASSERT_EQ(health.cumulativePollErrors, 1U);

    ASSERT_FALSE(this->component.pollStatusForTest());
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_FALSE(health.cacheValid);
    ASSERT_FALSE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 2U);
    ASSERT_EQ(health.cumulativePollErrors, 2U);

    ASSERT_TRUE(this->component.pollStatusForTest());
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_TRUE(health.cacheValid);
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 2U);
}

void EpsBridgeTester::testRuntimeFetchDoesNotMutatePollHealth() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(70.0F, 0x03U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, makeStatus(0.0F, 0x00U));
    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(71.0F, 0x03U));
    this->component.setTransportForTest(&transport);

    OBC::EPS::PollHealthState health = {};
    ASSERT_TRUE(this->component.pollStatusForTest());
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_TRUE(health.cacheValid);
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 0U);

    OBC::EPS::StatusData status = {};
    ASSERT_FALSE(this->component.getStatusForRuntime(status));
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_FALSE(health.cacheValid);
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 0U);

    ASSERT_TRUE(this->component.getStatusForRuntime(status));
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_TRUE(health.cacheValid);
    ASSERT_TRUE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, 0U);
    ASSERT_EQ(health.cumulativePollErrors, 0U);
}

void EpsBridgeTester::testPollHealthCountersSaturateAtMax() {
    FakeEpsTransport transport;
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, makeStatus(0.0F, 0x00U));
    this->component.setTransportForTest(&transport);

    OBC::EPS::PollHealthState state = {};
    state.cacheValid = true;
    state.lastPollSucceeded = true;
    state.consecutivePollFailures = std::numeric_limits<U32>::max();
    state.cumulativePollErrors = std::numeric_limits<U32>::max();
    this->component.setPollHealthForTest(state);

    ASSERT_FALSE(this->component.pollStatusForTest());

    OBC::EPS::PollHealthState health = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(health));
    ASSERT_FALSE(health.cacheValid);
    ASSERT_FALSE(health.lastPollSucceeded);
    ASSERT_EQ(health.consecutivePollFailures, std::numeric_limits<U32>::max());
    ASSERT_EQ(health.cumulativePollErrors, std::numeric_limits<U32>::max());
}

}  // namespace OBC
