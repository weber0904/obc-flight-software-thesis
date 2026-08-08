#include "RateGroupTimingProbeTester.hpp"

TEST(RateGroupTimingProbe, ForwardsScheduledContexts) {
    OBC::RateGroupTimingProbeTester tester;
    tester.testForwardsScheduledContexts();
}

TEST(RateGroupTimingProbe, EmitsSlowCycleSummary) {
    OBC::RateGroupTimingProbeTester tester;
    tester.testEmitsSlowCycleSummary();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
