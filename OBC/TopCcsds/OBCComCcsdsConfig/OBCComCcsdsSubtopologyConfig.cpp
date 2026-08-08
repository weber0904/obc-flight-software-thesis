#include "OBC/TopCcsds/OBCComCcsdsConfig/OBCComCcsdsSubtopologyConfig.hpp"

namespace OBCComCcsds {
namespace Allocation {
Fw::MallocAllocator mallocatorInstance;
Fw::MemAllocator& memAllocator = mallocatorInstance;
}  // namespace Allocation
}  // namespace OBCComCcsds
