#ifndef OBC_SIMULATORS_ADCS_ADCSSIMSERVER_HPP
#define OBC_SIMULATORS_ADCS_ADCSSIMSERVER_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "simulators/adcs/AdcsSimModel.hpp"
#include "simulators/csp/CspRuntime.hpp"

typedef struct csp_conn_s csp_conn_t;

namespace OBC {
namespace ADCS {

class AdcsSimServer {
  public:
    explicit AdcsSimServer(std::uint16_t nodeId = CSP::DEFAULT_ADCS_NODE_ID);

    ~AdcsSimServer();

    void configureControlSocket(const std::string& socketPath);

    bool applyControlCommandForTest(const std::string& command, std::string& response);

    StateData getResolvedStateForTest();

    void advanceForTest(std::chrono::milliseconds delta);

    std::uint32_t droppedStateReplyCountForTest();

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
    AdcsSimModel m_model;
    std::string m_controlSocketPath;
    int m_controlListenFd;
    std::thread m_controlThread;
};

}  // namespace ADCS
}  // namespace OBC

#endif
