#ifndef OBCCOMCCSDS_UHFSUBTOPOLOGY_DEFS_HPP
#define OBCCOMCCSDS_UHFSUBTOPOLOGY_DEFS_HPP

#include <Fw/Types/MallocAllocator.hpp>
#include <Svc/BufferManager/BufferManager.hpp>
#include <Svc/FrameAccumulator/FrameDetector/CcsdsTcFrameDetector.hpp>

#include "OBC/TopCcsds/OBCComCcsdsConfig/OBCComCcsdsSubtopologyConfig.hpp"
#include "OBC/TopCcsds/FppConstantsAc.hpp"
#include "OBC/TopCcsds/UhfPorts_ComBufferQueueEnumAc.hpp"
#include "OBC/TopCcsds/UhfPorts_ComPacketQueueEnumAc.hpp"

namespace OBCComCcsds {

struct UhfSubtopologyState {
    // Empty - no external state needed for the local UHF CCSDS subtopology.
};

struct TopologyState {
    UhfSubtopologyState uhfCcsds;
};

}  // namespace OBCComCcsds

#endif
