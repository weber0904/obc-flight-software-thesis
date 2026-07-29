#include "GroundLinkDriverTester.hpp"

#include <cstring>

namespace OBC {

FakeGroundLinkBackend::FakeGroundLinkBackend(OBC::COMM::GroundLinkBackendMode mode)
    : m_stats(),
      m_healthSemantics(mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP
                            ? OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP
                        : (mode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP
                               ? OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK
                               : OBC::COMM::GroundLinkHealthSemantics::DISABLED)),
      m_successfulStatusObservations(0U),
      m_observationStateCallCount(0U) {
    this->m_stats.mode = mode;
}

bool FakeGroundLinkBackend::start() {
    return true;
}

void FakeGroundLinkBackend::stop() {
    this->m_stats.connected = false;
}

OBC::COMM::GroundLinkReceiveStatus FakeGroundLinkBackend::receive(std::string& outChunk, std::uint32_t timeoutMs) {
    static_cast<void>(timeoutMs);
    outChunk.clear();

    if (this->m_receiveReplies.empty()) {
        return OBC::COMM::GroundLinkReceiveStatus::IDLE;
    }

    const ReceiveReply reply = this->m_receiveReplies.front();
    this->m_receiveReplies.pop_front();
    this->m_stats.connected = reply.connected;

    if (reply.status == OBC::COMM::GroundLinkReceiveStatus::DATA) {
        outChunk = reply.chunk;
        this->m_stats.rxChunks += 1U;
        this->m_stats.rxBytes += static_cast<std::uint32_t>(reply.chunk.size());
        return reply.status;
    }

    if (reply.status == OBC::COMM::GroundLinkReceiveStatus::ERROR) {
        this->m_stats.rxErrors += 1U;
    }
    return reply.status;
}

OBC::COMM::GroundLinkSendStatus FakeGroundLinkBackend::send(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        this->m_stats.txErrors += 1U;
        return OBC::COMM::GroundLinkSendStatus::ERROR;
    }

    if (this->m_sendReplies.empty()) {
        this->m_stats.txErrors += 1U;
        return OBC::COMM::GroundLinkSendStatus::ERROR;
    }

    const auto reply = this->m_sendReplies.front();
    this->m_sendReplies.pop_front();
    this->m_stats.connected = reply.second;

    if (reply.first == OBC::COMM::GroundLinkSendStatus::OK) {
        this->m_stats.txChunks += 1U;
        this->m_stats.txBytes += static_cast<std::uint32_t>(size);
    } else {
        this->m_stats.txErrors += 1U;
    }
    return reply.first;
}

OBC::COMM::GroundLinkStats FakeGroundLinkBackend::getStats() const {
    return this->m_stats;
}

OBC::COMM::GroundLinkObservationState FakeGroundLinkBackend::getObservationState() const {
    auto* self = const_cast<FakeGroundLinkBackend*>(this);
    self->m_observationStateCallCount += 1U;
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

bool FakeGroundLinkBackend::observeHealth() {
    if (this->m_healthReplies.empty()) {
        return false;
    }

    const HealthReply reply = this->m_healthReplies.front();
    this->m_healthReplies.pop_front();
    this->m_stats.connected = reply.connected;
    if (reply.success) {
        this->m_successfulStatusObservations += 1U;
    }
    return reply.success;
}

void FakeGroundLinkBackend::queueReceive(const ReceiveReply& reply) {
    this->m_receiveReplies.push_back(reply);
}

void FakeGroundLinkBackend::queueSend(OBC::COMM::GroundLinkSendStatus status, bool connected) {
    this->m_sendReplies.push_back(std::make_pair(status, connected));
}

void FakeGroundLinkBackend::queueHealthObservation(bool success, bool connected) {
    this->m_healthReplies.push_back({success, connected});
}

void FakeGroundLinkBackend::setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics) {
    this->m_healthSemantics = healthSemantics;
}

U32 FakeGroundLinkBackend::getObservationStateCallCount() const {
    return this->m_observationStateCallCount;
}

GroundLinkDriverTester::GroundLinkDriverTester()
    : GroundLinkDriverGTestBase("GroundLinkDriverTester", MAX_HISTORY_SIZE),
      m_backend(),
      component("GroundLinkDriver"),
      m_receivedPayloads(),
      m_ownedReceiveBuffers() {
    this->initComponents();
    this->connectPorts();
}

GroundLinkDriverTester::~GroundLinkDriverTester() {
    for (U8* const buffer : this->m_ownedReceiveBuffers) {
        delete[] buffer;
    }
}

void GroundLinkDriverTester::installBackend(std::unique_ptr<OBC::FakeGroundLinkBackend> backend) {
    this->m_backend = std::move(backend);
    this->component.setBackendForTest(this->m_backend.get());
}

void GroundLinkDriverTester::testPumpPublishesReadyAndReceive() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(
        new OBC::FakeGroundLinkBackend(OBC::COMM::GroundLinkBackendMode::COMM_CSP));
    backend->queueReceive({OBC::COMM::GroundLinkReceiveStatus::DATA, "ABC", true});
    this->installBackend(std::move(backend));

    this->clearHistory();
    ASSERT_TRUE(this->component.pumpOnceForTest());

    ASSERT_from_ready_SIZE(1);
    ASSERT_EQ(this->m_receivedPayloads.size(), 1U);
    ASSERT_EQ(this->m_receivedPayloads[0], "ABC");
    ASSERT_TLM_GROUND_LINK_MODE_SIZE(1);
    ASSERT_TLM_GROUND_LINK_MODE(0, static_cast<U8>(OBC::COMM::GroundLinkBackendMode::COMM_CSP));
    ASSERT_TLM_GROUND_LINK_CONNECTED_SIZE(1);
    ASSERT_TLM_GROUND_LINK_CONNECTED(0, true);
    ASSERT_EVENTS_GROUND_LINK_UP_SIZE(1);
}

void GroundLinkDriverTester::testSendSuccessPublishesCounters() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(new OBC::FakeGroundLinkBackend());
    backend->queueSend(OBC::COMM::GroundLinkSendStatus::OK, true);
    this->installBackend(std::move(backend));

    this->clearHistory();
    U8 data[] = {'P', 'I', 'N', 'G'};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, buffer);

    ASSERT_EQ(status, Drv::ByteStreamStatus::OP_OK);
    ASSERT_TLM_GROUND_LINK_TX_BYTES_SIZE(1);
    ASSERT_TLM_GROUND_LINK_TX_BYTES(0, 4U);
    ASSERT_TLM_GROUND_LINK_TX_CHUNKS_SIZE(1);
    ASSERT_TLM_GROUND_LINK_TX_CHUNKS(0, 1U);
}

void GroundLinkDriverTester::testSendRetryReturnsRetry() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(new OBC::FakeGroundLinkBackend());
    backend->queueSend(OBC::COMM::GroundLinkSendStatus::RETRY, false);
    this->installBackend(std::move(backend));

    this->clearHistory();
    U8 data[] = {'P', 'I', 'N', 'G'};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, buffer);

    ASSERT_EQ(status, Drv::ByteStreamStatus::SEND_RETRY);
    ASSERT_EVENTS_GROUND_LINK_ERROR_SIZE(1);
}

void GroundLinkDriverTester::testReceiveErrorPublishesWarning() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(new OBC::FakeGroundLinkBackend());
    backend->queueReceive({OBC::COMM::GroundLinkReceiveStatus::ERROR, "", false});
    this->installBackend(std::move(backend));

    this->clearHistory();
    ASSERT_FALSE(this->component.pumpOnceForTest());

    ASSERT_EVENTS_GROUND_LINK_ERROR_SIZE(1);
    ASSERT_TLM_GROUND_LINK_RX_ERRORS_SIZE(1);
    ASSERT_TLM_GROUND_LINK_RX_ERRORS(0, 1U);
}

void GroundLinkDriverTester::testRecvReturnDeallocates() {
    this->clearHistory();
    U8* const data = new U8[4];
    std::memset(data, 0, 4);
    Fw::Buffer buffer(data, 4U);
    this->invoke_to_recvReturnIn(0, buffer);

    ASSERT_from_deallocate_SIZE(1);
}

void GroundLinkDriverTester::testObserveHealthPublishesObservationState() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(
        new OBC::FakeGroundLinkBackend(OBC::COMM::GroundLinkBackendMode::COMM_CSP));
    backend->queueHealthObservation(true, true);
    this->installBackend(std::move(backend));

    this->clearHistory();
    ASSERT_TRUE(this->component.observeHealthForRuntime());

    const OBC::COMM::GroundLinkObservationState observation = this->component.getObservationForRuntime();
    ASSERT_EQ(observation.mode, OBC::COMM::GroundLinkBackendMode::COMM_CSP);
    ASSERT_EQ(observation.healthSemantics, OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    ASSERT_TRUE(observation.connected);
    ASSERT_EQ(observation.txBytes, 0U);
    ASSERT_EQ(observation.rxBytes, 0U);
    ASSERT_EQ(observation.successfulStatusObservations, 1U);
    ASSERT_TLM_GROUND_LINK_CONNECTED_SIZE(1);
    ASSERT_TLM_GROUND_LINK_CONNECTED(0, true);
    ASSERT_EVENTS_GROUND_LINK_UP_SIZE(1);
}

void GroundLinkDriverTester::testRuntimeObservationReadsCachedSnapshot() {
    std::unique_ptr<OBC::FakeGroundLinkBackend> backend(
        new OBC::FakeGroundLinkBackend(OBC::COMM::GroundLinkBackendMode::COMM_CSP));
    this->installBackend(std::move(backend));

    ASSERT_EQ(this->m_backend->getObservationStateCallCount(), 1U);

    this->m_backend->queueReceive({OBC::COMM::GroundLinkReceiveStatus::DATA, "XYZ", true});
    ASSERT_TRUE(this->component.pumpOnceForTest());
    ASSERT_EQ(this->m_backend->getObservationStateCallCount(), 2U);

    const OBC::COMM::GroundLinkObservationState first = this->component.getObservationForRuntime();
    const OBC::COMM::GroundLinkObservationState second = this->component.getObservationForRuntime();

    ASSERT_EQ(this->m_backend->getObservationStateCallCount(), 2U);
    ASSERT_EQ(first.mode, OBC::COMM::GroundLinkBackendMode::COMM_CSP);
    ASSERT_TRUE(first.connected);
    ASSERT_EQ(first.rxChunks, 1U);
    ASSERT_EQ(first.rxBytes, 3U);
    ASSERT_EQ(second.rxChunks, first.rxChunks);
    ASSERT_EQ(second.rxBytes, first.rxBytes);
}

void GroundLinkDriverTester::from_recv_handler(FwIndexType portNum,
                                               Fw::Buffer& recvBuffer,
                                               const Drv::ByteStreamStatus& recvStatus) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_recv(recvBuffer, recvStatus);
    this->m_receivedPayloads.emplace_back(reinterpret_cast<const char*>(recvBuffer.getData()), recvBuffer.getSize());
    this->m_ownedReceiveBuffers.push_back(recvBuffer.getData());
}

Fw::Buffer GroundLinkDriverTester::from_allocate_handler(FwIndexType portNum, FwSizeType size) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_allocate(size);
    return Fw::Buffer(new U8[size], size);
}

void GroundLinkDriverTester::from_deallocate_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_deallocate(fwBuffer);
    delete[] fwBuffer.getData();
}

}  // namespace OBC
