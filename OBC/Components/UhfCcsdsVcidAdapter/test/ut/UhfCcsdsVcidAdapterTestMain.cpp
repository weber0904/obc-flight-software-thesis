#include "UhfCcsdsVcidAdapterTester.hpp"

#include <gtest/gtest.h>

namespace {

TEST(UhfCcsdsVcidAdapter, DataPathStampsUhfVcid) {
    OBC::UhfCcsdsVcidAdapterTester tester;
    tester.testDataPathStampsUhfVcid();
}

TEST(UhfCcsdsVcidAdapter, ReturnPathPreservesContext) {
    OBC::UhfCcsdsVcidAdapterTester tester;
    tester.testReturnPathPreservesContext();
}

TEST(UhfCcsdsVcidAdapter, StatusPathPassesThrough) {
    OBC::UhfCcsdsVcidAdapterTester tester;
    tester.testStatusPathPassesThrough();
}

}  // namespace
