#include "OBC/Components/CommController/CommReliableTransfer.hpp"

#include <cstring>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include <gtest/gtest.h>

#include "Fw/Buffer/Buffer.hpp"
#include "Fw/FilePacket/FilePacket.hpp"

namespace {

class FakeReliableTransferRuntime final : public OBC::CSP::ICspRuntime {
  public:
    enum class Scenario {
        HAPPY,
        ACK_TIMEOUT_THEN_SUCCESS,
        ACK_RUNTIME_TIMEOUT_THEN_SUCCESS,
        RETRY_EXHAUSTED,
        RETRY_EXHAUSTED_ABORT_RUNTIME_ERROR,
        DUPLICATE_THEN_SUCCESS,
        REGRESSING_ACK_THEN_SUCCESS,
        BAD_REPLY_SEQUENCE,
        HASH_MISMATCH_ON_COMPLETE,
        CANCEL_RUNTIME_ERROR,
    };

    explicit FakeReliableTransferRuntime(Scenario scenario)
        : m_scenario(scenario) {
        this->m_metrics.initialized = true;
    }

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig&) override {
        this->m_metrics.initialized = true;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus ping(std::uint16_t, std::uint32_t, bool& success) override {
        success = true;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t, std::uint8_t targetPort, const std::string& data) override {
        if (targetPort != static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_DATA) ||
            data.size() != sizeof(OBC::COMM::CSP::ReliableTransferDataFrame)) {
            return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
        }
        OBC::COMM::CSP::ReliableTransferDataFrame frame = {};
        std::memcpy(&frame, data.data(), sizeof(frame));
        this->m_sendCount += 1U;
        this->m_sentPacketSizes.push_back(frame.packetSize);
        if (frame.packetSize > 0U && frame.packetSize <= OBC::COMM::CSP::MAX_RELIABLE_PACKET_BYTES) {
            Fw::Buffer buffer(frame.packetBytes, frame.packetSize);
            Fw::FilePacket packet = {};
            if (packet.fromBuffer(buffer) == Fw::FW_SERIALIZE_OK &&
                packet.asHeader().getType() == Fw::FilePacket::T_DATA) {
                const std::uint32_t sequenceIndex = packet.asDataPacket().asHeader().getSequenceIndex();
                if (sequenceIndex > 0U) {
                    const std::uint32_t segmentIndex = sequenceIndex - 1U;
                    if (segmentIndex < this->m_seenSegments.size()) {
                        this->m_seenSegments[segmentIndex] = true;
                    }
                }
            }
            this->m_uniqueSegmentsSeen = 0U;
            for (bool seen : this->m_seenSegments) {
                if (!seen) {
                    break;
                }
                this->m_uniqueSegmentsSeen += 1U;
            }
        }
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus requestReply(std::uint16_t,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t) override {
        if (targetPort != static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::RELIABLE_TRANSFER_CONTROL) ||
            requestSize != sizeof(OBC::COMM::CSP::ReliableTransferControlRequest) ||
            replyCapacity < sizeof(OBC::COMM::CSP::ReliableTransferControlReply)) {
            return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
        }

        OBC::COMM::CSP::ReliableTransferControlRequest request = {};
        std::memcpy(&request, requestData, sizeof(request));
        OBC::COMM::CSP::ReliableTransferControlReply reply =
            OBC::COMM::CSP::makeReliableTransferControlReply(
                request.header.seq,
                OBC::COMM::CSP::ResultCode::OK,
                static_cast<OBC::COMM::CSP::ReliableTransferOp>(request.op));
        if (this->m_scenario == Scenario::BAD_REPLY_SEQUENCE) {
            reply.header.seq = static_cast<std::uint16_t>(request.header.seq + 1U);
        }
        reply.transferId = request.transferId;

        switch (static_cast<OBC::COMM::CSP::ReliableTransferOp>(request.op)) {
            case OBC::COMM::CSP::ReliableTransferOp::BEGIN:
                this->m_totalSegments = request.segmentCount;
                this->m_seenSegments.assign(this->m_totalSegments, false);
                reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::OK);
                break;
            case OBC::COMM::CSP::ReliableTransferOp::ACK_POLL:
                this->m_ackPollCount += 1U;
                if (this->m_scenario == Scenario::ACK_RUNTIME_TIMEOUT_THEN_SUCCESS && this->m_ackPollCount == 1U) {
                    return OBC::CSP::RuntimeStatus::TIMEOUT;
                }
                if (this->m_scenario == Scenario::RETRY_EXHAUSTED ||
                    this->m_scenario == Scenario::RETRY_EXHAUSTED_ABORT_RUNTIME_ERROR) {
                    reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::NO_PROGRESS);
                } else if (this->m_scenario == Scenario::ACK_TIMEOUT_THEN_SUCCESS && this->m_ackPollCount == 1U) {
                    reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::NO_PROGRESS);
                } else if (this->m_scenario == Scenario::REGRESSING_ACK_THEN_SUCCESS && this->m_ackPollCount == 2U) {
                    reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::OK);
                    reply.contiguousSegments = 1U;
                    reply.committedBytes = OBC::COMM::CSP::RELIABLE_SEGMENT_DATA_BYTES;
                } else if (this->m_scenario == Scenario::DUPLICATE_THEN_SUCCESS && this->m_ackPollCount == 2U) {
                    reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::OK);
                    reply.duplicateSegments = 1U;
                    reply.contiguousSegments = this->m_totalSegments;
                    reply.committedBytes = this->m_totalSegments * OBC::COMM::CSP::RELIABLE_SEGMENT_DATA_BYTES;
                } else {
                    reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::OK);
                    reply.contiguousSegments = std::min(this->m_totalSegments, this->m_uniqueSegmentsSeen);
                    reply.committedBytes = reply.contiguousSegments * OBC::COMM::CSP::RELIABLE_SEGMENT_DATA_BYTES;
                }
                break;
            case OBC::COMM::CSP::ReliableTransferOp::COMPLETE:
                if (this->m_scenario == Scenario::HASH_MISMATCH_ON_COMPLETE) {
                    reply.transferResult =
                        static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::HASH_MISMATCH);
                } else {
                    reply.transferResult = static_cast<std::uint8_t>(this->m_uniqueSegmentsSeen >= this->m_totalSegments
                                                                         ? OBC::COMM::CSP::ReliableTransferResult::OK
                                                                         : OBC::COMM::CSP::ReliableTransferResult::INVALID);
                }
                break;
            case OBC::COMM::CSP::ReliableTransferOp::CANCEL:
                if (this->m_scenario == Scenario::CANCEL_RUNTIME_ERROR) {
                    return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
                }
                reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::CANCELLED);
                break;
            case OBC::COMM::CSP::ReliableTransferOp::ABORT:
                if (this->m_scenario == Scenario::CANCEL_RUNTIME_ERROR ||
                    this->m_scenario == Scenario::RETRY_EXHAUSTED_ABORT_RUNTIME_ERROR) {
                    return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
                }
                reply.transferResult = static_cast<std::uint8_t>(OBC::COMM::CSP::ReliableTransferResult::ABORTED);
                break;
        }

        replySize = sizeof(reply);
        std::memcpy(replyData, &reply, sizeof(reply));
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeMetrics metrics() const override {
        return this->m_metrics;
    }

    void shutdown() override {}

    std::uint32_t sendCount() const { return this->m_sendCount; }
    std::uint32_t ackPollCount() const { return this->m_ackPollCount; }

  private:
    Scenario m_scenario;
    OBC::CSP::RuntimeMetrics m_metrics = {};
    std::uint32_t m_totalSegments = 0U;
    std::uint32_t m_uniqueSegmentsSeen = 0U;
    std::uint32_t m_sendCount = 0U;
    std::uint32_t m_ackPollCount = 0U;
    std::vector<bool> m_seenSegments;
    std::vector<std::uint16_t> m_sentPacketSizes;
};

std::string makeTempFileWithBytes(std::size_t size) {
    char path[] = "/tmp/comm-reliable-transfer-test-XXXXXX";
    const int fd = ::mkstemp(path);
    EXPECT_GE(fd, 0);
    ::close(fd);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    for (std::size_t i = 0; i < size; ++i) {
        out.put(static_cast<char>('A' + (i % 20U)));
    }
    out.close();
    return path;
}

void removeIfExists(const std::string& path) {
    std::remove(path.c_str());
}

}  // namespace

TEST(CommReliableTransfer, HappyPathCompletes) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HAPPY);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    const Svc::SendFileResponse start =
        transfer.start(111U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EQ(start.get_status(), Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    EXPECT_EQ(finalResult.response.get_status(), Svc::SendFileStatus::STATUS_OK);
    EXPECT_GE(runtime.sendCount(), 3U);
    removeIfExists(path);
}

TEST(CommReliableTransfer, ZeroByteFileCompletesWithoutDataFrames) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HAPPY);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(0U);

    const Svc::SendFileResponse start =
        transfer.start(112U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    ASSERT_EQ(start.get_status(), Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    EXPECT_EQ(finalResult.response.get_status(), Svc::SendFileStatus::STATUS_OK);
    EXPECT_EQ(runtime.sendCount(), 0U);
    EXPECT_EQ(runtime.ackPollCount(), 0U);
    removeIfExists(path);
}

TEST(CommReliableTransfer, TimeoutResendThenSuccess) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::ACK_TIMEOUT_THEN_SUCCESS);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    ASSERT_EQ(transfer.start(222U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    bool sawResend = false;
    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
        sawResend = sawResend || finalResult.resendAttempted;
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    EXPECT_TRUE(sawResend);
    EXPECT_GT(runtime.sendCount(), 3U);
    removeIfExists(path);
}

TEST(CommReliableTransfer, AckRuntimeTimeoutUsesNoProgressSemantics) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::ACK_RUNTIME_TIMEOUT_THEN_SUCCESS);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    ASSERT_EQ(transfer.start(223U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    bool sawResend = false;
    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
        sawResend = sawResend || finalResult.resendAttempted;
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    EXPECT_TRUE(sawResend);
    EXPECT_GT(runtime.sendCount(), 3U);
    removeIfExists(path);
}

TEST(CommReliableTransfer, RetryExhaustedReturnsFinalFailure) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::RETRY_EXHAUSTED);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    ASSERT_EQ(transfer.start(333U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::RETRY_EXHAUSTED);
    EXPECT_EQ(finalResult.response.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    removeIfExists(path);
}

TEST(CommReliableTransfer, RetryExhaustedPreservesAbortTransportFailure) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::RETRY_EXHAUSTED_ABORT_RUNTIME_ERROR);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    ASSERT_EQ(transfer.start(334U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::IO_ERROR);
    EXPECT_EQ(finalResult.response.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    removeIfExists(path);
}

TEST(CommReliableTransfer, DuplicateObservationPropagatesToStepResult) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::DUPLICATE_THEN_SUCCESS);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(480U);

    ASSERT_EQ(transfer.start(444U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    bool sawDuplicate = false;
    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
        sawDuplicate = sawDuplicate || finalResult.duplicateObserved;
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    EXPECT_TRUE(sawDuplicate);
    removeIfExists(path);
}

TEST(CommReliableTransfer, AckProgressDoesNotRegress) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::REGRESSING_ACK_THEN_SUCCESS);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(480U);

    ASSERT_EQ(transfer.start(445U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult result = transfer.step();
    EXPECT_FALSE(result.finished);
    EXPECT_EQ(transfer.state().contiguousSegments, 2U);

    result = transfer.step();
    EXPECT_FALSE(result.finished);
    EXPECT_EQ(transfer.state().contiguousSegments, 2U);

    for (int i = 0; i < 10 && !result.finished; ++i) {
        result = transfer.step();
    }

    EXPECT_TRUE(result.finished);
    EXPECT_EQ(result.finalResult, OBC::CommReliableTransferResult::SUCCESS);
    removeIfExists(path);
}

TEST(CommReliableTransfer, CancelReturnsCancelledResult) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HAPPY);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(320U);

    ASSERT_EQ(transfer.start(555U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    const OBC::CommReliableTransferStepResult cancelled = transfer.cancel(false);
    EXPECT_TRUE(cancelled.finished);
    EXPECT_EQ(cancelled.finalResult, OBC::CommReliableTransferResult::CANCELLED);
    EXPECT_EQ(cancelled.response.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    EXPECT_FALSE(transfer.isActive());
    removeIfExists(path);
}

TEST(CommReliableTransfer, AbortReturnsAbortedResult) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HAPPY);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(320U);

    ASSERT_EQ(transfer.start(666U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    const OBC::CommReliableTransferStepResult aborted = transfer.cancel(true);
    EXPECT_TRUE(aborted.finished);
    EXPECT_EQ(aborted.finalResult, OBC::CommReliableTransferResult::ABORTED);
    EXPECT_EQ(aborted.response.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    EXPECT_FALSE(transfer.isActive());
    removeIfExists(path);
}

TEST(CommReliableTransfer, CancelTransportFailureReturnsIoError) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::CANCEL_RUNTIME_ERROR);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(320U);

    ASSERT_EQ(transfer.start(667U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    const OBC::CommReliableTransferStepResult aborted = transfer.cancel(true);
    EXPECT_TRUE(aborted.finished);
    EXPECT_EQ(aborted.finalResult, OBC::CommReliableTransferResult::IO_ERROR);
    EXPECT_EQ(aborted.response.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    EXPECT_FALSE(transfer.isActive());
    removeIfExists(path);
}

TEST(CommReliableTransfer, InvalidReplySequenceFailsStart) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::BAD_REPLY_SEQUENCE);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(320U);

    const Svc::SendFileResponse start =
        transfer.start(777U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    EXPECT_EQ(start.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    EXPECT_FALSE(transfer.isActive());
    EXPECT_EQ(transfer.state().lastStartFailureStage, OBC::CommReliableTransferStartFailureStage::BEGIN_REPLY_INVALID);
    EXPECT_EQ(transfer.state().lastStartFailureDetail, sizeof(OBC::COMM::CSP::ReliableTransferControlReply));
    removeIfExists(path);
}

TEST(CommReliableTransfer, StartRejectsFileAboveBoundedCeiling) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HAPPY);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path =
        makeTempFileWithBytes(static_cast<std::size_t>(OBC::COMM::CSP::MAX_RELIABLE_TRANSFER_FILE_BYTES) + 1U);

    const Svc::SendFileResponse start =
        transfer.start(778U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U);
    EXPECT_EQ(start.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    EXPECT_FALSE(transfer.isActive());
    EXPECT_EQ(transfer.state().lastStartFailureStage, OBC::CommReliableTransferStartFailureStage::FILE_TOO_LARGE);
    EXPECT_EQ(transfer.state().lastStartFailureDetail, OBC::COMM::CSP::MAX_RELIABLE_TRANSFER_FILE_BYTES + 1U);
    removeIfExists(path);
}

TEST(CommReliableTransfer, HashMismatchMapsToFinalFailure) {
    FakeReliableTransferRuntime runtime(FakeReliableTransferRuntime::Scenario::HASH_MISMATCH_ON_COMPLETE);
    OBC::CommReliableTransfer transfer(runtime);
    const std::string path = makeTempFileWithBytes(400U);

    ASSERT_EQ(transfer.start(888U, Fw::FileNameString(path.c_str()), Fw::FileNameString("hk.fdp"), 0U, 0U).get_status(),
              Svc::SendFileStatus::STATUS_OK);

    OBC::CommReliableTransferStepResult finalResult = {};
    for (int i = 0; i < 10 && !finalResult.finished; ++i) {
        finalResult = transfer.step();
    }

    EXPECT_TRUE(finalResult.finished);
    EXPECT_EQ(finalResult.finalResult, OBC::CommReliableTransferResult::HASH_MISMATCH);
    EXPECT_EQ(finalResult.response.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    removeIfExists(path);
}
