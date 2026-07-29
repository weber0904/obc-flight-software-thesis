#include "SecureLinkAuthorizerTester.hpp"

#include <gtest/gtest.h>

TEST(SecureLinkAuthorizer, ReqAuthProducesChallengeOnConfiguredIngress) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testReqAuthProducesChallengeOnConfiguredIngress();
}

TEST(SecureLinkAuthorizer, MatchingResponseProducesAuthGrantAndAuthenticatedStatus) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testMatchingResponseProducesAuthGrantAndAuthenticatedStatus();
}

TEST(SecureLinkAuthorizer, BadResponseReturnsNotAuthenticatedWithoutGrant) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testBadResponseReturnsNotAuthenticatedWithoutGrant();
}

TEST(SecureLinkAuthorizer, UnsupportedServiceReturnsStatusWithoutGrant) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testUnsupportedServiceReturnsStatusWithoutGrant();
}

TEST(SecureLinkAuthorizer, MalformedPacketRejectsWithoutStatusOrGrant) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testMalformedPacketRejectsWithoutStatusOrGrant();
}

TEST(SecureLinkAuthorizer, TimeoutRevokesActiveAuth) {
    OBC::SecureLinkAuthorizerTester tester;
    tester.testTimeoutRevokesActiveAuth();
}
