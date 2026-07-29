#include "CommandIngressAuthorityTester.hpp"

TEST(CommandIngressAuthority, AllowedCommandForwardsExactlyOnce) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testAllowedCommandForwardsExactlyOnce();
}

TEST(CommandIngressAuthority, UnconfiguredPortFailsClosed) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testUnconfiguredPortFailsClosed();
}

TEST(CommandIngressAuthority, RestrictedMalformedFailsClosed) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testRestrictedMalformedFailsClosed();
}

TEST(CommandIngressAuthority, PrimaryIngressRejectsLegacyCommandWithoutEnvelope) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testPrimaryIngressRejectsLegacyCommandWithoutEnvelope();
}

TEST(CommandIngressAuthority, LegacyEnvelopeFailsClosedBeforeSessionOrSequenceMutation) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testLegacyEnvelopeFailsClosedBeforeSessionOrSequenceMutation();
}

TEST(CommandIngressAuthority, DirectOfficialSeqDispatcherRunDenied) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testDirectOfficialSeqDispatcherRunDenied();
}

TEST(CommandIngressAuthority, WrapperSequenceRunRoutesToSequenceController) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testWrapperSequenceRunRoutesToSequenceController();
}

TEST(CommandIngressAuthority, AuthGrantSynthesizesSecureSessionAndAcceptsSecureCommand) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testAuthGrantSynthesizesSecureSessionAndAcceptsSecureCommand();
}

TEST(CommandIngressAuthority, SecureCommandRejectsWithoutAuth) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testSecureCommandRejectsWithoutAuth();
}

TEST(CommandIngressAuthority, SecureCommandRejectsBadMacWithoutMutatingSequence) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testSecureCommandRejectsBadMacWithoutMutatingSequence();
}

TEST(CommandIngressAuthority, AuthRevokedClearsSecureSessionAndBlocksFurtherCommands) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testAuthRevokedClearsSecureSessionAndBlocksFurtherCommands();
}

TEST(CommandIngressAuthority, RuntimeObserverTracksSecureSessionLifecycle) {
    OBC::CommandIngressAuthorityTester tester;
    tester.testRuntimeObserverTracksSecureSessionLifecycle();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
