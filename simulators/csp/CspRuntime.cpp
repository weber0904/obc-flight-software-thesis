#include "simulators/csp/CspRuntime.hpp"

#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

extern "C" {
#include <csp/csp.h>
#if defined(OBC_CSP_HAVE_SOCKETCAN)
#include <csp/drivers/can_socketcan.h>
#endif
#include <csp/csp_iflist.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include "csp_qfifo.h"
}

#if defined(OBC_CSP_HAVE_SOCKETCAN)
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace OBC {
namespace CSP {

namespace {

constexpr std::size_t kMaxCanDeviceNameLength = 15U;

std::uint16_t parsePortEnv(const char* key, std::uint16_t fallback) {
    const char* const value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > 65535UL) {
        return fallback;
    }
    return static_cast<std::uint16_t>(parsed);
}

std::string parseStringEnv(const char* key, const char* fallback) {
    const char* const value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback == nullptr ? std::string() : std::string(fallback);
    }
    return value;
}

bool parseBoolEnv(const char* key, bool fallback) {
    const char* const value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    if (std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0 || std::strcmp(value, "TRUE") == 0) {
        return true;
    }
    if (std::strcmp(value, "0") == 0 || std::strcmp(value, "false") == 0 || std::strcmp(value, "FALSE") == 0) {
        return false;
    }
    return fallback;
}

std::string canonicalizeTransportKind(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::uint32_t aggregateInterfaceErrors(const csp_iface_t* iface) {
    if (iface == nullptr) {
        return 0U;
    }
    return iface->tx_error + iface->rx_error + iface->drop + iface->autherr + iface->frame;
}

bool isSafeCanDeviceName(const std::string& value) {
    if (value.empty() || value.length() > kMaxCanDeviceNameLength) {
        return false;
    }
    for (const char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-' || ch == '.')) {
            return false;
        }
    }
    return true;
}

#if defined(OBC_CSP_HAVE_SOCKETCAN)
bool readUnsignedIntFile(const std::string& path, unsigned int& value) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }
    stream >> value;
    return !stream.fail();
}

bool socketCanPreflight(const std::string& device, std::string& failureReason) {
    if (!isSafeCanDeviceName(device)) {
        failureReason = "unsafe or empty device name";
        return false;
    }

    const std::string sysfsRoot = "/sys/class/net/" + device;
    unsigned int typeValue = 0U;
    if (!readUnsignedIntFile(sysfsRoot + "/type", typeValue)) {
        failureReason = "device is not present under /sys/class/net";
        return false;
    }
    if (typeValue != static_cast<unsigned int>(ARPHRD_CAN)) {
        failureReason = "device exists but is not a CAN interface";
        return false;
    }

    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        failureReason = std::string("socket() failed: ") + std::strerror(errno);
        return false;
    }

    struct ifreq request = {};
    std::strncpy(request.ifr_name, device.c_str(), IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFFLAGS, &request) < 0) {
        failureReason = std::string("SIOCGIFFLAGS failed: ") + std::strerror(errno);
        close(fd);
        return false;
    }
    close(fd);

    if ((request.ifr_flags & IFF_UP) == 0) {
        failureReason = "device exists but is DOWN";
        return false;
    }

    return true;
}
#endif

class ICspInterfaceBackend {
  public:
    virtual ~ICspInterfaceBackend() = default;

    virtual RuntimeStatus validate(const RuntimeConfig& config) const = 0;
    virtual RuntimeStatus init(const RuntimeConfig& config, csp_iface_t*& iface) const = 0;
    virtual void shutdown(csp_iface_t* iface) const = 0;
};

class ZmqHubCspInterfaceBackend final : public ICspInterfaceBackend {
  public:
    RuntimeStatus validate(const RuntimeConfig& config) const override {
        if (config.nodeId == 0U || config.hubHost.empty() || config.hubSubPort == 0U || config.hubPubPort == 0U) {
            return RuntimeStatus::INVALID_ARGUMENT;
        }
        return RuntimeStatus::OK;
    }

    RuntimeStatus init(const RuntimeConfig& config, csp_iface_t*& iface) const override {
        char publishEndpoint[128] = {};
        char subscribeEndpoint[128] = {};
        if (csp_zmqhub_make_endpoint(config.hubHost.c_str(), config.hubSubPort, publishEndpoint, sizeof(publishEndpoint)) !=
            CSP_ERR_NONE) {
            return RuntimeStatus::EXECUTION_ERROR;
        }
        if (csp_zmqhub_make_endpoint(config.hubHost.c_str(), config.hubPubPort, subscribeEndpoint, sizeof(subscribeEndpoint)) !=
            CSP_ERR_NONE) {
            return RuntimeStatus::EXECUTION_ERROR;
        }

        const int initResult = csp_zmqhub_init_w_name_endpoints_rxfilter(config.interfaceName.c_str(),
                                                                         config.nodeId,
                                                                         nullptr,
                                                                         0U,
                                                                         publishEndpoint,
                                                                         subscribeEndpoint,
                                                                         0U,
                                                                         &iface);
        return (initResult == CSP_ERR_NONE && iface != nullptr) ? RuntimeStatus::OK : RuntimeStatus::EXECUTION_ERROR;
    }

    void shutdown(csp_iface_t*) const override {}
};

#if defined(OBC_CSP_HAVE_SOCKETCAN)
class SocketCanCspInterfaceBackend final : public ICspInterfaceBackend {
  public:
    RuntimeStatus validate(const RuntimeConfig& config) const override {
        if (config.nodeId == 0U || !isSafeCanDeviceName(config.canDevice)) {
            return RuntimeStatus::INVALID_ARGUMENT;
        }
        return RuntimeStatus::OK;
    }

    RuntimeStatus init(const RuntimeConfig& config, csp_iface_t*& iface) const override {
        std::string failureReason;
        if (!socketCanPreflight(config.canDevice, failureReason)) {
            std::cerr << "SocketCAN preflight failed for " << config.canDevice << ": " << failureReason << "\n";
            return RuntimeStatus::EXECUTION_ERROR;
        }

        const int initResult = csp_can_socketcan_open_and_add_interface(config.canDevice.c_str(),
                                                                         config.interfaceName.c_str(),
                                                                         config.nodeId,
                                                                         0,
                                                                         config.canPromisc,
                                                                         &iface);
        if (initResult != CSP_ERR_NONE || iface == nullptr) {
            std::cerr << "SocketCAN init failed for " << config.canDevice << " with libcsp error " << initResult
                      << "\n";
            return RuntimeStatus::EXECUTION_ERROR;
        }
        return RuntimeStatus::OK;
    }

    void shutdown(csp_iface_t* iface) const override {
        if (iface == nullptr) {
            return;
        }
        const int stopResult = csp_can_socketcan_stop(iface);
        if (stopResult != CSP_ERR_NONE) {
            std::cerr << "SocketCAN shutdown failed with libcsp error " << stopResult << "\n";
        }
    }
};
#endif

const ICspInterfaceBackend* selectBackend(const RuntimeConfig& config) {
    static const ZmqHubCspInterfaceBackend zmqHubBackend = {};
#if defined(OBC_CSP_HAVE_SOCKETCAN)
    static const SocketCanCspInterfaceBackend socketCanBackend = {};
#endif

    const std::string transport = canonicalizeTransportKind(config.transportKind);
    if (transport == "zmqhub") {
        return &zmqHubBackend;
    }
#if defined(OBC_CSP_HAVE_SOCKETCAN)
    if (transport == "socketcan") {
        return &socketCanBackend;
    }
#endif

    return nullptr;
}

}  // namespace

struct LibCspRuntime::Impl {
    mutable std::mutex mutex;
    mutable std::mutex metricsMutex;
    std::atomic<bool> stopRequested{false};
    RuntimeConfig config = {};
    csp_iface_t* iface = nullptr;
    const ICspInterfaceBackend* backend = nullptr;
    std::thread routerThread = {};
    bool initialized = false;
    std::uint32_t localErrorCount = 0U;
    RuntimeMetrics cachedMetrics = {};
};

RuntimeMetrics LibCspRuntime::snapshotMetrics_(const Impl& impl) {
    RuntimeMetrics metrics = {};
    metrics.initialized = impl.initialized;
    metrics.localNodeId = impl.config.nodeId;
    metrics.errorCount = impl.localErrorCount;
    metrics.freeBuffers = impl.initialized ? static_cast<std::uint32_t>(csp_buffer_remaining()) : 0U;

    if (impl.iface != nullptr) {
        metrics.txPackets = impl.iface->tx;
        metrics.rxPackets = impl.iface->rx;
        metrics.errorCount += aggregateInterfaceErrors(impl.iface);
    }

    return metrics;
}

void LibCspRuntime::storeCachedMetrics_(Impl& impl, const RuntimeMetrics& metrics) {
    std::lock_guard<std::mutex> metricsLock(impl.metricsMutex);
    impl.cachedMetrics = metrics;
}

RuntimeMetrics LibCspRuntime::loadCachedMetrics_(const Impl& impl) {
    std::lock_guard<std::mutex> metricsLock(impl.metricsMutex);
    return impl.cachedMetrics;
}

void LibCspRuntime::refreshCachedMetrics_(Impl& impl) {
    storeCachedMetrics_(impl, snapshotMetrics_(impl));
}

LibCspRuntime::LibCspRuntime() : m_impl(new Impl()) {}

LibCspRuntime::~LibCspRuntime() {
    this->shutdown();
    delete this->m_impl;
    this->m_impl = nullptr;
}

RuntimeStatus LibCspRuntime::init(const RuntimeConfig& config) {
    const ICspInterfaceBackend* const backend = selectBackend(config);
    if (backend == nullptr) {
        return RuntimeStatus::INVALID_ARGUMENT;
    }
    const RuntimeStatus validationStatus = backend->validate(config);
    if (validationStatus != RuntimeStatus::OK) {
        return validationStatus;
    }

    std::lock_guard<std::mutex> lock(this->m_impl->mutex);
    if (this->m_impl->initialized) {
        const bool sameConfig = this->m_impl->config.nodeId == config.nodeId &&
                                canonicalizeTransportKind(this->m_impl->config.transportKind) ==
                                    canonicalizeTransportKind(config.transportKind) &&
                                this->m_impl->config.hubHost == config.hubHost &&
                                this->m_impl->config.hubSubPort == config.hubSubPort &&
                                this->m_impl->config.hubPubPort == config.hubPubPort &&
                                this->m_impl->config.canDevice == config.canDevice &&
                                this->m_impl->config.canPromisc == config.canPromisc &&
                                this->m_impl->config.interfaceName == config.interfaceName;
        return sameConfig ? RuntimeStatus::OK : RuntimeStatus::INVALID_ARGUMENT;
    }

    csp_conf.version = 2;
    csp_init();

    csp_iface_t* iface = nullptr;
    if (backend->init(config, iface) != RuntimeStatus::OK || iface == nullptr) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    iface->is_default = 1U;
    iface->netmask = 0U;
    if (csp_rtable_set(0U, 0, iface, CSP_NO_VIA_ADDRESS) != CSP_ERR_NONE) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }
    csp_iflist_check_dfl();

    this->m_impl->stopRequested.store(false);
    this->m_impl->routerThread = std::thread([impl = this->m_impl]() {
        while (!impl->stopRequested.load()) {
            const int result = csp_route_work();
            if (result != CSP_ERR_NONE && result != CSP_ERR_TIMEDOUT && !impl->stopRequested.load()) {
                std::lock_guard<std::mutex> lock(impl->mutex);
                impl->localErrorCount++;
            }
        }
    });

    this->m_impl->iface = iface;
    this->m_impl->backend = backend;
    this->m_impl->config = config;
    this->m_impl->initialized = true;
    refreshCachedMetrics_(*this->m_impl);
    return RuntimeStatus::OK;
}

RuntimeStatus LibCspRuntime::ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) {
    success = false;
    if (targetNode == 0U) {
        return RuntimeStatus::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(this->m_impl->mutex);
    if (!this->m_impl->initialized) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    const int result = csp_ping(targetNode, timeoutMs, 8U, CSP_O_NONE);
    success = (result >= 0);
    if (!success) {
        this->m_impl->localErrorCount++;
    }
    refreshCachedMetrics_(*this->m_impl);
    return RuntimeStatus::OK;
}

RuntimeStatus LibCspRuntime::sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& data) {
    if (targetNode == 0U || data.empty() || data.size() > CSP_BUFFER_SIZE) {
        return RuntimeStatus::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(this->m_impl->mutex);
    if (!this->m_impl->initialized) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    csp_conn_t* const conn = csp_connect(CSP_PRIO_NORM, targetNode, targetPort, 0U, CSP_O_NONE);
    if (conn == nullptr) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    csp_packet_t* const packet = csp_buffer_get(0);
    if (packet == nullptr) {
        csp_close(conn);
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    const std::uint32_t txPacketsBefore = this->m_impl->iface != nullptr ? this->m_impl->iface->tx : 0U;
    const std::uint32_t errorCountBefore =
        this->m_impl->iface != nullptr ? aggregateInterfaceErrors(this->m_impl->iface) : 0U;

    packet->length = static_cast<std::uint16_t>(data.size());
    std::memcpy(packet->data, data.data(), data.size());
    csp_send(conn, packet);
    csp_close(conn);

    const bool txObserved = this->m_impl->iface != nullptr && this->m_impl->iface->tx > txPacketsBefore;
    const bool errorObserved =
        this->m_impl->iface != nullptr && aggregateInterfaceErrors(this->m_impl->iface) > errorCountBefore;
    if (!txObserved || errorObserved) {
        this->m_impl->localErrorCount++;
        refreshCachedMetrics_(*this->m_impl);
        return RuntimeStatus::EXECUTION_ERROR;
    }

    refreshCachedMetrics_(*this->m_impl);
    return RuntimeStatus::OK;
}

RuntimeStatus LibCspRuntime::requestReply(std::uint16_t targetNode,
                                          std::uint8_t targetPort,
                                          const void* requestData,
                                          std::size_t requestSize,
                                          void* replyData,
                                          std::size_t replyCapacity,
                                          std::size_t& replySize,
                                          std::uint32_t timeoutMs) {
    replySize = 0U;
    if (targetNode == 0U || requestData == nullptr || requestSize == 0U || requestSize > CSP_BUFFER_SIZE ||
        replyData == nullptr || replyCapacity == 0U || replyCapacity > CSP_BUFFER_SIZE) {
        return RuntimeStatus::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(this->m_impl->mutex);
    if (!this->m_impl->initialized) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    const int result = csp_transaction(CSP_PRIO_NORM,
                                       targetNode,
                                       targetPort,
                                       timeoutMs,
                                       requestData,
                                       static_cast<int>(requestSize),
                                       replyData,
                                       static_cast<int>(replyCapacity));
    if (result <= 0) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::TIMEOUT;
    }

    if (static_cast<std::size_t>(result) > replyCapacity) {
        this->m_impl->localErrorCount++;
        return RuntimeStatus::EXECUTION_ERROR;
    }

    replySize = static_cast<std::size_t>(result);
    refreshCachedMetrics_(*this->m_impl);
    return RuntimeStatus::OK;
}

RuntimeMetrics LibCspRuntime::metrics() const {
    std::unique_lock<std::mutex> lock(this->m_impl->mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return loadCachedMetrics_(*this->m_impl);
    }

    const RuntimeMetrics metrics = snapshotMetrics_(*this->m_impl);
    storeCachedMetrics_(*this->m_impl, metrics);
    return metrics;
}

void LibCspRuntime::shutdown() {
    if (this->m_impl == nullptr) {
        return;
    }

    bool joinRouter = false;
    {
        std::lock_guard<std::mutex> lock(this->m_impl->mutex);
        if (!this->m_impl->initialized && !this->m_impl->routerThread.joinable()) {
            return;
        }
        this->m_impl->stopRequested.store(true);
        joinRouter = this->m_impl->routerThread.joinable();
    }

    csp_qfifo_wake_up();

    if (joinRouter) {
        this->m_impl->routerThread.join();
    }

    std::lock_guard<std::mutex> lock(this->m_impl->mutex);
    if (this->m_impl->backend != nullptr) {
        this->m_impl->backend->shutdown(this->m_impl->iface);
    }
    this->m_impl->iface = nullptr;
    this->m_impl->backend = nullptr;
    this->m_impl->initialized = false;
    refreshCachedMetrics_(*this->m_impl);
}

RuntimeConfig runtimeConfigFromEnvironment(std::uint16_t nodeId, const char* interfaceName) {
    RuntimeConfig config = {};
    config.nodeId = nodeId;
    config.transportKind = canonicalizeTransportKind(parseStringEnv("CSP_TRANSPORT", "zmqhub"));
    config.hubHost = parseStringEnv("CSP_HUB_HOST", "127.0.0.1");
    config.hubSubPort = parsePortEnv("CSP_HUB_SUB_PORT", 6100U);
    config.hubPubPort = parsePortEnv("CSP_HUB_PUB_PORT", 7100U);
    config.canDevice = parseStringEnv("CSP_CAN_DEVICE", "");
    config.canPromisc = parseBoolEnv("CSP_CAN_PROMISC", false);
    config.interfaceName = parseStringEnv("CSP_INTERFACE_NAME", interfaceName);
    return config;
}

ICspRuntime& defaultRuntime() {
    static LibCspRuntime runtime;
    return runtime;
}

}  // namespace CSP
}  // namespace OBC
