#ifndef OBC_SIMULATORS_EPS_EPSSIMSERVER_HPP
#define OBC_SIMULATORS_EPS_EPSSIMSERVER_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "simulators/csp/CspRuntime.hpp"
#include "simulators/eps/EpsSimModel.hpp"

struct csp_conn_s;
typedef struct csp_conn_s csp_conn_t;

namespace OBC {
namespace EPS {

class EpsSimServer {
  public:
    explicit EpsSimServer(std::uint16_t nodeId = CSP::DEFAULT_EPS_NODE_ID);

    ~EpsSimServer();

    void applyInitialSoc(float soc);

    void configureControlSocket(const std::string& socketPath);

    bool applyControlCommandForTest(const std::string& command, std::string& response);

    StatusData getResolvedStateForTest();

    void advanceForTest(std::chrono::milliseconds delta);

    std::uint32_t droppedStatusReplyCountForTest();

    bool start();

    void run();

    void stop();

  private:
    void handleConnection_(csp_conn_t* conn);
    bool startControlSocket_();
    void runControlSocket_();
    bool applyControlCommand_(const std::string& command, std::string& response);
    void cleanupControlSocket_();

  private:
    std::uint16_t m_nodeId;
    std::atomic<bool> m_running;
    std::mutex m_modelMutex;
    ::OBC::CSP::LibCspRuntime m_runtime;
    EpsSimModel m_model;
    std::string m_controlSocketPath;
    int m_controlListenFd;
    std::thread m_controlThread;
};

}  // namespace EPS
}  // namespace OBC

#endif
