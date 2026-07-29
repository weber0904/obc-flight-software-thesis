#include <gtest/gtest.h>

#include "OBC/Components/CommController/CommDownlinkScheduler.hpp"

namespace {

OBC::CommDownlinkRequest makeRequest(OBC::CommDownlinkOwner owner, const char* fileName) {
    OBC::CommDownlinkRequest request;
    request.owner = owner;
    request.sourceFileName = fileName;
    request.destFileName = fileName;
    return request;
}

}  // namespace

TEST(CommDownlinkScheduler, PrimaryLinkUnavailableRejects) {
    OBC::CommDownlinkScheduler scheduler;
    const OBC::CommDownlinkSubmitResult result =
        scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "a.fdp"));

    EXPECT_EQ(result.response.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    EXPECT_EQ(result.reason, OBC::CommDownlinkTransitionReason::PRIMARY_LINK_UNAVAILABLE);
}

TEST(CommDownlinkScheduler, ActiveDpRejectsSecondDp) {
    OBC::CommDownlinkScheduler scheduler;
    scheduler.setLinkAvailability(OBC::CommBand::SBAND, true);

    OBC::CommDownlinkSubmitResult first = scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "dp-1.fdp"));
    ASSERT_TRUE(first.launchNow);
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::DP_CATALOG);

    OBC::CommDownlinkSubmitResult second =
        scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "dp-2.fdp"));
    EXPECT_FALSE(second.launchNow);
    EXPECT_EQ(second.response.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    EXPECT_EQ(second.reason, OBC::CommDownlinkTransitionReason::BUSY_ACTIVE_DP);
    EXPECT_EQ(scheduler.getPendingOwner(), OBC::CommDownlinkOwner::NONE);
}

TEST(CommDownlinkScheduler, CompletionBeforeLaunchConfirmationDoesNotClearOwner) {
    OBC::CommDownlinkScheduler scheduler;
    scheduler.setLinkAvailability(OBC::CommBand::SBAND, true);

    OBC::CommDownlinkSubmitResult dp = scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "dp.fdp"));
    ASSERT_TRUE(dp.launchNow);

    const OBC::CommDownlinkCompleteResult staleBeforeConfirm =
        scheduler.complete(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 77U));
    EXPECT_FALSE(staleBeforeConfirm.ownerCleared);
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::DP_CATALOG);

    scheduler.confirmActiveLaunch(55U);
    const OBC::CommDownlinkCompleteResult mismatched =
        scheduler.complete(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 77U));
    EXPECT_FALSE(mismatched.ownerCleared);
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::DP_CATALOG);

    const OBC::CommDownlinkCompleteResult matched =
        scheduler.complete(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 55U));
    EXPECT_TRUE(matched.ownerCleared);
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::NONE);
}

TEST(CommDownlinkScheduler, CompletionClearsActiveDpOwner) {
    OBC::CommDownlinkScheduler scheduler;
    scheduler.setLinkAvailability(OBC::CommBand::SBAND, true);

    OBC::CommDownlinkSubmitResult dp = scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "dp.fdp"));
    ASSERT_TRUE(dp.launchNow);
    scheduler.confirmActiveLaunch(55U);

    const OBC::CommDownlinkCompleteResult matched =
        scheduler.complete(Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 55U));
    EXPECT_TRUE(matched.ownerCleared);
    EXPECT_TRUE(matched.notifyDpCatalog);
    EXPECT_TRUE(matched.hasDpCatalogResponse);
    EXPECT_EQ(matched.dpCatalogResponse.get_status(), Svc::SendFileStatus::STATUS_OK);
    EXPECT_EQ(matched.dpCatalogResponse.get_context(), dp.response.get_context());
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::NONE);
}

TEST(CommDownlinkScheduler, LinkLossDropsActiveDp) {
    OBC::CommDownlinkScheduler scheduler;
    scheduler.setLinkAvailability(OBC::CommBand::SBAND, true);

    OBC::CommDownlinkSubmitResult dp = scheduler.submit(makeRequest(OBC::CommDownlinkOwner::DP_CATALOG, "dp.fdp"));
    ASSERT_TRUE(dp.launchNow);

    const OBC::CommDownlinkCompleteResult drop = scheduler.dropForLinkLoss(OBC::CommBand::SBAND);
    EXPECT_TRUE(drop.ownerCleared);
    EXPECT_EQ(drop.clearedOwner, OBC::CommDownlinkOwner::DP_CATALOG);
    EXPECT_TRUE(drop.notifyDpCatalog);
    EXPECT_TRUE(drop.hasDpCatalogResponse);
    EXPECT_EQ(drop.dpCatalogResponse.get_context(), dp.response.get_context());
    EXPECT_EQ(scheduler.getActiveOwner(), OBC::CommDownlinkOwner::NONE);
    EXPECT_EQ(scheduler.getPendingOwner(), OBC::CommDownlinkOwner::NONE);
}
