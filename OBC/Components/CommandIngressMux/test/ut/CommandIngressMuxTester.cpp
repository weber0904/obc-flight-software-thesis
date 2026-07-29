#include "CommandIngressMuxTester.hpp"

#include <Fw/Cmd/CmdPacket.hpp>

namespace OBC {

CommandIngressMuxTester::CommandIngressMuxTester()
    : CommandIngressMuxGTestBase("CommandIngressMuxTester", MAX_HISTORY_SIZE), component("CommandIngressMux") {
    this->initComponents();
    this->connectPorts();
}

CommandIngressMuxTester::~CommandIngressMuxTester() = default;

void CommandIngressMuxTester::testMergesBothIngressPorts() {
    this->m_forwardedCommands.clear();
    this->m_forwardedStatuses.clear();

    Fw::ComBuffer sband = this->makeCommand_(0x100U);
    this->invoke_to_commandIn(0, sband, 41U);
    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);
    EXPECT_EQ(this->m_forwardedCommands[0].portNum, 0);
    EXPECT_NE(this->m_forwardedCommands[0].context, 41U);

    Fw::ComBuffer uhf = this->makeCommand_(0x101U);
    this->invoke_to_commandIn(1, uhf, 42U);
    ASSERT_EQ(this->m_forwardedCommands.size(), 2U);
    EXPECT_EQ(this->m_forwardedCommands[1].portNum, 0);
    EXPECT_NE(this->m_forwardedCommands[1].context, 42U);
    EXPECT_NE(this->m_forwardedCommands[0].context, this->m_forwardedCommands[1].context);
}

void CommandIngressMuxTester::testRoutesStatusesBackToRememberedIngress() {
    this->m_forwardedCommands.clear();
    this->m_forwardedStatuses.clear();

    Fw::ComBuffer uhf = this->makeCommand_(0x101U);
    this->invoke_to_commandIn(1, uhf, 42U);
    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);

    const U32 dispatchContext = this->m_forwardedCommands[0].context;
    this->invoke_to_commandStatusIn(0, 0x101U, dispatchContext, Fw::CmdResponse::OK);
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].portNum, 1);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, 0x101U);
    EXPECT_EQ(this->m_forwardedStatuses[0].context, 42U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::OK);
}

void CommandIngressMuxTester::testDuplicateOriginalContextsUseMuxDispatchContext() {
    this->m_forwardedCommands.clear();
    this->m_forwardedStatuses.clear();

    Fw::ComBuffer sband = this->makeCommand_(0x100U);
    Fw::ComBuffer uhf = this->makeCommand_(0x101U);
    this->invoke_to_commandIn(0, sband, 77U);
    this->invoke_to_commandIn(1, uhf, 77U);
    ASSERT_EQ(this->m_forwardedCommands.size(), 2U);
    const U32 sbandDispatchContext = this->m_forwardedCommands[0].context;
    const U32 uhfDispatchContext = this->m_forwardedCommands[1].context;
    ASSERT_NE(sbandDispatchContext, uhfDispatchContext);

    this->invoke_to_commandStatusIn(0, 0x101U, uhfDispatchContext, Fw::CmdResponse::OK);
    this->invoke_to_commandStatusIn(0, 0x100U, sbandDispatchContext, Fw::CmdResponse::EXECUTION_ERROR);

    ASSERT_EQ(this->m_forwardedStatuses.size(), 2U);
    EXPECT_EQ(this->m_forwardedStatuses[0].portNum, 1);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, 0x101U);
    EXPECT_EQ(this->m_forwardedStatuses[0].context, 77U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::OK);
    EXPECT_EQ(this->m_forwardedStatuses[1].portNum, 0);
    EXPECT_EQ(this->m_forwardedStatuses[1].opcode, 0x100U);
    EXPECT_EQ(this->m_forwardedStatuses[1].context, 77U);
    EXPECT_EQ(this->m_forwardedStatuses[1].response, Fw::CmdResponse::EXECUTION_ERROR);
}

void CommandIngressMuxTester::testFullTableFailsClosedWithoutOverwritingExistingMappings() {
    this->m_forwardedCommands.clear();
    this->m_forwardedStatuses.clear();

    for (FwIndexType slot = 0; slot < CommandIngressMuxIngressPorts * 4; slot++) {
        Fw::ComBuffer command = this->makeCommand_(static_cast<FwOpcodeType>(0x200U + slot));
        this->invoke_to_commandIn(slot % CommandIngressMuxIngressPorts, command, 100U + slot);
    }
    ASSERT_EQ(this->m_forwardedCommands.size(), static_cast<size_t>(CommandIngressMuxIngressPorts * 4));
    EXPECT_TRUE(this->m_forwardedStatuses.empty());

    Fw::ComBuffer overflow = this->makeCommand_(0x300U);
    this->invoke_to_commandIn(1, overflow, 999U);
    ASSERT_EQ(this->m_forwardedCommands.size(), static_cast<size_t>(CommandIngressMuxIngressPorts * 4));
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].portNum, 1);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, 0x300U);
    EXPECT_EQ(this->m_forwardedStatuses[0].context, 999U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::EXECUTION_ERROR);

    this->invoke_to_commandStatusIn(0, 0x200U, this->m_forwardedCommands[0].context, Fw::CmdResponse::OK);
    ASSERT_EQ(this->m_forwardedStatuses.size(), 2U);
    EXPECT_EQ(this->m_forwardedStatuses[1].portNum, 0);
    EXPECT_EQ(this->m_forwardedStatuses[1].opcode, 0x200U);
    EXPECT_EQ(this->m_forwardedStatuses[1].context, 100U);
    EXPECT_EQ(this->m_forwardedStatuses[1].response, Fw::CmdResponse::OK);
}

void CommandIngressMuxTester::from_commandOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    this->m_forwardedCommands.push_back({portNum, data, context});
}

void CommandIngressMuxTester::from_commandStatusOut_handler(FwIndexType portNum,
                                                            FwOpcodeType opCode,
                                                            U32 cmdSeq,
                                                            const Fw::CmdResponse& response) {
    this->m_forwardedStatuses.push_back({portNum, opCode, cmdSeq, response});
}

Fw::ComBuffer CommandIngressMuxTester::makeCommand_(FwOpcodeType opcode) const {
    Fw::ComBuffer buffer;
    EXPECT_EQ(buffer.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(buffer.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
    return buffer;
}

}  // namespace OBC
