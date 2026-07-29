#include "simulators/comm/CommCspProtocol.hpp"

#include <gtest/gtest.h>

TEST(CommReliableTransferProtocol, ControlRequestDefaultsToReliableControlPort) {
    const auto request =
        OBC::COMM::CSP::makeReliableTransferControlRequest(OBC::COMM::CSP::ReliableTransferOp::BEGIN, 17U);
    EXPECT_EQ(request.header.version, OBC::COMM::CSP::VERSION);
    EXPECT_EQ(request.header.service,
              static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_CONTROL));
    EXPECT_EQ(request.header.seq, 17U);
    EXPECT_EQ(request.op, static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferOp::BEGIN));
}

TEST(CommReliableTransferProtocol, DataFrameUsesBoundedPacketCeiling) {
    const auto frame = OBC::COMM::CSP::makeReliableTransferDataFrame(42U);
    EXPECT_EQ(frame.version, OBC::COMM::CSP::VERSION);
    EXPECT_EQ(frame.kind, OBC::COMM::CSP::RELIABLE_DATA_PACKET);
    EXPECT_EQ(frame.transferId, 42U);
    EXPECT_GE(OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES, 171U);
    EXPECT_EQ(OBC::COMM::CSP::RELIABLE_SEGMENT_DATA_BYTES, 160U);
}
