#include "simulators/comm/CommNodeApp.hpp"
#include "simulators/comm/CommCspProtocol.hpp"

int main(int argc, char* argv[]) {
    const OBC::COMM::CommNodeAppDefaults defaults = {
        "comm_csp_node",
        "generic",
        OBC::COMM::CSP::DEFAULT_COMM_NODE_ID,
        "COMMCSP",
    };
    return OBC::COMM::runCommNodeApp(argc, argv, defaults);
}
