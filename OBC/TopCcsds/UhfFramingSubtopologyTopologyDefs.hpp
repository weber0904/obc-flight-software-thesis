#ifndef OBCCOMCCSDS_UHFFRAMINGSUBTOPOLOGY_DEFS_HPP
#define OBCCOMCCSDS_UHFFRAMINGSUBTOPOLOGY_DEFS_HPP

#include <Fw/Types/MallocAllocator.hpp>
#include <Svc/BufferManager/BufferManager.hpp>
#include <Svc/FrameAccumulator/FrameDetector/CcsdsTcFrameDetector.hpp>

#include "OBC/TopCcsds/OBCComCcsdsConfig/OBCComCcsdsSubtopologyConfig.hpp"
#include "OBC/TopCcsds/FppConstantsAc.hpp"
#include "OBC/TopCcsds/UhfPorts_ComBufferQueueEnumAc.hpp"
#include "OBC/TopCcsds/UhfPorts_ComPacketQueueEnumAc.hpp"

namespace OBCComCcsds {

struct UhfFramingSubtopologyState {
    // Empty - no external state needed for the local UHF CCSDS framing subtopology.
};

struct TopologyState {
    UhfFramingSubtopologyState uhfCcsds;
};

}  // namespace OBCComCcsds

#endif
