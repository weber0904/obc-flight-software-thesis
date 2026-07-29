#include "SequenceCallbackFanoutTester.hpp"

TEST(SequenceCallbackFanout, SeqStartFansOutToBothSinks) {
    OBC::SequenceCallbackFanoutTester tester;
    tester.testSeqStartFansOutToBothSinks();
}

TEST(SequenceCallbackFanout, SeqDoneFansOutToBothSinks) {
    OBC::SequenceCallbackFanoutTester tester;
    tester.testSeqDoneFansOutToBothSinks();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
