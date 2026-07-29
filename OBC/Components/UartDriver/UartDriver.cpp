#include "OBC/Components/UartDriver/UartDriver.hpp"

namespace {

bool equalByteStreamStats(const OBC::COMM::ByteStreamStats& lhs, const OBC::COMM::ByteStreamStats& rhs) {
    return lhs.txBytes == rhs.txBytes && lhs.rxBytes == rhs.rxBytes && lhs.txErrors == rhs.txErrors &&
           lhs.rxErrors == rhs.rxErrors && lhs.connected == rhs.connected;
}

}  // namespace

namespace OBC {

UartDriver::UartDriver(const char* const compName)
    : UartDriverComponentBase(compName),
      m_transport(nullptr),
      m_openLatched(false),
      m_pendingRuntimeSend(),
      m_pendingRuntimeSendSize(0U),
      m_havePendingRuntimeSend(false),
      m_havePublishedStats(false),
      m_lastPublishedStats() {}

UartDriver::~UartDriver() = default;

void UartDriver::configureTransport(std::unique_ptr<OBC::COMM::IByteStreamTransport> transport) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_sharedTransport.reset();
    this->m_ownedTransport = std::move(transport);
    this->m_transport = this->m_ownedTransport.get();
    this->m_openLatched = false;
    this->m_havePendingRuntimeSend = false;
    this->m_pendingRuntimeSendSize = 0U;
    this->m_havePublishedStats = false;
    this->m_lastPublishedStats = {};
}

void UartDriver::configureTransport(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& transport) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_ownedTransport.reset();
    this->m_sharedTransport = transport;
    this->m_transport = this->m_sharedTransport.get();
    this->m_openLatched = false;
    this->m_havePendingRuntimeSend = false;
    this->m_pendingRuntimeSendSize = 0U;
    this->m_havePublishedStats = false;
    this->m_lastPublishedStats = {};
}

void UartDriver::setTransportForTest(OBC::COMM::IByteStreamTransport* transport) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_ownedTransport.reset();
    this->m_sharedTransport.reset();
    this->m_transport = transport;
    this->m_openLatched = false;
    this->m_havePendingRuntimeSend = false;
    this->m_pendingRuntimeSendSize = 0U;
    this->m_havePublishedStats = false;
    this->m_lastPublishedStats = {};
}

bool UartDriver::pollForTest() {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    return this->pollLocked_();
}

bool UartDriver::exchangeForTest(const std::string& request, std::string& response) {
    return this->exchangeDelimitedForTest(request, response, '\n');
}

bool UartDriver::exchangeDelimitedForTest(const std::string& request, std::string& response, char delimiter) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    if (this->m_transport == nullptr) {
        return false;
    }

    const OBC::COMM::ByteStreamStatus status = this->m_transport->exchangeDelimited(request, response, delimiter);
    this->publishStats_();
    if (status != OBC::COMM::ByteStreamStatus::OK) {
        this->emitError_(status == OBC::COMM::ByteStreamStatus::TIMEOUT ? 2U : 1U);
        return false;
    }

    if (this->m_transport->isConnected() && !this->m_openLatched) {
        this->log_ACTIVITY_HI_UART_OPEN();
        this->m_openLatched = true;
    }
    return true;
}

bool UartDriver::exchangeForRuntime(const std::string& request, std::string& response) {
    return this->exchangeForTest(request, response);
}

bool UartDriver::exchangeDelimitedForRuntime(const std::string& request, std::string& response, char delimiter) {
    return this->exchangeDelimitedForTest(request, response, delimiter);
}

bool UartDriver::sendForRuntime(const U8* data, U32 size) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    return this->sendImmediate_(data, size);
}

bool UartDriver::queueSendForRuntime(const U8* data, U32 size) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    if (this->m_transport == nullptr || !this->m_transport->isConnected() || data == nullptr || size == 0U ||
        size > MAX_RUNTIME_SEND_SIZE || this->m_havePendingRuntimeSend) {
        return false;
    }

    ::memcpy(this->m_pendingRuntimeSend.data(), data, size);
    this->m_pendingRuntimeSendSize = size;
    this->m_havePendingRuntimeSend = true;
    return true;
}

bool UartDriver::sendImmediate_(const U8* data, U32 size) {
    if (this->m_transport == nullptr || data == nullptr || size == 0U) {
        return false;
    }

    const OBC::COMM::ByteStreamStatus status =
        this->m_transport->send(reinterpret_cast<const std::uint8_t*>(data), static_cast<std::size_t>(size));
    this->publishStats_();
    if (status != OBC::COMM::ByteStreamStatus::OK) {
        this->emitError_(status == OBC::COMM::ByteStreamStatus::TIMEOUT ? 2U : 1U);
        return false;
    }

    if (this->m_transport->isConnected() && !this->m_openLatched) {
        this->log_ACTIVITY_HI_UART_OPEN();
        this->m_openLatched = true;
    }
    return true;
}

OBC::COMM::ByteStreamStats UartDriver::getStatsForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    if (this->m_transport == nullptr) {
        return OBC::COMM::ByteStreamStats{0U, 0U, 0U, 0U, false};
    }
    return this->m_transport->getStats();
}

void UartDriver::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    static_cast<void>(this->pollLocked_());
}

bool UartDriver::pollLocked_() {
    if (this->m_transport == nullptr) {
        return false;
    }

    if (!this->m_transport->isConnected()) {
        if (this->m_transport->connect()) {
            if (!this->m_openLatched) {
                this->log_ACTIVITY_HI_UART_OPEN();
                this->m_openLatched = true;
            }
        } else {
            this->emitError_(1U);
        }
    }

    if (!this->m_transport->isConnected()) {
        this->m_openLatched = false;
    }

    bool sentPending = false;
    if (this->m_havePendingRuntimeSend && this->m_transport->isConnected()) {
        const U32 size = this->m_pendingRuntimeSendSize;
        this->m_havePendingRuntimeSend = false;
        this->m_pendingRuntimeSendSize = 0U;
        sentPending = true;
        static_cast<void>(this->sendImmediate_(this->m_pendingRuntimeSend.data(), size));
    }

    if (!sentPending) {
        this->publishStats_();
    }
    return this->m_transport->isConnected();
}

void UartDriver::publishStats_() {
    if (this->m_transport == nullptr) {
        return;
    }

    const OBC::COMM::ByteStreamStats stats = this->m_transport->getStats();
    if (!this->shouldPublishStats_(stats)) {
        return;
    }
    this->tlmWrite_UART_TX_BYTES(stats.txBytes);
    this->tlmWrite_UART_RX_BYTES(stats.rxBytes);
    this->tlmWrite_UART_TX_ERRORS(stats.txErrors);
    this->tlmWrite_UART_RX_ERRORS(stats.rxErrors);
    this->tlmWrite_UART_CONNECTED(stats.connected);
}

void UartDriver::emitError_(U32 code) {
    this->log_WARNING_LO_UART_ERROR(code);
}

bool UartDriver::shouldPublishStats_(const OBC::COMM::ByteStreamStats& stats) {
    if (!this->m_havePublishedStats || !equalByteStreamStats(this->m_lastPublishedStats, stats)) {
        this->m_havePublishedStats = true;
        this->m_lastPublishedStats = stats;
        return true;
    }
    return false;
}

}  // namespace OBC
