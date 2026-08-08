#include "simulators/comm/CommNodeApp.hpp"
#include "simulators/comm/CommCspProtocol.hpp"

int main(int argc, char* argv[]) {
    const OBC::COMM::CommNodeAppDefaults defaults = {
        "sband_comm_csp_node",
        "sband",
        OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
        "SBANDCSP",
    };
    return OBC::COMM::runCommNodeApp(argc, argv, defaults);
}
