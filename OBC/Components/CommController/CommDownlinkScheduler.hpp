#ifndef OBC_Components_CommDownlinkScheduler_HPP
#define OBC_Components_CommDownlinkScheduler_HPP

#include "Fw/Types/FileNameString.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"
#include "Svc/FileDownlinkPorts/SendFileResponseSerializableAc.hpp"

namespace OBC {

enum class CommDownlinkOwner : U8 {
    NONE = 0,
    DP_CATALOG = 1,
};

enum class CommDownlinkTransitionReason : U8 {
    NONE = 0,
    ACCEPTED = 1,
    BUSY_ACTIVE_DP = 2,
    BUSY_PENDING_DP = 3,
    PRIMARY_LINK_UNAVAILABLE = 4,
    OWNER_COMPLETED = 5,
    OWNER_DROPPED_LINK_LOSS = 6,
    OWNER_DROPPED_PRIMARY_SWITCH = 7,
};

struct CommDownlinkRequest {
    CommDownlinkOwner owner = CommDownlinkOwner::NONE;
    Fw::FileNameString sourceFileName;
    Fw::FileNameString destFileName;
    U32 offset = 0U;
    U32 length = 0U;
    U32 requestContext = 0U;
};

struct CommDownlinkSubmitResult {
    Svc::SendFileResponse response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, 0U);
    bool launchNow = false;
    CommDownlinkRequest launchRequest = {};
    CommDownlinkTransitionReason reason = CommDownlinkTransitionReason::NONE;
};

struct CommDownlinkCompleteResult {
    bool ownerCleared = false;
    bool notifyDpCatalog = false;
    bool hasDpCatalogResponse = false;
    bool launchPending = false;
    CommDownlinkRequest launchRequest = {};
    Svc::SendFileResponse dpCatalogResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, 0U);
    CommDownlinkOwner clearedOwner = CommDownlinkOwner::NONE;
    CommDownlinkTransitionReason reason = CommDownlinkTransitionReason::NONE;
};

class CommDownlinkScheduler final {
  public:
    void setPrimaryFileLink(OBC::CommBand band);

    void setLinkAvailability(OBC::CommBand band, bool available);

    CommDownlinkSubmitResult submit(const CommDownlinkRequest& request);

    void confirmActiveLaunch(U32 context);

    CommDownlinkCompleteResult failActiveLaunch(bool notifyDpCatalog = true);

    CommDownlinkCompleteResult complete(const Svc::SendFileResponse& response);

    CommDownlinkCompleteResult dropForPrimarySwitch();

    CommDownlinkCompleteResult dropForLinkLoss(OBC::CommBand band);

    CommDownlinkOwner getActiveOwner() const;

    CommDownlinkOwner getPendingOwner() const;

    OBC::CommBand getActiveLink() const;

  private:
    U32 allocateRequestContext_();

    void clearActive_();

  private:
    static constexpr U32 NO_ACTIVE_CONTEXT = 0xFFFFFFFFU;

    OBC::CommBand m_primaryFileLink = OBC::CommBand::SBAND;
    bool m_sbandAvailable = false;
    bool m_uhfAvailable = false;
    CommDownlinkRequest m_active = {};
    OBC::CommBand m_activeLink = OBC::CommBand::SBAND;
    U32 m_activeLaunchContext = NO_ACTIVE_CONTEXT;
    bool m_havePending = false;
    CommDownlinkRequest m_pending = {};
    U32 m_nextRequestContext = 1U;
};

}  // namespace OBC

#endif
