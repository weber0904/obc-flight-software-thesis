#ifndef OBC_TOPCCSDS_PAYLOADCSPSERVICE_HPP
#define OBC_TOPCCSDS_PAYLOADCSPSERVICE_HPP

#include <atomic>
#include <array>
#include <thread>

#include "OBC/Components/PayloadOpsController/PayloadOpsController.hpp"

struct csp_socket_s;
struct csp_packet_s;
typedef struct csp_socket_s csp_socket_t;
typedef struct csp_packet_s csp_packet_t;

namespace OBCApp {

class PayloadCspService final {
  public:
    PayloadCspService();
    ~PayloadCspService();

    void configure(OBC::PayloadOpsController* controller);
    bool start();
    void stop();

  private:
    bool bindSockets_();
    void run_();
    void handlePacket_(csp_packet_t* packet);

    OBC::PayloadOpsController* m_controller;
    std::atomic<bool> m_running;
    std::thread m_worker;
    std::array<csp_socket_t*, 3> m_boundSockets;
};

}  // namespace OBCApp

#endif
