#ifndef OBC_TOPCCSDS_OBCAPPTOPOLOGY_HPP
#define OBC_TOPCCSDS_OBCAPPTOPOLOGY_HPP

#include "Fw/Time/TimeInterval.hpp"
#include "OBC/TopCcsds/AppTopologyDefs.hpp"

namespace OBCApp {

bool setupTopology(const TopologyState& state);

void startRateGroups(const Fw::TimeInterval& interval);

void stopRateGroups();

bool startPayloadCspService();

void stopPayloadCspService();

void teardownTopology(const TopologyState& state);

}

#endif
