#include "AdcsBridgeTester.hpp"

#include <cstring>
#include <deque>
#include <unordered_map>

namespace OBC {

namespace {

class FakeAdcsTransport final : public OBC::ADCS::IAdcsTransport {
  public:
    struct Reply {
        OBC::ADCS::TransportStatus status;
        OBC::ADCS::StateData data;
    };

    void pushStateReply(OBC::ADCS::TransportStatus status, const OBC::ADCS::StateData& data) {
        this->m_stateReplies.push_back({status, data});
    }

    void pushModeReply(OBC::ADCS::TransportStatus status, const OBC::ADCS::StateData& data) {
        this->m_modeReplies.push_back({status, data});
    }

    void pushTargetReply(OBC::ADCS::TransportStatus status, const OBC::ADCS::StateData& data) {
        this->m_targetReplies.push_back({status, data});
    }

    void pushResetReply(OBC::ADCS::TransportStatus status, const OBC::ADCS::StateData& data) {
        this->m_resetReplies.push_back({status, data});
    }

    OBC::ADCS::TransportStatus getState(OBC::ADCS::StateData& outState) override {
        return this->popReply_(this->m_stateReplies, outState);
    }

    OBC::ADCS::TransportStatus setMode(std::uint8_t mode, OBC::ADCS::StateData& outState) override {
        static_cast<void>(mode);
        return this->popReply_(this->m_modeReplies, outState);
    }

    OBC::ADCS::TransportStatus setTarget(double q0,
                                         double q1,
                                         double q2,
                                         double q3,
                                         OBC::ADCS::StateData& outState) override {
        static_cast<void>(q0);
        static_cast<void>(q1);
        static_cast<void>(q2);
        static_cast<void>(q3);
        return this->popReply_(this->m_targetReplies, outState);
    }

    OBC::ADCS::TransportStatus calibrate(std::uint8_t sensorId, OBC::ADCS::StateData& outState) override {
        static_cast<void>(sensorId);
        return this->popReply_(this->m_stateReplies, outState);
    }

    OBC::ADCS::TransportStatus reset(OBC::ADCS::StateData& outState) override {
        return this->popReply_(this->m_resetReplies, outState);
    }

  private:
    OBC::ADCS::TransportStatus popReply_(std::deque<Reply>& queue, OBC::ADCS::StateData& outState) {
        if (queue.empty()) {
            return OBC::ADCS::TransportStatus::TRANSPORT_ERROR;
        }

        const Reply reply = queue.front();
        queue.pop_front();
        outState = reply.data;
        return reply.status;
    }

  private:
    std::deque<Reply> m_stateReplies;
    std::deque<Reply> m_modeReplies;
    std::deque<Reply> m_targetReplies;
    std::deque<Reply> m_resetReplies;
};

class FakeAsyncAdcsRuntime final : public OBC::CSP::ICspRuntime, public OBC::IAsyncCspRuntimeOwner {
  public:
    struct QueuedStateCompletion {
        OBC::ADCS::TransportStatus status;
        OBC::ADCS::StateData data;
    };

    void pushStateCompletion(OBC::ADCS::TransportStatus status, const OBC::ADCS::StateData& data) {
        this->m_stateCompletions.push_back({status, data});
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
        if (this->m_stateCompletions.empty()) {
            return true;
        }

        const auto queued = this->m_stateCompletions.front();
        this->m_stateCompletions.pop_front();
        OBC::AsyncCspRequestReplyCompletion completion = {};
        if (queued.status == OBC::ADCS::TransportStatus::OK) {
            const auto* request = static_cast<const OBC::ADCS::CSP::Request*>(requestData);
            OBC::ADCS::CSP::Reply reply = OBC::ADCS::CSP::makeBlankReply(OBC::ADCS::CSP::ServicePort::STATE,
                                                                         request->header.seq);
            reply.header.result = static_cast<std::uint8_t>(OBC::ADCS::ResultCode::OK);
            reply.state = queued.data;
            completion.status = OBC::CSP::RuntimeStatus::OK;
            completion.reply.resize(sizeof(reply));
            std::memcpy(completion.reply.data(), &reply, sizeof(reply));
            completion.replySize = sizeof(reply);
        } else if (queued.status == OBC::ADCS::TransportStatus::TIMEOUT) {
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
    std::deque<QueuedStateCompletion> m_stateCompletions;
    std::unordered_map<std::uint64_t, OBC::AsyncCspRequestReplyCompletion> m_requestReplyCompletions;
};

OBC::ADCS::StateData makeState(std::uint8_t mode, float omegaNorm, float pointingErr, bool sensorValid = true) {
    OBC::ADCS::StateData state = {};
    state.q0 = 1.0;
    state.q1 = 0.0;
    state.q2 = 0.0;
    state.q3 = 0.0;
    state.omega_x = omegaNorm;
    state.omega_y = 0.0F;
    state.omega_z = 0.0F;
    state.mag_x = 0.2F;
    state.mag_y = -0.1F;
    state.mag_z = 0.4F;
    state.pointing_error_deg = pointingErr;
    state.mode = mode;
    state.sensor_valid = sensorValid ? 1U : 0U;
    return state;
}

}  // namespace

AdcsBridgeTester::AdcsBridgeTester()
    : AdcsBridgeGTestBase("AdcsBridgeTester", MAX_HISTORY_SIZE), component("AdcsBridge") {
    this->initComponents();
    this->connectPorts();
}

AdcsBridgeTester::~AdcsBridgeTester() = default;

void AdcsBridgeTester::from_adcsStatusRefreshTlmOut_handler(FwIndexType portNum,
                                                            FwChanIdType id,
                                                            Fw::Time& timeTag,
                                                            Fw::TlmBuffer& val) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_adcsStatusRefreshTlmOut(id, timeTag, val);
    this->dispatchTlm(id, timeTag, val);
}

void AdcsBridgeTester::testGetAttitudePublishesExplicitRefreshTelemetry() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_ADCS_GET_ATTITUDE, 0, Fw::CmdResponse::OK);
    ASSERT_TLM_ADCS_MODE_SIZE(1);
    ASSERT_TLM_ADCS_MODE(0, OBC::AdcsMode::IDLE);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_Q1_SIZE(1);
    ASSERT_TLM_ADCS_Q2_SIZE(1);
    ASSERT_TLM_ADCS_Q3_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X(0, 0.12F);
    ASSERT_TLM_ADCS_OMEGA_Y_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_Z_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(1);
    ASSERT_TLM_ADCS_MAG_Y_SIZE(1);
    ASSERT_TLM_ADCS_MAG_Z_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR(0, 18.0F);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(0);
}

void AdcsBridgeTester::testGetAttitudeRepublishesExplicitRefreshWhenValuesUnchanged() {
    FakeAdcsTransport transport;
    const auto state = makeState(0U, 0.12F, 18.0F);
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, state);
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, state);
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);
    ASSERT_TLM_ADCS_MODE_SIZE(1);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_ADCS_GET_ATTITUDE, 1, Fw::CmdResponse::OK);
    ASSERT_TLM_ADCS_MODE_SIZE(1);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(1);
}

void AdcsBridgeTester::testSetModeCanTriggerDetumbleCompletion() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushModeReply(OBC::ADCS::TransportStatus::OK, makeState(1U, 0.04F, 18.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_ADCS_SET_MODE(TEST_INSTANCE_ID, 1, OBC::AdcsMode::DETUMBLE);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_ADCS_SET_MODE, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_ADCS_MODE_CHANGE_SIZE(1);
    ASSERT_EVENTS_ADCS_MODE_CHANGE(0, OBC::AdcsMode::DETUMBLE);
    ASSERT_EVENTS_ADCS_DETUMBLE_COMPLETE_SIZE(1);
    ASSERT_TLM_ADCS_MODE_SIZE(2);
    ASSERT_TLM_ADCS_MODE(0, OBC::AdcsMode::DETUMBLE);
    ASSERT_TLM_ADCS_MODE(1, OBC::AdcsMode::DETUMBLE);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(1);
}

void AdcsBridgeTester::testPointingAcquiredAfterTargetUpdate() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 7.0F));
    transport.pushTargetReply(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.02F, 4.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_ADCS_SET_TARGET(TEST_INSTANCE_ID, 1, 1.0, 0.0, 0.0, 0.0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_ADCS_SET_TARGET, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_ADCS_POINTING_ACQUIRED_SIZE(1);
    ASSERT_TLM_ADCS_MODE_SIZE(1);
    ASSERT_TLM_ADCS_MODE(0, OBC::AdcsMode::POINTING);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR(0, 4.0F);
}

void AdcsBridgeTester::testScheduledPollPublishesContinuousOnly() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 7.0F));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStateForTest());

    ASSERT_TLM_ADCS_MODE_SIZE(0);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_Q1_SIZE(1);
    ASSERT_TLM_ADCS_Q2_SIZE(1);
    ASSERT_TLM_ADCS_Q3_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_Y_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_Z_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(0);
    ASSERT_TLM_ADCS_MAG_Y_SIZE(0);
    ASSERT_TLM_ADCS_MAG_Z_SIZE(0);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(0);
}

void AdcsBridgeTester::testScheduledPollPublishesChangeDrivenTelemetryOnModeChange() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 7.0F));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    ASSERT_TRUE(this->component.pollStateForTest());

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStateForTest());

    ASSERT_EVENTS_ADCS_MODE_CHANGE_SIZE(1);
    ASSERT_EVENTS_ADCS_MODE_CHANGE(0, OBC::AdcsMode::POINTING);
    ASSERT_TLM_ADCS_MODE_SIZE(1);
    ASSERT_TLM_ADCS_MODE(0, OBC::AdcsMode::POINTING);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(0);
    ASSERT_TLM_ADCS_MAG_X_SIZE(0);
}

void AdcsBridgeTester::testScheduledPollThrottlesContinuousTelemetry() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.11F, 17.0F));
    this->component.setTransportForTest(&transport);
    this->component.configureOperatorTelemetryPeriodForTest(3U);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStateForTest());
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStateForTest());
    ASSERT_TLM_ADCS_Q0_SIZE(0);
    ASSERT_TLM_ADCS_Q1_SIZE(0);
    ASSERT_TLM_ADCS_Q2_SIZE(0);
    ASSERT_TLM_ADCS_Q3_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_Y_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_Z_SIZE(0);
    ASSERT_TLM_ADCS_MODE_SIZE(0);
}

void AdcsBridgeTester::testSchedInUsesAsyncOwnerFlow() {
    FakeAsyncAdcsRuntime runtime;
    runtime.pushStateCompletion(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 6.5F));
    this->component.configureCspRuntimeForRuntime(runtime);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();

    ASSERT_EQ(runtime.submitCount, 1U);
    ASSERT_TLM_ADCS_Q0_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(0);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(0);

    this->clearHistory();
    this->component.tickScheduledPollForTest();

    ASSERT_EQ(runtime.submitCount, 2U);
    ASSERT_TLM_ADCS_MODE_SIZE(0);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_Q1_SIZE(1);
    ASSERT_TLM_ADCS_Q2_SIZE(1);
    ASSERT_TLM_ADCS_Q3_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X(0, 0.03F);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(0);
    ASSERT_TLM_ADCS_MAG_X_SIZE(0);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(0);
}

void AdcsBridgeTester::testSchedInHonorsConfiguredPollPeriod() {
    FakeAsyncAdcsRuntime runtime;
    runtime.pushStateCompletion(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 5.5F));
    this->component.configureCspRuntimeForRuntime(runtime);
    this->component.configureScheduledPollPeriodForTest(3U);
    this->component.configureOperatorTelemetryPeriodForTest(1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 1U);

    this->clearHistory();
    this->component.tickScheduledPollForTest();
    ASSERT_EQ(runtime.submitCount, 2U);
}

void AdcsBridgeTester::testInvalidSensorReplyPreservesLastTelemetry() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.01F, 1.0F, false));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    ASSERT_FALSE(this->component.pollStateForTest());

    ASSERT_EVENTS_ADCS_SENSOR_FAULT_SIZE(1);
    ASSERT_TLM_ADCS_Q0_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(0);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(0);
}

void AdcsBridgeTester::testTimeoutPreservesLastTelemetry() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushStateReply(OBC::ADCS::TransportStatus::TIMEOUT, makeState(0U, 0.0F, 0.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    ASSERT_FALSE(this->component.pollStateForTest());

    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(1);
    ASSERT_TLM_ADCS_Q0_SIZE(0);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(0);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(0);
}

void AdcsBridgeTester::testCommandFailureDoesNotMutateScheduledPollHealth() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushStateReply(OBC::ADCS::TransportStatus::TIMEOUT, makeState(0U, 0.0F, 0.0F));
    this->component.setTransportForTest(&transport);

    ASSERT_TRUE(this->component.pollStateForTest());
    OBC::ADCS::PollHealthState before = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(before));
    ASSERT_TRUE(before.lastScheduledValidRefresh);
    ASSERT_EQ(before.consecutiveTransportFailures, 0U);

    this->clearHistory();
    this->sendCmd_ADCS_GET_ATTITUDE(TEST_INSTANCE_ID, 0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_ADCS_GET_ATTITUDE, 0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(1);

    OBC::ADCS::PollHealthState after = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(after));
    ASSERT_EQ(after.lastScheduledTransportOk, before.lastScheduledTransportOk);
    ASSERT_EQ(after.lastScheduledValidRefresh, before.lastScheduledValidRefresh);
    ASSERT_EQ(after.consecutiveTransportFailures, before.consecutiveTransportFailures);
    ASSERT_EQ(after.consecutiveNoValidRefresh, before.consecutiveNoValidRefresh);
}

void AdcsBridgeTester::testRecoveryResetUpdatesCachedState() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(2U, 0.03F, 7.0F));
    transport.pushResetReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 0.0F));
    this->component.setTransportForTest(&transport);

    ASSERT_TRUE(this->component.pollStateForTest());

    this->clearHistory();
    OBC::ADCS::StateData state = {};
    ASSERT_EQ(this->component.resetAdcsForRecovery(state), Fw::CmdResponse::OK);

    ASSERT_EQ(state.mode, 0U);
    ASSERT_TLM_ADCS_MODE_SIZE(2);
    ASSERT_TLM_ADCS_MODE(0, OBC::AdcsMode::IDLE);
    ASSERT_TLM_ADCS_MODE(1, OBC::AdcsMode::IDLE);
    ASSERT_TLM_ADCS_Q0_SIZE(1);
    ASSERT_TLM_ADCS_Q0(0, 1.0);
    ASSERT_TLM_ADCS_OMEGA_X_SIZE(1);
    ASSERT_TLM_ADCS_MAG_X_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR_SIZE(1);
    ASSERT_TLM_ADCS_POINTING_ERR(0, 0.0F);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(0);

    OBC::ADCS::StateData cached = {};
    ASSERT_TRUE(this->component.getCachedStateForRuntime(cached));
    ASSERT_EQ(cached.mode, 0U);
    ASSERT_FLOAT_EQ(cached.omega_x, 0.12F);
    ASSERT_FLOAT_EQ(cached.pointing_error_deg, 0.0F);
}

void AdcsBridgeTester::testRecoveryResetFailureDoesNotMutateScheduledPollHealth() {
    FakeAdcsTransport transport;
    transport.pushStateReply(OBC::ADCS::TransportStatus::OK, makeState(0U, 0.12F, 18.0F));
    transport.pushResetReply(OBC::ADCS::TransportStatus::TIMEOUT, makeState(0U, 0.0F, 0.0F));
    this->component.setTransportForTest(&transport);

    ASSERT_TRUE(this->component.pollStateForTest());
    OBC::ADCS::PollHealthState before = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(before));

    this->clearHistory();
    OBC::ADCS::StateData state = {};
    ASSERT_EQ(this->component.resetAdcsForRecovery(state), Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_ADCS_COMM_ERROR_SIZE(1);

    OBC::ADCS::PollHealthState after = {};
    ASSERT_TRUE(this->component.getPollHealthForRuntime(after));
    ASSERT_EQ(after.lastScheduledTransportOk, before.lastScheduledTransportOk);
    ASSERT_EQ(after.lastScheduledValidRefresh, before.lastScheduledValidRefresh);
    ASSERT_EQ(after.consecutiveTransportFailures, before.consecutiveTransportFailures);
    ASSERT_EQ(after.consecutiveNoValidRefresh, before.consecutiveNoValidRefresh);
}

}  // namespace OBC
