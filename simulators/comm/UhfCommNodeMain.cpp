#include "simulators/comm/CommNodeApp.hpp"
#include "simulators/comm/CommCspProtocol.hpp"

int main(int argc, char* argv[]) {
    const OBC::COMM::CommNodeAppDefaults defaults = {
        "uhf_comm_csp_node",
        "uhf",
        OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
        "UHFCSP",
    };
    return OBC::COMM::runCommNodeApp(argc, argv, defaults);
}
