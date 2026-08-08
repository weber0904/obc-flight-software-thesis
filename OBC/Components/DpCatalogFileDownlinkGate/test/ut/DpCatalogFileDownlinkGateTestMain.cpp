#include "DpCatalogFileDownlinkGateTester.hpp"

TEST(DpCatalogFileDownlinkGate, MatchingCompletionForwards) {
    OBC::DpCatalogFileDownlinkGateTester tester;
    tester.testMatchingCompletionForwards();
}

TEST(DpCatalogFileDownlinkGate, ForeignCompletionIgnored) {
    OBC::DpCatalogFileDownlinkGateTester tester;
    tester.testForeignCompletionIgnored();
}

TEST(DpCatalogFileDownlinkGate, BusyWhilePending) {
    OBC::DpCatalogFileDownlinkGateTester tester;
    tester.testBusyWhilePending();
}

TEST(DpCatalogFileDownlinkGate, SendFailureDoesNotLatchPending) {
    OBC::DpCatalogFileDownlinkGateTester tester;
    tester.testSendFailureDoesNotLatchPending();
}
