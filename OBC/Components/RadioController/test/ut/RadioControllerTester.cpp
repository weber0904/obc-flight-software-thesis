#include "RadioControllerTester.hpp"

#include <deque>

namespace OBC {

namespace {

class FakeByteStreamTransport final : public OBC::COMM::IByteStreamTransport {
  public:
    bool connect() override { return true; }

    void disconnect() override {}

    bool isConnected() const override { return true; }

    OBC::COMM::ByteStreamStatus exchange(const std::string& request, std::string& response) override {
        static_cast<void>(request);
        response.clear();
        return OBC::COMM::ByteStreamStatus::OK;
    }

    OBC::COMM::ByteStreamStatus exchangeDelimited(const std::string& request,
                                                  std::string& response,
                                                  char delimiter) override {
        static_cast<void>(delimiter);
        return this->exchange(request, response);
    }

    OBC::COMM::ByteStreamStatus send(const std::uint8_t* data, std::size_t size) override {
        static_cast<void>(data);
        static_cast<void>(size);
        return OBC::COMM::ByteStreamStatus::OK;
    }

    OBC::COMM::ByteStreamStats getStats() const override {
        return OBC::COMM::ByteStreamStats{0U, 0U, 0U, 0U, true};
    }
};

class FakeRadioTransport final : public OBC::COMM::IRadioTransport {
  public:
    struct Reply {
        OBC::COMM::RadioTransportStatus status;
        OBC::COMM::RadioStatus data;
    };

    void pushGetReply(OBC::COMM::RadioTransportStatus status, const OBC::COMM::RadioStatus& data) {
        this->m_getReplies.push_back({status, data});
    }

    void pushEnableReply(OBC::COMM::RadioTransportStatus status, const OBC::COMM::RadioStatus& data) {
        this->m_enableReplies.push_back({status, data});
    }

    void pushPowerReply(OBC::COMM::RadioTransportStatus status, const OBC::COMM::RadioStatus& data) {
        this->m_powerReplies.push_back({status, data});
    }

    void pushFreqReply(OBC::COMM::RadioTransportStatus status, const OBC::COMM::RadioStatus& data) {
        this->m_freqReplies.push_back({status, data});
    }

    OBC::COMM::RadioTransportStatus getStatus(OBC::COMM::RadioStatus& status) override {
        return this->popReply_(this->m_getReplies, status);
    }

    OBC::COMM::RadioTransportStatus setEnabled(bool enabled, OBC::COMM::RadioStatus& status) override {
        static_cast<void>(enabled);
        return this->popReply_(this->m_enableReplies, status);
    }

    OBC::COMM::RadioTransportStatus setPower(std::uint8_t powerDbm, OBC::COMM::RadioStatus& status) override {
        static_cast<void>(powerDbm);
        return this->popReply_(this->m_powerReplies, status);
    }

    OBC::COMM::RadioTransportStatus setFrequency(std::uint32_t freqHz, OBC::COMM::RadioStatus& status) override {
        static_cast<void>(freqHz);
        return this->popReply_(this->m_freqReplies, status);
    }

    OBC::COMM::ByteStreamStats getLinkStats() const override {
        return OBC::COMM::ByteStreamStats{0U, 0U, 0U, 0U, true};
    }

  private:
    OBC::COMM::RadioTransportStatus popReply_(std::deque<Reply>& replies, OBC::COMM::RadioStatus& status) {
        if (replies.empty()) {
            return OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR;
        }

        const Reply reply = replies.front();
        replies.pop_front();
        status = reply.data;
        return reply.status;
    }

  private:
    std::deque<Reply> m_getReplies;
    std::deque<Reply> m_enableReplies;
    std::deque<Reply> m_powerReplies;
    std::deque<Reply> m_freqReplies;
};

OBC::COMM::RadioStatus makeStatus(bool enabled,
                                  std::uint8_t powerDbm,
                                  std::uint32_t freqHz,
                                  float tempC = 32.5F,
                                  std::int16_t rssiDbm = -68) {
    OBC::COMM::RadioStatus status = {};
    status.enabled = enabled;
    status.powerDbm = powerDbm;
    status.freqHz = freqHz;
    status.temperatureC = tempC;
    status.rssiDbm = rssiDbm;
    return status;
}

}  // namespace

RadioControllerTester::RadioControllerTester()
    : RadioControllerGTestBase("RadioControllerTester", MAX_HISTORY_SIZE), component("RadioController") {
    this->initComponents();
    this->connectPorts();
}

RadioControllerTester::~RadioControllerTester() = default;

void RadioControllerTester::testGetStatusPublishesTelemetry() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 20U, 437000000U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_RADIO_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_RADIO_GET_STATUS, 0, Fw::CmdResponse::OK);
    ASSERT_TLM_RADIO_ENABLED_SIZE(1);
    ASSERT_TLM_RADIO_ENABLED(0, true);
    ASSERT_TLM_RADIO_TX_POWER_SIZE(1);
    ASSERT_TLM_RADIO_TX_POWER(0, 20U);
    ASSERT_TLM_RADIO_FREQ_SIZE(1);
    ASSERT_TLM_RADIO_FREQ(0, 437000000U);
}

void RadioControllerTester::testEnablePublishesPowerEvent() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(false, 10U, 437000000U));
    transport.pushEnableReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 10U, 437000000U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_RADIO_GET_STATUS(TEST_INSTANCE_ID, 0);

    this->clearHistory();
    this->sendCmd_RADIO_ENABLE(TEST_INSTANCE_ID, 1, true);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_RADIO_ENABLE, 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_RADIO_POWERED_ON_SIZE(1);
    ASSERT_TLM_RADIO_ENABLED_SIZE(1);
    ASSERT_TLM_RADIO_ENABLED(0, true);
}

void RadioControllerTester::testOvertempRaisesEvent() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 18U, 437000000U, 74.0F));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_RADIO_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_EVENTS_RADIO_OVERTEMP_SIZE(1);
    ASSERT_EVENTS_RADIO_OVERTEMP(0, 74.0F);
}

void RadioControllerTester::testScheduledPollPublishesSummaryOnly() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 18U, 437000000U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollStatusForTest());

    ASSERT_TLM_RADIO_ENABLED_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_SAMPLE_AVAILABLE_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_RESULT_SIZE(1);
    ASSERT_TLM_RADIO_TX_POWER_SIZE(0);
    ASSERT_TLM_RADIO_FREQ_SIZE(0);
    ASSERT_TLM_RADIO_TEMP_SIZE(0);
    ASSERT_TLM_RADIO_RSSI_SIZE(0);
}

void RadioControllerTester::testInvalidReplyMapsToValidationError() {
    FakeRadioTransport transport;
    transport.pushPowerReply(OBC::COMM::RadioTransportStatus::INVALID_RESPONSE, makeStatus(true, 18U, 437000000U));
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    this->sendCmd_RADIO_SET_POWER(TEST_INSTANCE_ID, 0, 18U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_RADIO_SET_POWER, 0, Fw::CmdResponse::VALIDATION_ERROR);
}

void RadioControllerTester::testCachedObservationAgesWhenPollFails() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 15U, 437000000U));
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::TIMEOUT, makeStatus(true, 15U, 437000000U));
    this->component.setTransportForTest(&transport);

    ASSERT_TRUE(this->component.pollStatusForTest());

    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);

    const OBC::RadioObservationState observation = this->component.getObservationForRuntime();
    ASSERT_TRUE(observation.haveSample);
    ASSERT_EQ(observation.statusAgeTicks, 1U);
    ASSERT_EQ(observation.lastResult, OBC::RadioObservationResult::TIMEOUT);
    ASSERT_TRUE(observation.lastStatus.enabled);
    ASSERT_EQ(observation.lastStatus.powerDbm, 15U);
    ASSERT_TLM_RADIO_STATUS_SAMPLE_AVAILABLE_SIZE(0);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS_SIZE(2);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS(0, 0U);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS(1, 1U);
    ASSERT_TLM_RADIO_STATUS_RESULT_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_RESULT(0, static_cast<U32>(OBC::RadioObservationResult::TIMEOUT));
}

void RadioControllerTester::testUnavailableObservationWithoutSample() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::INVALID_RESPONSE, makeStatus(true, 20U, 437000000U));
    this->component.setTransportForTest(&transport);

    ASSERT_FALSE(this->component.pollStatusForTest());

    const OBC::RadioObservationState observation = this->component.getObservationForRuntime();
    ASSERT_FALSE(observation.haveSample);
    ASSERT_EQ(observation.statusAgeTicks, 0U);
    ASSERT_EQ(observation.lastResult, OBC::RadioObservationResult::INVALID_RESPONSE);
}

void RadioControllerTester::testTransportErrorUpdatesCachedObservation() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(false, 12U, 437000000U));
    this->component.setTransportForTest(&transport);
    ASSERT_TRUE(this->component.pollStatusForTest());

    ASSERT_FALSE(this->component.pollStatusForTest());

    const OBC::RadioObservationState observation = this->component.getObservationForRuntime();
    ASSERT_TRUE(observation.haveSample);
    ASSERT_EQ(observation.lastResult, OBC::RadioObservationResult::TRANSPORT_ERROR);
}

void RadioControllerTester::testUnsupportedProtocolUpdatesObservation() {
    auto transport = std::make_unique<OBC::COMM::HostedRadioTransport>(
        std::shared_ptr<OBC::COMM::IByteStreamTransport>(new FakeByteStreamTransport()),
        OBC::COMM::makeRadioProtocolAdapter("transparent-passive"));
    this->component.configureTransport(std::move(transport));

    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);

    const OBC::RadioObservationState observation = this->component.getObservationForRuntime();
    ASSERT_FALSE(observation.haveSample);
    ASSERT_EQ(observation.statusAgeTicks, 0U);
    ASSERT_EQ(observation.lastResult, OBC::RadioObservationResult::UNSUPPORTED);
    ASSERT_TLM_RADIO_STATUS_SAMPLE_AVAILABLE_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_SAMPLE_AVAILABLE(0, false);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS(0, 0U);
    ASSERT_TLM_RADIO_STATUS_RESULT_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_RESULT(0, static_cast<U32>(OBC::RadioObservationResult::UNSUPPORTED));
}

void RadioControllerTester::testSetterFailureUpdatesCachedObservation() {
    FakeRadioTransport transport;
    transport.pushGetReply(OBC::COMM::RadioTransportStatus::OK, makeStatus(true, 10U, 437000000U));
    transport.pushPowerReply(OBC::COMM::RadioTransportStatus::INVALID_RESPONSE, makeStatus(true, 18U, 437000000U));
    this->component.setTransportForTest(&transport);

    ASSERT_TRUE(this->component.pollStatusForTest());

    this->clearHistory();
    this->sendCmd_RADIO_SET_POWER(TEST_INSTANCE_ID, 0, 18U);

    const OBC::RadioObservationState observation = this->component.getObservationForRuntime();
    ASSERT_TRUE(observation.haveSample);
    ASSERT_EQ(observation.statusAgeTicks, 0U);
    ASSERT_EQ(observation.lastResult, OBC::RadioObservationResult::INVALID_RESPONSE);
    ASSERT_TRUE(observation.lastStatus.enabled);
    ASSERT_EQ(observation.lastStatus.powerDbm, 10U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_RADIO_SET_POWER, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_TLM_RADIO_STATUS_SAMPLE_AVAILABLE_SIZE(0);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_AGE_TICKS(0, 0U);
    ASSERT_TLM_RADIO_STATUS_RESULT_SIZE(1);
    ASSERT_TLM_RADIO_STATUS_RESULT(0, static_cast<U32>(OBC::RadioObservationResult::INVALID_RESPONSE));
}

}  // namespace OBC
