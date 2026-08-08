#include "PersistentFaultManagerTester.hpp"

TEST(PersistentFaultManager, HistoryCommandRequiresConfiguration) {
    OBC::PersistentFaultManagerTester tester;
    tester.testHistoryCommandRequiresConfiguration();
}

TEST(PersistentFaultManager, HistoryCommandReportsLatestFirst) {
    OBC::PersistentFaultManagerTester tester;
    tester.testHistoryCommandReportsLatestFirst();
}

TEST(PersistentFaultManager, HistoryCommandClampsLargeLimit) {
    OBC::PersistentFaultManagerTester tester;
    tester.testHistoryCommandClampsLargeLimit();
}

TEST(PersistentFaultManager, HistoryCommandReportsEmptyConfiguredSet) {
    OBC::PersistentFaultManagerTester tester;
    tester.testHistoryCommandReportsEmptyConfiguredSet();
}
