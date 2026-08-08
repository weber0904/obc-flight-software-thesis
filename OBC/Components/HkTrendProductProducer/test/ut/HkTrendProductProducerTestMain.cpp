#include "HkTrendProductProducerTester.hpp"

TEST(HkTrendProductProducer, ManualFlushEmitsChunkWithRawValues) {
    OBC::HkTrendProductProducerTester tester;
    tester.testManualFlushEmitsChunkWithRawValues();
}

TEST(HkTrendProductProducer, CadenceWaitsBetweenSamples) {
    OBC::HkTrendProductProducerTester tester;
    tester.testCadenceWaitsBetweenSamples();
}

TEST(HkTrendProductProducer, SizeThresholdChunkingKeepsContiguousSequences) {
    OBC::HkTrendProductProducerTester tester;
    tester.testSizeThresholdChunkingKeepsContiguousSequences();
}

TEST(HkTrendProductProducer, MissingStateUsesValidityFlags) {
    OBC::HkTrendProductProducerTester tester;
    tester.testMissingStateUsesValidityFlags();
}

TEST(HkTrendProductProducer, FlushCommandOnEmptyChunkIsNoOp) {
    OBC::HkTrendProductProducerTester tester;
    tester.testFlushCommandOnEmptyChunkIsNoOp();
}

TEST(HkTrendProductProducer, GetStatusCommandReportsPendingState) {
    OBC::HkTrendProductProducerTester tester;
    tester.testGetStatusCommandReportsPendingState();
}

TEST(HkTrendProductProducer, ThresholdChangeFlushesImmediately) {
    OBC::HkTrendProductProducerTester tester;
    tester.testThresholdChangeFlushesImmediately();
}

TEST(HkTrendProductProducer, ParameterLoadDefaultPathUses8192) {
    OBC::HkTrendProductProducerTester tester;
    tester.testParameterLoadDefaultPathUses8192();
}

TEST(HkTrendProductProducer, OutOfRangeParameterIsClampedAndPersisted) {
    OBC::HkTrendProductProducerTester tester;
    tester.testOutOfRangeParameterIsClampedAndPersisted();
}

TEST(HkTrendProductProducer, UnavailableSourceReportsError) {
    OBC::HkTrendProductProducerTester tester;
    tester.testUnavailableSourceReportsError();
}

TEST(HkTrendProductProducer, SerializeFailureReturnsDpBuffer) {
    OBC::HkTrendProductProducerTester tester;
    tester.testSerializeFailureReturnsDpBuffer();
}

TEST(HkTrendProductProducer, ThresholdFlushFailureRetainsCurrentSample) {
    OBC::HkTrendProductProducerTester tester;
    tester.testThresholdFlushFailureRetainsCurrentSample();
}

TEST(HkTrendProductProducer, RetainedBacklogIsBoundedWhenFlushKeepsFailing) {
    OBC::HkTrendProductProducerTester tester;
    tester.testRetainedBacklogIsBoundedWhenFlushKeepsFailing();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
