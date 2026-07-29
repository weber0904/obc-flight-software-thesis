#include "UhfCcsdsVcidAdapterTester.hpp"

namespace OBC {

UhfCcsdsVcidAdapterTester::UhfCcsdsVcidAdapterTester()
    : UhfCcsdsVcidAdapterGTestBase("UhfCcsdsVcidAdapterTester", MAX_HISTORY_SIZE), component("vcidAdapter") {
    this->initComponents();
    this->connectPorts();
}

UhfCcsdsVcidAdapterTester::~UhfCcsdsVcidAdapterTester() = default;

void UhfCcsdsVcidAdapterTester::testDataPathStampsUhfVcid() {
    U8 dataBytes[4] = {1U, 2U, 3U, 4U};
    Fw::Buffer buffer(dataBytes, sizeof(dataBytes));
    ComCfg::FrameContext context;
    context.set_vcId(1U);
    context.set_apid(ComCfg::Apid::FW_PACKET_TELEM);
    context.set_sequenceCount(33U);
    context.set_comQueueIndex(1U);

    this->clearHistory();
    this->invoke_to_dataIn(0, buffer, context);

    ASSERT_from_dataOut_SIZE(1);
    const auto& forwarded = this->fromPortHistory_dataOut->at(0);
    EXPECT_EQ(forwarded.data.getData(), buffer.getData());
    EXPECT_EQ(forwarded.data.getSize(), buffer.getSize());
    EXPECT_EQ(forwarded.context.get_apid(), context.get_apid());
    EXPECT_EQ(forwarded.context.get_sequenceCount(), context.get_sequenceCount());
    EXPECT_EQ(forwarded.context.get_comQueueIndex(), context.get_comQueueIndex());
    EXPECT_EQ(forwarded.context.get_vcId(), 2U);
}

void UhfCcsdsVcidAdapterTester::testReturnPathPreservesContext() {
    U8 dataBytes[2] = {9U, 8U};
    Fw::Buffer buffer(dataBytes, sizeof(dataBytes));
    ComCfg::FrameContext context;
    context.set_vcId(2U);
    context.set_apid(ComCfg::Apid::FW_PACKET_FILE);
    context.set_sequenceCount(44U);
    context.set_comQueueIndex(0U);

    this->clearHistory();
    this->invoke_to_dataReturnIn(0, buffer, context);

    ASSERT_from_dataReturnOut_SIZE(1);
    const auto& returned = this->fromPortHistory_dataReturnOut->at(0);
    EXPECT_EQ(returned.data.getData(), buffer.getData());
    EXPECT_EQ(returned.data.getSize(), buffer.getSize());
    EXPECT_EQ(returned.context.get_vcId(), context.get_vcId());
    EXPECT_EQ(returned.context.get_apid(), context.get_apid());
    EXPECT_EQ(returned.context.get_sequenceCount(), context.get_sequenceCount());
    EXPECT_EQ(returned.context.get_comQueueIndex(), context.get_comQueueIndex());
}

void UhfCcsdsVcidAdapterTester::testStatusPathPassesThrough() {
    Fw::Success status = Fw::Success::SUCCESS;

    this->clearHistory();
    this->invoke_to_comStatusIn(0, status);

    ASSERT_from_comStatusOut_SIZE(1);
    EXPECT_EQ(this->fromPortHistory_comStatusOut->at(0).condition, Fw::Success::SUCCESS);
}

}  // namespace OBC
