#ifndef OBC_COMPONENTS_LINUXWATCHDOGSINK_LINUXWATCHDOGRUNTIME_HPP
#define OBC_COMPONENTS_LINUXWATCHDOGSINK_LINUXWATCHDOGRUNTIME_HPP

#include <string>

#include "Fw/FPrimeBasicTypes.hpp"

namespace OBC {

struct LinuxWatchdogRuntimeStatus {
    bool enabled = false;
    bool open = false;
    std::string devicePath;
    U32 timeoutSec = 0U;
    U32 feedCount = 0U;
    U32 lastFeedCode = 0U;
    U32 lastError = 0U;
};

}  // namespace OBC

#endif
