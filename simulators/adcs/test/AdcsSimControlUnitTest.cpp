#include "simulators/adcs/AdcsSimServer.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        return false;
    }
    return true;
}

bool sendSocketCommand(const std::string& socketPath, const std::string& command, std::string& response) {
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        response = "socket() failed";
        return false;
    }

    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath.c_str());
    if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        response = "connect() failed";
        ::close(fd);
        return false;
    }

    const std::string payload = command + "\n";
    if (::write(fd, payload.data(), payload.size()) < 0) {
        response = "write() failed";
        ::close(fd);
        return false;
    }

    char buffer[256] = {};
    const ssize_t bytesRead = ::read(fd, buffer, sizeof(buffer) - 1);
    ::close(fd);
    if (bytesRead <= 0) {
        response = "read() failed";
        return false;
    }

    response.assign(buffer, static_cast<std::size_t>(bytesRead));
    while (!response.empty() && (response.back() == '\n' || response.back() == '\r')) {
        response.pop_back();
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;
    OBC::ADCS::AdcsSimServer server;
    std::string response;

    ok = check(server.applyControlCommandForTest("restart-pointing-pass", response),
               "restart-pointing-pass command should succeed") &&
         ok;
    ok = check(response == "OK pointing_pass_restarted=1", "restart-pointing-pass should return explicit OK response") &&
         ok;

    ok = check(server.applyControlCommandForTest("drop-state 6", response), "drop-state command should succeed") && ok;
    ok = check(response == "OK dropped_state_remaining=6", "drop-state response should report remaining count") && ok;
    ok = check(server.droppedStateReplyCountForTest() == 6U, "drop-state should update remaining counter") && ok;

    ok = check(server.applyControlCommandForTest("drop-state 0", response), "drop-state zero should clear the counter") && ok;
    ok = check(server.droppedStateReplyCountForTest() == 0U, "drop-state zero should clear remaining counter") && ok;

    ok = check(!server.applyControlCommandForTest("drop-state nope", response), "invalid count should be rejected") && ok;
    ok = check(response == "ERROR invalid-count", "invalid count response should be explicit") && ok;
    ok = check(server.droppedStateReplyCountForTest() == 0U, "invalid count should not perturb remaining counter") && ok;
    ok = check(!server.applyControlCommandForTest("drop-state -1", response), "negative count should be rejected") && ok;
    ok = check(response == "ERROR invalid-count", "negative count response should be explicit") && ok;
    ok = check(server.droppedStateReplyCountForTest() == 0U, "negative count should not perturb remaining counter") && ok;

    char tempDirTemplate[] = "/tmp/adcs-sim-control-unit.XXXXXX";
    char* tempDir = ::mkdtemp(tempDirTemplate);
    ok = check(tempDir != nullptr, "mkdtemp should succeed for reconnect test") && ok;
    if (tempDir != nullptr) {
        const std::string socketPath = std::string(tempDir) + "/adcs-control.sock";
        OBC::ADCS::AdcsSimServer liveServer(203U);
        liveServer.configureControlSocket(socketPath);
        ok = check(liveServer.start(), "live server should start with control socket enabled") && ok;

        std::string liveResponse;
        ok = check(sendSocketCommand(socketPath, "drop-state 4", liveResponse), "first control-socket client should connect") &&
             ok;
        ok = check(liveResponse == "OK dropped_state_remaining=4", "first control-socket response should be OK") && ok;
        ok = check(liveServer.droppedStateReplyCountForTest() == 4U, "first control-socket command should update counter") &&
             ok;

        ok = check(sendSocketCommand(socketPath, "restart-pointing-pass", liveResponse),
                   "restart-pointing-pass control-socket client should connect") &&
             ok;
        ok = check(liveResponse == "OK pointing_pass_restarted=1",
                   "restart-pointing-pass control-socket response should be OK") &&
             ok;

        ok = check(sendSocketCommand(socketPath, "drop-state 2", liveResponse),
                   "second control-socket client should reconnect cleanly") &&
             ok;
        ok = check(liveResponse == "OK dropped_state_remaining=2", "second control-socket response should be OK") && ok;
        ok = check(liveServer.droppedStateReplyCountForTest() == 2U,
                   "second control-socket command should update counter after reconnect") &&
             ok;

        liveServer.stop();
        ::unlink(socketPath.c_str());
        ::rmdir(tempDir);
    }

    return ok ? 0 : 1;
}
