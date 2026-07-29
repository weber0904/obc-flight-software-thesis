#ifndef OBC_SIMULATORS_COMM_COMMNODEAPP_HPP
#define OBC_SIMULATORS_COMM_COMMNODEAPP_HPP

#include <cstdint>

namespace OBC {
namespace COMM {

struct CommNodeAppDefaults {
    const char* executableName;
    const char* linkIdentity;
    std::uint16_t nodeId;
    const char* interfaceName;
};

int runCommNodeApp(int argc, char* argv[], const CommNodeAppDefaults& defaults);

}  // namespace COMM
}  // namespace OBC

#endif
