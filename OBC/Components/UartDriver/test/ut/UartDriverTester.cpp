#include "UartDriverTester.hpp"

#include <deque>

namespace OBC {

namespace {

class FakeByteStreamTransport final : public OBC::COMM::IByteStreamTransport {
  public:
    struct ExchangeReply {
        OBC::COMM::ByteStreamStatus status;
        std::string response;
    };

    FakeByteStreamTransport() : m_canConnect(true), m_connected(false), m_stats{0U, 0U, 0U, 0U, false} {}

    void setCanConnect(bool canConnect) {
        this->m_canConnect = canConnect;
    }

    void pushExchangeReply(OBC::COMM::ByteStreamStatus status, const std::string& response) {
        this->m_exchangeReplies.push_back({status, response});
    }

    bool connect() override {
        if (!this->m_canConnect) {
            this->m_stats.txErrors += 1U;
            this->m_connected = false;
            this->m_stats.connected = false;
            return false;
        }

        this->m_connected = true;
        this->m_stats.connected = true;
        return true;
    }

    void disconnect() override {
        this->m_connected = false;
        this->m_stats.connected = false;
    }

    bool isConnected() const override {
        return this->m_connected;
    }

    OBC::COMM::ByteStreamStatus exchange(const std::string& request, std::string& response) override {
        return this->exchangeDelimited(request, response, '\n');
    }

    OBC::COMM::ByteStreamStatus exchangeDelimited(const std::string& request,
                                                  std::string& response,
                                                  char delimiter) override {
        if (!this->m_connected && !this->connect()) {
            return OBC::COMM::ByteStreamStatus::IO_ERROR;
        }

        this->m_stats.txBytes += static_cast<std::uint32_t>(request.size());
        if (this->m_exchangeReplies.empty()) {
            this->m_stats.rxErrors += 1U;
            return OBC::COMM::ByteStreamStatus::IO_ERROR;
        }

        const ExchangeReply reply = this->m_exchangeReplies.front();
        this->m_exchangeReplies.pop_front();
        response = reply.response;
        static_cast<void>(delimiter);

        if (reply.status == OBC::COMM::ByteStreamStatus::OK) {
            this->m_stats.rxBytes += static_cast<std::uint32_t>(reply.response.size());
        } else {
            this->m_stats.rxErrors += 1U;
        }

        return reply.status;
    }

    OBC::COMM::ByteStreamStatus send(const std::uint8_t* data, std::size_t size) override {
        if (!this->m_connected && !this->connect()) {
            return OBC::COMM::ByteStreamStatus::IO_ERROR;
        }
        if (data == nullptr || size == 0U) {
            this->m_stats.txErrors += 1U;
            return OBC::COMM::ByteStreamStatus::IO_ERROR;
        }
        this->m_stats.txBytes += static_cast<std::uint32_t>(size);
        return OBC::COMM::ByteStreamStatus::OK;
    }

    OBC::COMM::ByteStreamStats getStats() const override {
        return this->m_stats;
    }

  private:
    bool m_canConnect;
    bool m_connected;
    OBC::COMM::ByteStreamStats m_stats;
    std::deque<ExchangeReply> m_exchangeReplies;
};

}  // namespace

UartDriverTester::UartDriverTester() : UartDriverGTestBase("UartDriverTester", MAX_HISTORY_SIZE), component("UartDriver") {
    this->initComponents();
    this->connectPorts();
}

UartDriverTester::~UartDriverTester() = default;

void UartDriverTester::testConnectPublishesOpenAndTelemetry() {
    FakeByteStreamTransport transport;
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollForTest());

    ASSERT_EVENTS_UART_OPEN_SIZE(1);
    ASSERT_TLM_UART_CONNECTED_SIZE(1);
    ASSERT_TLM_UART_CONNECTED(0, true);
}

void UartDriverTester::testExchangeUpdatesCounters() {
    FakeByteStreamTransport transport;
    transport.pushExchangeReply(OBC::COMM::ByteStreamStatus::OK, "STATUS enabled=1\n");
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollForTest());

    this->clearHistory();
    std::string response;
    ASSERT_TRUE(this->component.exchangeForTest("STATUS\n", response));

    ASSERT_EQ(response, "STATUS enabled=1\n");
    ASSERT_TLM_UART_TX_BYTES_SIZE(1);
    ASSERT_TLM_UART_RX_BYTES_SIZE(1);
    ASSERT_TLM_UART_TX_BYTES(0, 7U);
    ASSERT_TLM_UART_RX_BYTES(0, 17U);
}

void UartDriverTester::testTimeoutPublishesError() {
    FakeByteStreamTransport transport;
    transport.pushExchangeReply(OBC::COMM::ByteStreamStatus::TIMEOUT, "");
    this->component.setTransportForTest(&transport);

    this->clearHistory();
    ASSERT_TRUE(this->component.pollForTest());

    this->clearHistory();
    std::string response;
    ASSERT_FALSE(this->component.exchangeForTest("STATUS\n", response));

    ASSERT_EVENTS_UART_ERROR_SIZE(1);
    ASSERT_TLM_UART_RX_ERRORS_SIZE(1);
    ASSERT_TLM_UART_RX_ERRORS(0, 1U);
}

void UartDriverTester::testRuntimeSendReportsTransportResult() {
    FakeByteStreamTransport transport;
    this->component.setTransportForTest(&transport);

    const U8 payload[] = {0x4FU, 0x42U, 0x43U};
    this->clearHistory();
    ASSERT_TRUE(this->component.sendForRuntime(payload, sizeof(payload)));

    ASSERT_EVENTS_UART_OPEN_SIZE(1);
    ASSERT_TLM_UART_TX_BYTES_SIZE(1);
    ASSERT_TLM_UART_TX_BYTES(0, sizeof(payload));
}

void UartDriverTester::testRuntimeSendFailsWithoutTransport() {
    const U8 payload[] = {0x4FU, 0x42U, 0x43U};
    this->clearHistory();
    ASSERT_FALSE(this->component.sendForRuntime(payload, sizeof(payload)));

    ASSERT_EVENTS_UART_OPEN_SIZE(0);
    ASSERT_EVENTS_UART_ERROR_SIZE(0);
    ASSERT_TLM_UART_TX_BYTES_SIZE(0);
}

void UartDriverTester::testRuntimeSendFailurePublishesError() {
    FakeByteStreamTransport transport;
    transport.setCanConnect(false);
    this->component.setTransportForTest(&transport);

    const U8 payload[] = {0x4FU, 0x42U, 0x43U};
    this->clearHistory();
    ASSERT_FALSE(this->component.sendForRuntime(payload, sizeof(payload)));

    ASSERT_EVENTS_UART_OPEN_SIZE(0);
    ASSERT_EVENTS_UART_ERROR_SIZE(1);
    ASSERT_TLM_UART_TX_ERRORS_SIZE(1);
    ASSERT_TLM_UART_TX_ERRORS(0, 1U);
}

void UartDriverTester::testQueuedRuntimeSendDrainsOnPoll() {
    FakeByteStreamTransport transport;
    this->component.setTransportForTest(&transport);

    const U8 payload[] = {0x4FU, 0x42U, 0x43U};
    this->clearHistory();
    ASSERT_TRUE(this->component.pollForTest());
    this->clearHistory();
    ASSERT_TRUE(this->component.queueSendForRuntime(payload, sizeof(payload)));
    ASSERT_TLM_UART_TX_BYTES_SIZE(0);

    ASSERT_TRUE(this->component.pollForTest());
    ASSERT_EVENTS_UART_OPEN_SIZE(0);
    ASSERT_TLM_UART_TX_BYTES_SIZE(1);
    ASSERT_TLM_UART_TX_BYTES(0, sizeof(payload));
}

void UartDriverTester::testQueuedRuntimeSendRejectsDisconnectedTransport() {
    FakeByteStreamTransport transport;
    this->component.setTransportForTest(&transport);

    const U8 payload[] = {0x4FU, 0x42U, 0x43U};
    this->clearHistory();
    ASSERT_FALSE(this->component.queueSendForRuntime(payload, sizeof(payload)));
    ASSERT_TLM_UART_TX_BYTES_SIZE(0);
}

void UartDriverTester::testQueuedRuntimeSendRejectsFullQueue() {
    FakeByteStreamTransport transport;
    this->component.setTransportForTest(&transport);

    const U8 first[] = {0x4FU, 0x42U, 0x43U};
    const U8 second[] = {0x31U, 0x32U, 0x33U};
    this->clearHistory();
    ASSERT_TRUE(this->component.pollForTest());
    this->clearHistory();
    ASSERT_TRUE(this->component.queueSendForRuntime(first, sizeof(first)));
    ASSERT_FALSE(this->component.queueSendForRuntime(second, sizeof(second)));
    ASSERT_TLM_UART_TX_BYTES_SIZE(0);

    ASSERT_TRUE(this->component.pollForTest());
    ASSERT_TLM_UART_TX_BYTES_SIZE(1);
    ASSERT_TLM_UART_TX_BYTES(0, sizeof(first));
}

}  // namespace OBC
