#include "OBC/Components/GroundLinkDriver/GroundLinkDriver.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <limits>

namespace OBC {

namespace {

static constexpr std::uint32_t DEFAULT_POLL_TIMEOUT_MS = 1000U;
static constexpr std::uint32_t DISABLED_ERROR_CODE = 1U;
static constexpr std::uint32_t ALLOCATE_ERROR_CODE = 2U;
static constexpr std::uint32_t SEND_ERROR_CODE = 3U;
static constexpr std::uint32_t RECEIVE_ERROR_CODE = 4U;
static constexpr std::uint32_t DISABLED_BACKOFF_MS = 50U;
static constexpr std::uint32_t DISCONNECTED_BACKOFF_MS = 50U;

std::uint32_t readTimeoutOverrideOrDefault(const char* name, std::uint32_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno == ERANGE || end == value || *end != '\0' || parsed == 0ULL ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<std::uint32_t>::max())) {
        return fallback;
    }
    return static_cast<std::uint32_t>(parsed);
}

bool equalGroundLinkStats(const OBC::COMM::GroundLinkStats& lhs, const OBC::COMM::GroundLinkStats& rhs) {
    return lhs.mode == rhs.mode && lhs.connected == rhs.connected && lhs.txChunks == rhs.txChunks &&
           lhs.rxChunks == rhs.rxChunks && lhs.txBytes == rhs.txBytes && lhs.rxBytes == rhs.rxBytes &&
           lhs.txErrors == rhs.txErrors && lhs.rxErrors == rhs.rxErrors;
}

}  // namespace

GroundLinkDriver::GroundLinkDriver(const char* const compName)
    : GroundLinkDriverComponentBase(compName),
      m_backend(),
      m_workerThread(),
      m_running(false),
      m_connectedLatched(false),
      m_havePublishedStats(false),
      m_lastPublishedStats(),
      m_lastObservation() {}

GroundLinkDriver::~GroundLinkDriver() {
    this->stopWithObservability_(false);
    this->join();
}

void GroundLinkDriver::configureDirectTcp(const std::string& host, std::uint16_t port, std::size_t maxChunkBytes) {
    this->installOwnedBackend_(OBC::COMM::makeDirectTcpGroundLinkBackend(host, port, maxChunkBytes));
}

void GroundLinkDriver::configureCommCsp(std::uint16_t targetNode,
                                        OBC::CSP::ICspRuntime& runtime,
                                        OBC::COMM::GroundLinkHealthSemantics healthSemantics) {
    this->installOwnedBackend_(OBC::COMM::makeCommCspGroundLinkBackend(targetNode, runtime, healthSemantics));
}

void GroundLinkDriver::clearConfiguration() {
    this->stop();
    this->join();

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_backend.reset();
        this->m_connectedLatched = false;
        this->m_havePublishedStats = false;
        this->m_lastPublishedStats = {};
        this->m_lastObservation = {};
    }

    OBC::COMM::GroundLinkStats stats = {};
    this->publishStats_(stats);
}

void GroundLinkDriver::setBackendForTest(OBC::COMM::IGroundLinkBackend* backend) {
    this->stop();
    this->join();

    const OBC::COMM::GroundLinkObservationState initialObservation =
        backend != nullptr ? backend->getObservationState() : OBC::COMM::GroundLinkObservationState{};
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_backend = std::shared_ptr<OBC::COMM::IGroundLinkBackend>(backend, [](OBC::COMM::IGroundLinkBackend*) {});
    this->m_connectedLatched = false;
    this->m_havePublishedStats = false;
    this->m_lastPublishedStats = {};
    this->m_lastObservation = initialObservation;
}

bool GroundLinkDriver::start() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_workerThread.joinable()) {
        return true;
    }
    if (this->m_backend == nullptr) {
        this->emitError_(DISABLED_ERROR_CODE);
        return false;
    }

    this->m_running.store(true);
    this->m_workerThread = std::thread([this]() { this->workerLoop_(); });
    return true;
}

void GroundLinkDriver::stop() {
    this->stopWithObservability_(true);
}

void GroundLinkDriver::stopWithObservability_(bool publishObservability) {
    std::shared_ptr<OBC::COMM::IGroundLinkBackend> backend;
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        backend = this->m_backend;
        this->m_running.store(false);
    }

    if (backend != nullptr) {
        backend->stop();
        if (publishObservability) {
            this->publishStats_(backend->getStats());
            this->updateObservationCache_(backend);
        }
    }
}

void GroundLinkDriver::join() {
    if (this->m_workerThread.joinable()) {
        this->m_workerThread.join();
    }
}

bool GroundLinkDriver::pumpOnceForTest(std::uint32_t timeoutMs) {
    return this->runReceiveOnce_(timeoutMs);
}

OBC::COMM::GroundLinkStats GroundLinkDriver::getStatsForRuntime() const {
    const std::shared_ptr<OBC::COMM::IGroundLinkBackend> backend = this->getBackend_();
    if (backend == nullptr) {
        return OBC::COMM::GroundLinkStats{};
    }
    return backend->getStats();
}

OBC::COMM::GroundLinkObservationState GroundLinkDriver::getObservationForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastObservation;
}

bool GroundLinkDriver::isRunningForRuntime() const {
    return this->m_running.load();
}

bool GroundLinkDriver::observeHealthForRuntime() {
    const std::shared_ptr<OBC::COMM::IGroundLinkBackend> backend = this->getBackend_();
    if (backend == nullptr) {
        return false;
    }

    const bool observed = backend->observeHealth();
    this->publishStats_(backend->getStats());
    this->updateObservationCache_(backend);
    return observed;
}

Drv::ByteStreamStatus GroundLinkDriver::send_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);

    const std::shared_ptr<OBC::COMM::IGroundLinkBackend> backend = this->getBackend_();
    if (backend == nullptr) {
        this->emitError_(DISABLED_ERROR_CODE);
        return Drv::ByteStreamStatus::SEND_RETRY;
    }

    const OBC::COMM::GroundLinkSendStatus status = backend->send(fwBuffer.getData(), fwBuffer.getSize());
    const OBC::COMM::GroundLinkStats stats = backend->getStats();
    this->publishStats_(stats);
    this->updateObservationCache_(backend);

    switch (status) {
        case OBC::COMM::GroundLinkSendStatus::OK:
            return Drv::ByteStreamStatus::OP_OK;
        case OBC::COMM::GroundLinkSendStatus::RETRY:
            this->emitError_(SEND_ERROR_CODE);
            return Drv::ByteStreamStatus::SEND_RETRY;
        case OBC::COMM::GroundLinkSendStatus::ERROR:
        default:
            this->emitError_(SEND_ERROR_CODE);
            return Drv::ByteStreamStatus::OTHER_ERROR;
    }
}

void GroundLinkDriver::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);
    this->deallocate_out(0, fwBuffer);
}

void GroundLinkDriver::workerLoop_() {
    const std::uint32_t pollTimeoutMs = readTimeoutOverrideOrDefault(
        "COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS",
        DEFAULT_POLL_TIMEOUT_MS);
    while (this->m_running.load()) {
        static_cast<void>(this->runReceiveOnce_(pollTimeoutMs));
    }
}

bool GroundLinkDriver::runReceiveOnce_(std::uint32_t timeoutMs) {
    const std::shared_ptr<OBC::COMM::IGroundLinkBackend> backend = this->getBackend_();
    if (backend == nullptr) {
        this->emitError_(DISABLED_ERROR_CODE);
        std::this_thread::sleep_for(std::chrono::milliseconds(DISABLED_BACKOFF_MS));
        return false;
    }

    std::string chunk;
    const OBC::COMM::GroundLinkReceiveStatus status = backend->receive(chunk, timeoutMs);
    const OBC::COMM::GroundLinkStats stats = backend->getStats();
    this->publishStats_(stats);
    this->updateObservationCache_(backend);

    if (status == OBC::COMM::GroundLinkReceiveStatus::IDLE) {
        return true;
    }

    if (status == OBC::COMM::GroundLinkReceiveStatus::DISCONNECTED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DISCONNECTED_BACKOFF_MS));
        return true;
    }

    if (status == OBC::COMM::GroundLinkReceiveStatus::ERROR) {
        this->emitError_(RECEIVE_ERROR_CODE);
        return false;
    }

    if (chunk.empty()) {
        return true;
    }

    Fw::Buffer buffer = this->allocate_out(0, static_cast<FwSizeType>(chunk.size()));
    if (buffer.getData() == nullptr || buffer.getSize() < chunk.size()) {
        this->emitError_(ALLOCATE_ERROR_CODE);
        if (buffer.getData() != nullptr) {
            this->deallocate_out(0, buffer);
        }
        return false;
    }

    std::memcpy(buffer.getData(), chunk.data(), chunk.size());
    buffer.setSize(static_cast<FwSizeType>(chunk.size()));
    this->recv_out(0, buffer, Drv::ByteStreamStatus::OP_OK);
    return true;
}

void GroundLinkDriver::installOwnedBackend_(std::unique_ptr<OBC::COMM::IGroundLinkBackend> backend) {
    this->stop();
    this->join();

    std::shared_ptr<OBC::COMM::IGroundLinkBackend> sharedBackend(std::move(backend));
    const OBC::COMM::GroundLinkObservationState initialObservation =
        sharedBackend != nullptr ? sharedBackend->getObservationState() : OBC::COMM::GroundLinkObservationState{};
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_backend = std::move(sharedBackend);
    this->m_connectedLatched = false;
    this->m_havePublishedStats = false;
    this->m_lastPublishedStats = {};
    this->m_lastObservation = initialObservation;
}

void GroundLinkDriver::publishStats_(const OBC::COMM::GroundLinkStats& stats) {
    this->updateConnectionState_(stats);
    if (!this->shouldPublishStats_(stats)) {
        return;
    }
    this->tlmWrite_GROUND_LINK_MODE(static_cast<U8>(stats.mode));
    this->tlmWrite_GROUND_LINK_CONNECTED(stats.connected);
    this->tlmWrite_GROUND_LINK_TX_CHUNKS(stats.txChunks);
    this->tlmWrite_GROUND_LINK_RX_CHUNKS(stats.rxChunks);
    this->tlmWrite_GROUND_LINK_TX_BYTES(stats.txBytes);
    this->tlmWrite_GROUND_LINK_RX_BYTES(stats.rxBytes);
    this->tlmWrite_GROUND_LINK_TX_ERRORS(stats.txErrors);
    this->tlmWrite_GROUND_LINK_RX_ERRORS(stats.rxErrors);
}

bool GroundLinkDriver::shouldPublishStats_(const OBC::COMM::GroundLinkStats& stats) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_havePublishedStats || !equalGroundLinkStats(this->m_lastPublishedStats, stats)) {
        this->m_havePublishedStats = true;
        this->m_lastPublishedStats = stats;
        return true;
    }
    return false;
}

void GroundLinkDriver::updateObservationCache_(const std::shared_ptr<OBC::COMM::IGroundLinkBackend>& backend) {
    const OBC::COMM::GroundLinkObservationState observation =
        backend != nullptr ? backend->getObservationState() : OBC::COMM::GroundLinkObservationState{};
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_lastObservation = observation;
}

void GroundLinkDriver::updateConnectionState_(const OBC::COMM::GroundLinkStats& stats) {
    bool emitUp = false;
    bool emitDown = false;
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (stats.connected && !this->m_connectedLatched) {
            this->m_connectedLatched = true;
            emitUp = true;
        } else if (!stats.connected && this->m_connectedLatched) {
            this->m_connectedLatched = false;
            emitDown = true;
        }
    }

    if (emitUp) {
        if (this->isConnected_ready_OutputPort(0)) {
            this->ready_out(0);
        }
        this->log_ACTIVITY_HI_GROUND_LINK_UP(static_cast<U8>(stats.mode));
        return;
    }

    if (emitDown) {
        this->log_WARNING_LO_GROUND_LINK_DOWN(static_cast<U8>(stats.mode));
    }
}

void GroundLinkDriver::emitError_(U32 code) {
    this->log_WARNING_LO_GROUND_LINK_ERROR(code);
}

std::shared_ptr<OBC::COMM::IGroundLinkBackend> GroundLinkDriver::getBackend_() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_backend;
}

}  // namespace OBC
