#ifndef OBC_COMPONENTS_COMMCONTROLLER_RUNTIME_HPP
#define OBC_COMPONENTS_COMMCONTROLLER_RUNTIME_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthRuntime.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"

namespace OBC {

enum class CommFdirFaultKind : U32 {
    NONE = 0U,
    PRIMARY_UNAVAILABLE = 1U,
    PRIMARY_TRANSPORT = 2U,
};

enum class CommUhfBeaconSuppressClearReason : U32 {
    NONE = 0U,
    INACTIVITY_TIMEOUT = 1U,
    SESSION_REVOKED = 2U,
    SESSION_REPLACED = 3U,
    ROLE_INVALIDATED = 4U,
};

enum class CommLiveObservabilityStateReason : U32 {
    NONE = 0U,
    ACTIVE_AUTHENTICATED_SESSION = 1U,
    INACTIVE_NO_SESSION = 2U,
    INACTIVE_NON_PRIMARY_BAND = 3U,
    INACTIVE_DIAGNOSTIC_QUIET = 4U,
};

struct CommRuntimeState {
    OBC::CommBand activeBand;
    bool passActive;
    U32 passRemainingSec;
    U32 totalPasses;
    OBC::CommBand primaryCommandLink;
    OBC::CommBand primaryTelemetryLink;
    OBC::CommBand primaryFileLink;
    bool sbandAvailable;
    bool uhfAvailable;
    U32 downlinkActiveOwner;
    U32 downlinkPendingOwner;
    U32 sessionRevokeTotal;
    U32 downlinkRejectTotal;
    bool fdirFaultLatched;
    OBC::CommFdirFaultKind fdirFaultKind;
    U32 sbandActivityAgeTicks;
    U32 uhfActivityAgeTicks;
    OBC::CommLinkAvailabilityReason sbandAvailabilityReason;
    OBC::CommLinkAvailabilityReason uhfAvailabilityReason;
    U32 consecutivePrimaryUnavailable;
    U32 consecutivePrimaryTransportGrowth;
    U32 recoveryFailoverTotal;
    U32 recoveryOwnerClearTotal;
    bool sbandLiveObservabilityActive;
    OBC::CommLiveObservabilityStateReason sbandLiveObservabilityReason;
    U32 sbandLiveObservabilityIngressPort;
    AuthorityLinkRole sbandLiveObservabilityRole;
    U32 sbandLiveObservabilitySessionId;
    U32 sbandLiveObservabilityLastAcceptedSequence;
    bool uhfPrimaryPacketQuietActive;
    bool uhfBeaconSuppressActive;
    U32 uhfBeaconSuppressIngressPort;
    AuthorityLinkRole uhfBeaconSuppressRole;
    U32 uhfBeaconSuppressSessionId;
    U32 uhfBeaconSuppressLastAcceptedSequence;
    U32 uhfBeaconSuppressRemainingTicks;
    U32 uhfBeaconSuppressTimeoutTicks;
};

class ICommSubsystemHealthProbe {
  public:
    virtual ~ICommSubsystemHealthProbe() = default;

    virtual bool probeNodeResponsiveForRuntime(U16 nodeId, U32 timeoutMs) = 0;
};

struct CommSubsystemFdirConfig {
    U32 pingTimeoutMs = 100U;
    U32 unavailableFailureThreshold = 3U;
    bool useSubsystemResponsiveness = false;
    U32 primaryProbePeriodTicks = 3U;
    U32 primaryProbeFailureThreshold = 2U;
};

}  // namespace OBC

#endif
