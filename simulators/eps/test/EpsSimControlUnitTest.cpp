#include "simulators/eps/EpsSimServer.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

bool approxEqual(float lhs, float rhs, float tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

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
    OBC::EPS::EpsSimServer server;
    std::string response;

    ok = check(server.applyControlCommandForTest("set-load-mode high-draw", response),
               "set-load-mode should succeed") && ok;
    ok = check(response == "OK load_mode=high-draw", "set-load-mode should return explicit OK response") && ok;
    ok = check(server.applyControlCommandForTest("set-load-mode normal", response),
               "set-load-mode normal should succeed") && ok;
    ok = check(response == "OK load_mode=normal", "set-load-mode normal should return explicit OK response") && ok;
    ok = check(!server.applyControlCommandForTest("set-load-mode nope", response),
               "invalid load mode should be rejected") && ok;
    ok = check(response == "ERROR invalid-mode", "invalid load mode response should be explicit") && ok;

    ok = check(server.applyControlCommandForTest("set-soc 55", response), "immediate set command should succeed") && ok;
    ok = check(response.rfind("OK ", 0) == 0, "immediate set should return OK response") && ok;
    ok = check(approxEqual(server.getResolvedStateForTest().soc, 55.0F, 0.05F), "immediate set should update SoC") && ok;

    ok = check(server.applyControlCommandForTest("set-soc 80 0.20", response), "timed ramp command should succeed") && ok;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const OBC::EPS::StatusData midState = server.getResolvedStateForTest();
    ok = check(midState.soc > 55.0F && midState.soc < 80.0F, "timed ramp should report an intermediate SoC") && ok;
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    ok = check(approxEqual(server.getResolvedStateForTest().soc, 80.0F, 0.10F), "timed ramp should converge to target SoC") && ok;

    const OBC::EPS::StatusData beforeInvalid = server.getResolvedStateForTest();
    ok = check(!server.applyControlCommandForTest("set-soc nope", response), "invalid value should be rejected") && ok;
    ok = check(response == "ERROR invalid-value", "invalid value response should be explicit") && ok;
    ok = check(approxEqual(server.getResolvedStateForTest().soc, beforeInvalid.soc, 0.01F),
               "invalid value should not perturb current SoC") &&
         ok;

    ok = check(server.applyControlCommandForTest("drop-status 4", response), "drop-status command should succeed") && ok;
    ok = check(response == "OK dropped_status_remaining=4", "drop-status should return remaining count") && ok;
    ok = check(server.droppedStatusReplyCountForTest() == 4U, "drop-status should update dropped reply count") && ok;
    ok = check(!server.applyControlCommandForTest("drop-status nope", response), "invalid drop-status count should fail") && ok;
    ok = check(response == "ERROR invalid-count", "invalid drop-status count should be explicit") && ok;
    ok = check(!server.applyControlCommandForTest("drop-status -1", response), "negative drop-status count should fail") && ok;
    ok = check(response == "ERROR invalid-count", "negative drop-status count should be explicit") && ok;

    char tempDirTemplate[] = "/tmp/eps-sim-control-unit.XXXXXX";
    char* tempDir = ::mkdtemp(tempDirTemplate);
    ok = check(tempDir != nullptr, "mkdtemp should succeed for reconnect test") && ok;
    if (tempDir != nullptr) {
        const std::string socketPath = std::string(tempDir) + "/eps-control.sock";
        OBC::EPS::EpsSimServer liveServer(202U);
        liveServer.configureControlSocket(socketPath);
        ok = check(liveServer.start(), "live server should start with control socket enabled") && ok;

        std::string liveResponse;
        ok = check(sendSocketCommand(socketPath, "set-soc 61", liveResponse), "first control-socket client should connect") &&
             ok;
        ok = check(liveResponse.rfind("OK ", 0) == 0, "first control-socket response should be OK") && ok;
        ok = check(approxEqual(liveServer.getResolvedStateForTest().soc, 61.0F, 0.05F),
                   "first control-socket command should update SoC") &&
             ok;

        ok = check(sendSocketCommand(socketPath, "set-soc 47", liveResponse),
                   "second control-socket client should reconnect cleanly") &&
             ok;
        ok = check(liveResponse.rfind("OK ", 0) == 0, "second control-socket response should be OK") && ok;
        ok = check(approxEqual(liveServer.getResolvedStateForTest().soc, 47.0F, 0.05F),
                   "second control-socket command should update SoC after reconnect") &&
             ok;

        ok = check(sendSocketCommand(socketPath, "drop-status 3", liveResponse),
                   "drop-status control-socket client should connect cleanly") &&
             ok;
        ok = check(liveResponse == "OK dropped_status_remaining=3",
                   "drop-status control-socket response should report remaining count") &&
             ok;
        ok = check(liveServer.droppedStatusReplyCountForTest() == 3U,
                   "drop-status control-socket command should update reply-drop count") &&
             ok;

        ok = check(sendSocketCommand(socketPath, "set-load-mode high-draw", liveResponse),
                   "set-load-mode control-socket client should connect cleanly") &&
             ok;
        ok = check(liveResponse == "OK load_mode=high-draw",
                   "set-load-mode control-socket response should report selected mode") &&
             ok;

        liveServer.stop();
        ::unlink(socketPath.c_str());
        ::rmdir(tempDir);
    }

    return ok ? 0 : 1;
}
