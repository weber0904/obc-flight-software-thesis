#include "CspRuntimeOwnerTester.hpp"

TEST(CspRuntimeOwner, SyncRequestReplyCopiesReplyBuffer) {
    OBC::CspRuntimeOwnerTester tester;
    tester.testSyncRequestReplyCopiesReplyBuffer();
}

TEST(CspRuntimeOwner, AsyncPingCompletionReturnsWorkerResult) {
    OBC::CspRuntimeOwnerTester tester;
    tester.testAsyncPingCompletionReturnsWorkerResult();
}

TEST(CspRuntimeOwner, AsyncRequestReplyCompletionReturnsReply) {
    OBC::CspRuntimeOwnerTester tester;
    tester.testAsyncRequestReplyCompletionReturnsReply();
}

TEST(CspRuntimeOwner, TimeoutPublishesEventAndTelemetry) {
    OBC::CspRuntimeOwnerTester tester;
    tester.testTimeoutPublishesEventAndTelemetry();
}

TEST(CspRuntimeOwner, StopWorkerRejectsNewRequests) {
    OBC::CspRuntimeOwnerTester tester;
    tester.testStopWorkerRejectsNewRequests();
}
