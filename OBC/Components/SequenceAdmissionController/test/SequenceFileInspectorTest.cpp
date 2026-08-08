#include "gtest/gtest.h"

#include "OBC/Components/SequenceAdmissionController/SequenceFileInspector.hpp"
#include "OBC/Components/test/OfficialSequenceTestSupport.hpp"

namespace OBC {
namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;

TEST(SequenceFileInspector, ValidSequenceReturnsRecordsAndCrc) {
    TestSupport::TempDirectory tempDir;
    ASSERT_TRUE(tempDir.valid());

    const std::string filePath = TestSupport::joinPath(tempDir.path(), "valid.seq");
    ASSERT_TRUE(TestSupport::writeOfficialSequenceFile(
        filePath, {{0U, 10U, 0U, TestSupport::makeCommandBytes(OPCODE_MODE_GET)}}));

    SequenceFileInspector inspector;
    const SequenceInspectionResult result = inspector.inspect(filePath, Fw::Time::zero(TimeBase::TB_NONE));
    ASSERT_TRUE(result.valid);
    ASSERT_EQ(result.records.size(), 1U);
    EXPECT_EQ(result.records[0].opcode, OPCODE_MODE_GET);
    EXPECT_NE(result.crc, 0U);
}

TEST(SequenceFileInspector, CrcMismatchRejectsFile) {
    TestSupport::TempDirectory tempDir;
    ASSERT_TRUE(tempDir.valid());

    const std::string filePath = TestSupport::joinPath(tempDir.path(), "bad-crc.seq");
    ASSERT_TRUE(TestSupport::writeOfficialSequenceFile(
        filePath, {{1U, 3U, 0U, TestSupport::makeCommandBytes(OPCODE_MODE_GET)}}));

    std::vector<U8> bytes = TestSupport::readBinaryFile(filePath);
    ASSERT_FALSE(bytes.empty());
    bytes.back() ^= 0xFFU;
    ASSERT_TRUE(TestSupport::writeBinaryFile(filePath, bytes));

    SequenceFileInspector inspector;
    const SequenceInspectionResult result = inspector.inspect(filePath, Fw::Time::zero(TimeBase::TB_NONE));
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.error, SequenceInspectionError::CRC_MISMATCH);
}

TEST(SequenceFileInspector, TimeBaseMismatchRejectsFile) {
    TestSupport::TempDirectory tempDir;
    ASSERT_TRUE(tempDir.valid());

    const std::string filePath = TestSupport::joinPath(tempDir.path(), "time-base.seq");
    ASSERT_TRUE(TestSupport::writeOfficialSequenceFile(
        filePath, {{0U, 0U, 0U, TestSupport::makeCommandBytes(OPCODE_MODE_SET)}}, TimeBase::TB_WORKSTATION_TIME));

    SequenceFileInspector inspector;
    const SequenceInspectionResult result = inspector.inspect(filePath, Fw::Time::zero(TimeBase::TB_SC_TIME));
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.error, SequenceInspectionError::TIME_BASE_MISMATCH);
}

}  // namespace
}  // namespace OBC
