#include "OBC/Components/CommController/CommDownlinkScheduler.hpp"

namespace OBC {

constexpr U32 CommDownlinkScheduler::NO_ACTIVE_CONTEXT;

namespace {

bool isAvailable(OBC::CommBand band, bool sbandAvailable, bool uhfAvailable) {
    return band == OBC::CommBand::UHF ? uhfAvailable : sbandAvailable;
}

}  // namespace

void CommDownlinkScheduler::setPrimaryFileLink(OBC::CommBand band) {
    this->m_primaryFileLink = band;
}

void CommDownlinkScheduler::setLinkAvailability(OBC::CommBand band, bool available) {
    if (band == OBC::CommBand::UHF) {
        this->m_uhfAvailable = available;
    } else {
        this->m_sbandAvailable = available;
    }
}

CommDownlinkSubmitResult CommDownlinkScheduler::submit(const CommDownlinkRequest& request) {
    CommDownlinkSubmitResult result;
    if (!isAvailable(this->m_primaryFileLink, this->m_sbandAvailable, this->m_uhfAvailable)) {
        result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, 0U);
        result.reason = CommDownlinkTransitionReason::PRIMARY_LINK_UNAVAILABLE;
        return result;
    }

    CommDownlinkRequest trackedRequest = request;
    trackedRequest.requestContext = this->allocateRequestContext_();

    if (this->m_active.owner == CommDownlinkOwner::NONE) {
        this->m_active = trackedRequest;
        this->m_activeLink = this->m_primaryFileLink;
        this->m_activeLaunchContext = NO_ACTIVE_CONTEXT;
        result.launchNow = true;
        result.launchRequest = trackedRequest;
        result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, trackedRequest.requestContext);
        result.reason = CommDownlinkTransitionReason::ACCEPTED;
        return result;
    }

    result.response = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, 0U);
    result.reason = CommDownlinkTransitionReason::BUSY_ACTIVE_DP;
    return result;
}

void CommDownlinkScheduler::confirmActiveLaunch(U32 context) {
    if (this->m_active.owner == CommDownlinkOwner::NONE) {
        return;
    }
    this->m_activeLaunchContext = context;
}

CommDownlinkCompleteResult CommDownlinkScheduler::failActiveLaunch(bool notifyDpCatalog) {
    CommDownlinkCompleteResult result;
    if (this->m_active.owner == CommDownlinkOwner::NONE) {
        return result;
    }

    result.ownerCleared = true;
    result.clearedOwner = this->m_active.owner;
    result.notifyDpCatalog = notifyDpCatalog && this->m_active.owner == CommDownlinkOwner::DP_CATALOG;
    if (result.notifyDpCatalog) {
        result.hasDpCatalogResponse = true;
        result.dpCatalogResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, this->m_active.requestContext);
    }
    result.reason = CommDownlinkTransitionReason::OWNER_COMPLETED;
    this->clearActive_();
    return result;
}

CommDownlinkCompleteResult CommDownlinkScheduler::complete(const Svc::SendFileResponse& response) {
    CommDownlinkCompleteResult result;
    if (this->m_active.owner == CommDownlinkOwner::NONE) {
        return result;
    }

    if (this->m_activeLaunchContext == NO_ACTIVE_CONTEXT || response.get_context() != this->m_activeLaunchContext) {
        return result;
    }

    result.ownerCleared = true;
    result.notifyDpCatalog = this->m_active.owner == CommDownlinkOwner::DP_CATALOG;
    result.hasDpCatalogResponse = result.notifyDpCatalog;
    if (result.notifyDpCatalog) {
        result.dpCatalogResponse = Svc::SendFileResponse(response.get_status(), this->m_active.requestContext);
    }
    result.clearedOwner = this->m_active.owner;
    result.reason = CommDownlinkTransitionReason::OWNER_COMPLETED;
    this->clearActive_();

    if (this->m_havePending && isAvailable(this->m_primaryFileLink, this->m_sbandAvailable, this->m_uhfAvailable)) {
        this->m_active = this->m_pending;
        this->m_activeLink = this->m_primaryFileLink;
        this->m_havePending = false;
        this->m_pending = {};
        result.launchPending = true;
        result.launchRequest = this->m_active;
    }

    return result;
}

CommDownlinkCompleteResult CommDownlinkScheduler::dropForPrimarySwitch() {
    CommDownlinkCompleteResult result;
    if (this->m_active.owner != CommDownlinkOwner::NONE) {
        result.ownerCleared = true;
        result.clearedOwner = this->m_active.owner;
        result.notifyDpCatalog = this->m_active.owner == CommDownlinkOwner::DP_CATALOG;
        result.hasDpCatalogResponse = result.notifyDpCatalog;
        if (result.notifyDpCatalog) {
            result.dpCatalogResponse =
                Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, this->m_active.requestContext);
        }
        result.reason = CommDownlinkTransitionReason::OWNER_DROPPED_PRIMARY_SWITCH;
    }
    if (!result.notifyDpCatalog && this->m_havePending && this->m_pending.owner == CommDownlinkOwner::DP_CATALOG) {
        result.notifyDpCatalog = true;
        result.hasDpCatalogResponse = true;
        result.dpCatalogResponse =
            Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, this->m_pending.requestContext);
    }
    this->clearActive_();
    this->m_havePending = false;
    this->m_pending = {};
    return result;
}

CommDownlinkCompleteResult CommDownlinkScheduler::dropForLinkLoss(OBC::CommBand band) {
    CommDownlinkCompleteResult result;
    if (this->m_active.owner != CommDownlinkOwner::NONE && this->m_activeLink == band) {
        result.ownerCleared = true;
        result.clearedOwner = this->m_active.owner;
        result.notifyDpCatalog = this->m_active.owner == CommDownlinkOwner::DP_CATALOG;
        result.hasDpCatalogResponse = result.notifyDpCatalog;
        if (result.notifyDpCatalog) {
            result.dpCatalogResponse =
                Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, this->m_active.requestContext);
        }
        result.reason = CommDownlinkTransitionReason::OWNER_DROPPED_LINK_LOSS;
        this->clearActive_();
    }
    if (this->m_havePending && this->m_primaryFileLink == band) {
        if (this->m_pending.owner == CommDownlinkOwner::DP_CATALOG) {
            result.notifyDpCatalog = true;
            result.hasDpCatalogResponse = true;
            result.dpCatalogResponse =
                Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, this->m_pending.requestContext);
        }
        this->m_havePending = false;
        this->m_pending = {};
    }
    return result;
}

CommDownlinkOwner CommDownlinkScheduler::getActiveOwner() const {
    return this->m_active.owner;
}

CommDownlinkOwner CommDownlinkScheduler::getPendingOwner() const {
    return this->m_havePending ? this->m_pending.owner : CommDownlinkOwner::NONE;
}

OBC::CommBand CommDownlinkScheduler::getActiveLink() const {
    return this->m_activeLink;
}

U32 CommDownlinkScheduler::allocateRequestContext_() {
    const U32 context = this->m_nextRequestContext;
    this->m_nextRequestContext++;
    if (this->m_nextRequestContext == 0U) {
        this->m_nextRequestContext = 1U;
    }
    return context;
}

void CommDownlinkScheduler::clearActive_() {
    this->m_active = {};
    this->m_activeLaunchContext = NO_ACTIVE_CONTEXT;
}

}  // namespace OBC
