#include "CommandIngressMuxTester.hpp"

TEST(CommandIngressMux, MergesBothIngressPorts) {
    OBC::CommandIngressMuxTester tester;
    tester.testMergesBothIngressPorts();
}

TEST(CommandIngressMux, RoutesStatusesBackToRememberedIngress) {
    OBC::CommandIngressMuxTester tester;
    tester.testRoutesStatusesBackToRememberedIngress();
}

TEST(CommandIngressMux, DuplicateOriginalContextsUseMuxDispatchContext) {
    OBC::CommandIngressMuxTester tester;
    tester.testDuplicateOriginalContextsUseMuxDispatchContext();
}

TEST(CommandIngressMux, FullTableFailsClosedWithoutOverwritingExistingMappings) {
    OBC::CommandIngressMuxTester tester;
    tester.testFullTableFailsClosedWithoutOverwritingExistingMappings();
}
