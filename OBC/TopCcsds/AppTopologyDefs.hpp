#ifndef OBC_TOPCCSDS_APPTOPOLOGYDEFS_HPP
#define OBC_TOPCCSDS_APPTOPOLOGYDEFS_HPP

#include "Svc/Health/Health.hpp"
#include "Svc/Subtopologies/CdhCore/PingEntries.hpp"
#include "Svc/Subtopologies/CdhCore/SubtopologyTopologyDefs.hpp"
#include "Svc/Subtopologies/ComCcsds/Ports_ComBufferQueueEnumAc.hpp"
#include "Svc/Subtopologies/ComCcsds/Ports_ComPacketQueueEnumAc.hpp"
#include "Svc/Subtopologies/ComCcsds/PingEntries.hpp"
#include "Svc/Subtopologies/ComCcsds/SubtopologyTopologyDefs.hpp"
#include "OBC/TopCcsds/UhfPorts_ComBufferQueueEnumAc.hpp"
#include "OBC/TopCcsds/UhfPorts_ComPacketQueueEnumAc.hpp"
#include "OBC/TopCcsds/UhfSubtopologyTopologyDefs.hpp"
#include "Svc/Subtopologies/FileHandling/SubtopologyTopologyDefs.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/CommController/CommControllerRuntime.hpp"
#include "OBC/Components/GroundLinkDriver/GroundLinkObservationRuntime.hpp"

namespace OBCApp {

struct TopologyState {
    OBC::COMM::GroundLinkBackendMode groundLinkMode;
    const char* groundLinkHost;
    U16 groundLinkPort;
    U16 commCspNode;
    OBC::COMM::GroundLinkHealthSemantics groundLinkHealthSemantics;
    bool enablePrimaryGroundLinkDriver;
    U16 sbandSubsystemHealthNode;
    U16 uhfSubsystemHealthNode;
    OBC::CommSubsystemFdirConfig commSubsystemFdirConfig;
    OBC::CommBand initialCommBand;
    const char* runtimeRoot;
    bool hardwareWatchdogEnabled;
    const char* hardwareWatchdogDevice;
    U32 hardwareWatchdogTimeoutSec;
    bool diagnosticQuietPacketEgress;
    bool beaconBroadcastEnabled;
    bool uhfBeaconSideChannelEnabled;
    U16 uhfBeaconCspNode;
    const char* commandAuthModuleSerial;
    const U8* sbandSecureAuthKeyBytes;
    FwSizeType sbandSecureAuthKeyLength;
    const U8* uhfSecureAuthKeyBytes;
    FwSizeType uhfSecureAuthKeyLength;
    OBC::AuthorityConfig sbandCommandAuthorityConfig;
    OBC::AuthorityConfig uhfCommandAuthorityConfig;
    CdhCore::SubtopologyState cdhCore;
    ComCcsds::SubtopologyState comCcsds;
    OBCComCcsds::UhfSubtopologyState uhfCcsds;
    FileHandling::SubtopologyState fileHandling;
};

namespace ConfigObjects {
namespace CdhCore_health {
static Svc::Health::PingEntry pingEntries[] = {
    {PingEntries::CdhCore_cmdDisp::WARN, PingEntries::CdhCore_cmdDisp::FATAL, Fw::String("CdhCore.cmdDisp")},
    {PingEntries::CdhCore_events::WARN, PingEntries::CdhCore_events::FATAL, Fw::String("CdhCore.events")},
    {PingEntries::CdhCore_tlmSend::WARN, PingEntries::CdhCore_tlmSend::FATAL, Fw::String("CdhCore.tlmSend")},
};
}
}  // namespace ConfigObjects

namespace PingEntries = ::PingEntries;

}

#endif
