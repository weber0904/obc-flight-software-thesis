#include "FileIngressAuthorityTester.hpp"

TEST(FileIngressAuthority, StartRequiresAuthAndAllowedPolicy) {
    OBC::FileIngressAuthorityTester tester;
    tester.testStartRequiresAuthAndAllowedPolicy();
}

TEST(FileIngressAuthority, UhfBackupDeniedWithActiveAuth) {
    OBC::FileIngressAuthorityTester tester;
    tester.testUhfBackupDeniedWithActiveAuth();
}

TEST(FileIngressAuthority, RevocationDropsPacketsUntilNewStart) {
    OBC::FileIngressAuthorityTester tester;
    tester.testRevocationDropsPacketsUntilNewStart();
}

TEST(FileIngressAuthority, NonStagingDestinationRejected) {
    OBC::FileIngressAuthorityTester tester;
    tester.testNonStagingDestinationRejected();
}

TEST(FileIngressAuthority, SecondIngressReturnRoutesToOriginalSource) {
    OBC::FileIngressAuthorityTester tester;
    tester.testSecondIngressReturnRoutesToOriginalSource();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
