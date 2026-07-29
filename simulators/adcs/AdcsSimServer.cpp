#include "simulators/adcs/AdcsSimServer.hpp"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>

extern "C" {
#include <csp/csp.h>
#include <csp/csp_buffer.h>
}

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace OBC {
namespace ADCS {

namespace {

constexpr auto CONTROL_CLIENT_READ_TIMEOUT = std::chrono::milliseconds(250);
constexpr auto CONTROL_CLIENT_IDLE_TIMEOUT = std::chrono::seconds(5);

bool parseUnsignedCount(const std::string& token, std::uint32_t& count) {
    if (token.empty() || token.front() == '-') {
        return false;
    }
    std::istringstream stream(token);
    stream.imbue(std::locale::classic());
    unsigned long parsed = 0UL;
    stream >> parsed;
    if (stream.fail() || !stream.eof() || parsed > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    count = static_cast<std::uint32_t>(parsed);
    return true;
}

bool writeAll(int fd, const std::string& text) {
    const char* cursor = text.data();
    std::size_t remaining = text.size();
    while (remaining > 0U) {
        const ssize_t written = ::write(fd, cursor, remaining);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (written == 0) {
            return false;
        }
        cursor += written;
        remaining -= static_cast<std::size_t>(written);
    }
    return true;
}

}  // namespace

AdcsSimServer::AdcsSimServer(std::uint16_t nodeId)
    : m_nodeId(nodeId),
      m_running(false),
      m_modelMutex(),
      m_runtime(),
      m_model(),
      m_controlSocketPath(),
      m_controlListenFd(-1),
      m_controlThread() {}

AdcsSimServer::~AdcsSimServer() {
    this->stop();
}

void AdcsSimServer::configureControlSocket(const std::string& socketPath) {
    this->m_controlSocketPath = socketPath;
}

bool AdcsSimServer::applyControlCommandForTest(const std::string& command, std::string& response) {
    return this->applyControlCommand_(command, response);
}

StateData AdcsSimServer::getResolvedStateForTest() {
    std::lock_guard<std::mutex> lock(this->m_modelMutex);
    return this->m_model.getState();
}

void AdcsSimServer::advanceForTest(std::chrono::milliseconds delta) {
    std::lock_guard<std::mutex> lock(this->m_modelMutex);
    this->m_model.advanceForTest(delta);
}

std::uint32_t AdcsSimServer::droppedStateReplyCountForTest() {
    std::lock_guard<std::mutex> lock(this->m_modelMutex);
    return this->m_model.getDroppedStateReplyCount();
}

bool AdcsSimServer::start() {
    if (this->m_running.load()) {
        return true;
    }

    ::OBC::CSP::RuntimeConfig config = ::OBC::CSP::runtimeConfigFromEnvironment(this->m_nodeId, "ADCSCSP");
    if (this->m_runtime.init(config) != ::OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize ADCS CSP node " << this->m_nodeId << std::endl;
        return false;
    }

    if (!this->startControlSocket_()) {
        this->m_runtime.shutdown();
        return false;
    }

    this->m_running.store(true);
    return true;
}

void AdcsSimServer::run() {
    if (!this->start()) {
        return;
    }

    csp_socket_t socket = {};
    csp_bind(&socket, CSP_ANY);
    csp_listen(&socket, 8);

    while (this->m_running.load()) {
        csp_conn_t* const conn = csp_accept(&socket, 100);
        if (conn == nullptr) {
            continue;
        }

        this->handleConnection_(conn);
        csp_close(conn);
    }

    csp_socket_close(&socket);
}

void AdcsSimServer::stop() {
    this->m_running.store(false);
    if (this->m_controlListenFd >= 0) {
        ::close(this->m_controlListenFd);
        this->m_controlListenFd = -1;
    }
    if (this->m_controlThread.joinable()) {
        this->m_controlThread.join();
    }
    this->cleanupControlSocket_();
    this->m_runtime.shutdown();
}

void AdcsSimServer::handleConnection_(csp_conn_t* conn) {
    while (this->m_running.load()) {
        csp_packet_t* const packet = csp_read(conn, 50);
        if (packet == nullptr) {
            break;
        }

        const int destinationPort = packet->id.dport;
        if (destinationPort < static_cast<int>(CSP::ServicePort::STATE) ||
            destinationPort > static_cast<int>(CSP::ServicePort::RESET)) {
            csp_service_handler(packet);
            continue;
        }

        if (packet->length != sizeof(CSP::Request)) {
            csp_buffer_free(packet);
            continue;
        }

        CSP::Request request = {};
        std::memcpy(&request, packet->data, sizeof(request));
        csp_buffer_free(packet);

        const CSP::ServicePort service = static_cast<CSP::ServicePort>(request.header.service);
        CSP::Reply reply = {};
        {
            std::lock_guard<std::mutex> lock(this->m_modelMutex);
            if (this->m_model.consumeDroppedStateReply(service)) {
                continue;
            }
            this->m_model.processCspRequest(request, reply);
        }

        csp_packet_t* const replyPacket = csp_buffer_get(0);
        if (replyPacket == nullptr) {
            break;
        }

        replyPacket->length = sizeof(reply);
        std::memcpy(replyPacket->data, &reply, sizeof(reply));
        csp_send(conn, replyPacket);
    }
}

bool AdcsSimServer::startControlSocket_() {
    if (this->m_controlSocketPath.empty()) {
        return true;
    }

    if (this->m_controlSocketPath.size() >= sizeof(sockaddr_un::sun_path)) {
        std::cerr << "ADCS control socket path too long: " << this->m_controlSocketPath << std::endl;
        return false;
    }

    this->cleanupControlSocket_();
    const int listenFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::perror("socket(AF_UNIX)");
        return false;
    }

    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", this->m_controlSocketPath.c_str());
    ::unlink(address.sun_path);
    if (::bind(listenFd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        std::perror("bind(AF_UNIX)");
        ::close(listenFd);
        return false;
    }
    if (::listen(listenFd, 4) != 0) {
        std::perror("listen(AF_UNIX)");
        ::close(listenFd);
        ::unlink(address.sun_path);
        return false;
    }

    const int flags = ::fcntl(listenFd, F_GETFL, 0);
    if (flags >= 0) {
        static_cast<void>(::fcntl(listenFd, F_SETFL, flags | O_NONBLOCK));
    }

    this->m_controlListenFd = listenFd;
    this->m_controlThread = std::thread([this]() { this->runControlSocket_(); });
    return true;
}

void AdcsSimServer::runControlSocket_() {
    while (this->m_running.load() || this->m_controlListenFd >= 0) {
        const int listenFd = this->m_controlListenFd;
        if (listenFd < 0) {
            break;
        }

        int clientFd = ::accept(listenFd, nullptr, nullptr);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (errno == EBADF || errno == EINVAL) {
                break;
            }
            continue;
        }

        timeval clientTimeout = {};
        clientTimeout.tv_sec =
            static_cast<decltype(clientTimeout.tv_sec)>(CONTROL_CLIENT_READ_TIMEOUT.count() / 1000);
        clientTimeout.tv_usec = static_cast<decltype(clientTimeout.tv_usec)>(
            (CONTROL_CLIENT_READ_TIMEOUT.count() % 1000) * 1000);
        static_cast<void>(::setsockopt(
            clientFd, SOL_SOCKET, SO_RCVTIMEO, &clientTimeout, static_cast<socklen_t>(sizeof(clientTimeout))));

        std::string command;
        char buffer[256] = {};
        const auto idleDeadline = std::chrono::steady_clock::now() + CONTROL_CLIENT_IDLE_TIMEOUT;
        while (true) {
            const ssize_t bytesRead = ::read(clientFd, buffer, sizeof(buffer));
            if (bytesRead < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (!this->m_running.load() || std::chrono::steady_clock::now() >= idleDeadline) {
                        break;
                    }
                    continue;
                }
                break;
            }
            if (bytesRead <= 0) {
                break;
            }
            command.append(buffer, static_cast<std::size_t>(bytesRead));
            if (command.find('\n') != std::string::npos) {
                break;
            }
        }

        if (!command.empty()) {
            const std::size_t newline = command.find('\n');
            if (newline != std::string::npos) {
                command.resize(newline);
            }
            std::string response;
            static_cast<void>(this->applyControlCommand_(command, response));
            response.push_back('\n');
            static_cast<void>(writeAll(clientFd, response));
        }
        ::close(clientFd);
    }
}

bool AdcsSimServer::applyControlCommand_(const std::string& command, std::string& response) {
    std::istringstream stream(command);
    std::string verb;
    stream >> verb;
    if (verb == "restart-pointing-pass") {
        std::lock_guard<std::mutex> lock(this->m_modelMutex);
        this->m_model.restartPointingPassForRuntime();
        response = "OK pointing_pass_restarted=1";
        return true;
    }

    if (verb != "drop-state") {
        response = "ERROR unsupported-command";
        return false;
    }

    std::string countToken;
    if (!(stream >> countToken)) {
        response = "ERROR missing-count";
        return false;
    }

    std::uint32_t count = 0U;
    if (!parseUnsignedCount(countToken, count)) {
        response = "ERROR invalid-count";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_modelMutex);
        this->m_model.setDroppedStateReplyCount(count);
        response = "OK dropped_state_remaining=" + std::to_string(this->m_model.getDroppedStateReplyCount());
    }
    return true;
}

void AdcsSimServer::cleanupControlSocket_() {
    if (!this->m_controlSocketPath.empty()) {
        ::unlink(this->m_controlSocketPath.c_str());
    }
}

}  // namespace ADCS
}  // namespace OBC
