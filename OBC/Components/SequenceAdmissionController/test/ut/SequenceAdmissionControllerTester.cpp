#include "SequenceAdmissionControllerTester.hpp"

#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>

#include "Fw/Cmd/CmdPacket.hpp"
#include "Fw/Types/WaitEnumAc.hpp"
#include "OBC/Components/FileIngressAuthority/FileIngressPolicy.hpp"
#include "OBC/Components/test/OfficialSequenceTestSupport.hpp"

namespace OBC {

namespace {

std::string canonicalizePath(const std::string& path) {
    char resolved[PATH_MAX] = {};
    if (::realpath(path.c_str(), resolved) == nullptr) {
        return path;
    }
    return resolved;
}

std::string findAliasName(const std::string& workingRoot, const char* prefix) {
    DIR* dir = ::opendir(workingRoot.c_str());
    if (dir == nullptr) {
        return std::string();
    }
    std::string aliasName;
    while (dirent* entry = ::readdir(dir)) {
        if (std::strncmp(entry->d_name, prefix, std::strlen(prefix)) == 0) {
            aliasName = entry->d_name;
            break;
        }
    }
    ::closedir(dir);
    return aliasName;
}

std::string expectedAdmittedPhysicalPath(const std::string& runtimeRoot, U32 contextId, const char* leafName) {
    return TestSupport::joinPath(canonicalizePath(runtimeRoot),
                                 TestSupport::joinPath("sequences/admitted",
                                                       "ctx-" + std::to_string(contextId) + "-" + leafName));
}

std::string expectedAdmittedCommandPath(const std::string& runtimeRoot, U32 contextId, const char* leafName) {
    const std::string aliasName = findAliasName(runtimeRoot, ".adm-");
    return aliasName + "/ctx-" + std::to_string(contextId) + "-" + leafName;
}

}  // namespace

SequenceAdmissionControllerTester::SequenceAdmissionControllerTester()
    : SequenceAdmissionControllerGTestBase("SequenceAdmissionControllerTester", MAX_HISTORY_SIZE),
      component("SequenceAdmissionController") {
    EXPECT_TRUE(this->m_runtimeRoot.valid());
    this->initComponents();
    this->connectPorts();
    EXPECT_TRUE(this->component.configureRuntime(this->m_runtimeRoot.path(), this->m_runtimeRoot.path()));
    EXPECT_TRUE(TestSupport::ensureDirectory(TestSupport::joinPath(this->m_runtimeRoot.path(), "sequences/staging")));
}

SequenceAdmissionControllerTester::~SequenceAdmissionControllerTester() {
    this->component.deinit();
}

void SequenceAdmissionControllerTester::testValidateAcceptsBackupReadableSequence() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("readable.seq", OPCODE_MODE_GET);

    const SequenceControlResult result = this->invoke_to_controlIn(
        0, this->makeRequest(SequenceControlAction::SEQ_VALIDATE, logicalPath.c_str(), AuthorityLinkIdentity::UHF,
                             AuthorityLinkRole::BACKUP));

    EXPECT_TRUE(result.get_accepted());
    EXPECT_FALSE(result.get_deferred());
    EXPECT_EQ(result.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::OK));
    EXPECT_TRUE(this->m_internalCommands.empty());
}

void SequenceAdmissionControllerTester::testRunRejectsBackupHighAuthoritySequence() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("denied.seq", OPCODE_MODE_SET);

    const SequenceControlResult result = this->invoke_to_controlIn(
        0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(), AuthorityLinkIdentity::UHF,
                             AuthorityLinkRole::BACKUP, 0U, Fw::Wait::NO_WAIT));

    EXPECT_FALSE(result.get_accepted());
    EXPECT_EQ(result.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    ASSERT_TLM_SEQ_REJECT_TOTAL(0, 1U);
    EXPECT_TRUE(this->m_internalCommands.empty());
}

void SequenceAdmissionControllerTester::testPrepareManualUsesAdmittedCopyAndAllowsOwnedStart() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("manual.seq", OPCODE_MODE_GET);
    const std::vector<U8> originalBytes =
        TestSupport::readBinaryFile(TestSupport::joinPath(this->m_runtimeRoot.path(), "sequences/staging/manual.seq"));

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::UHF, AuthorityLinkRole::BACKUP));

    ASSERT_TRUE(prepare.get_accepted());
    ASSERT_TRUE(prepare.get_deferred());
    ASSERT_EQ(this->m_internalCommands.size(), 1U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[0].data),
              OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    ASSERT_TRUE(TestSupport::writeOfficialSequenceFile(
        TestSupport::joinPath(this->m_runtimeRoot.path(), "sequences/staging/manual.seq"),
        {{0U, 0U, 0U, TestSupport::makeCommandBytes(OPCODE_MODE_SET)}}));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    ASSERT_EQ(this->m_internalCommands.size(), 2U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[1].data),
              OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    const std::string expectedCommandPath =
        expectedAdmittedCommandPath(this->m_runtimeRoot.path(), prepare.get_contextId(), "manual.seq");
    const std::string expectedPhysicalPath =
        expectedAdmittedPhysicalPath(this->m_runtimeRoot.path(), prepare.get_contextId(), "manual.seq");
    const std::string admittedPath = this->decodeCommandPath(this->m_internalCommands[1].data);
    EXPECT_FALSE(admittedPath.empty());
    EXPECT_EQ(admittedPath, expectedCommandPath);
    EXPECT_EQ(this->decodeCommandBlockState(this->m_internalCommands[1].data), Svc::CmdSequencer_BlockState::NO_BLOCK);
    EXPECT_EQ(TestSupport::readBinaryFile(expectedPhysicalPath), originalBytes);

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    ASSERT_EQ(this->m_controlStatuses.size(), 1U);
    EXPECT_EQ(this->m_controlStatuses[0].status.get_originalOpcode(), OBC_SEQ_PREPARE_MANUAL_OPCODE);

    const SequenceControlResult start =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_START, "", AuthorityLinkIdentity::UHF,
                                                       AuthorityLinkRole::BACKUP, prepare.get_contextId()));
    ASSERT_TRUE(start.get_accepted());
    ASSERT_TRUE(start.get_deferred());
    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_A_BASE_ID));
}

void SequenceAdmissionControllerTester::testManualCompletionRestoresAutoMode() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("restore.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const std::string admittedPath = expectedAdmittedCommandPath(this->m_runtimeRoot.path(), prepare.get_contextId(), "restore.seq");
    this->invoke_to_seqStartIn(0, Fw::CmdStringArg(admittedPath.c_str()));
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqDoneIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    const std::size_t controlStatusCount = this->m_controlStatuses.size();
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 4U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    EXPECT_EQ(this->m_controlStatuses.size(), controlStatusCount);
}

void SequenceAdmissionControllerTester::testManualCancelRestoresAutoMode() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("cancel-restore.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const SequenceControlResult cancel =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_CANCEL, "", AuthorityLinkIdentity::SBAND,
                                                       AuthorityLinkRole::PRIMARY, prepare.get_contextId()));
    ASSERT_TRUE(cancel.get_accepted());
    ASSERT_TRUE(cancel.get_deferred());
    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 4U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[3].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    const std::size_t controlStatusCount = this->m_controlStatuses.size();
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 4U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    EXPECT_EQ(this->m_controlStatuses.size(), controlStatusCount);
}

void SequenceAdmissionControllerTester::testManualLoadFailureRestoresAutoMode() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("manual-load-fail.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    ASSERT_EQ(this->m_internalCommands.size(), 2U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[1].data),
              OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U,
                                        Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));
    ASSERT_EQ(this->m_controlStatuses.size(), 1U);
    EXPECT_EQ(this->m_controlStatuses[0].status.get_originalOpcode(), OBC_SEQ_PREPARE_MANUAL_OPCODE);
    EXPECT_EQ(this->m_controlStatuses[0].status.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::EXECUTION_ERROR));

    const std::size_t controlStatusCount = this->m_controlStatuses.size();
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    EXPECT_EQ(this->m_controlStatuses.size(), controlStatusCount);
}

void SequenceAdmissionControllerTester::testAutoRestoreFailureIsRetried() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("auto-retry.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const std::string admittedPath =
        expectedAdmittedCommandPath(this->m_runtimeRoot.path(), prepare.get_contextId(), "auto-retry.seq");
    this->invoke_to_seqStartIn(0, Fw::CmdStringArg(admittedPath.c_str()));
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqDoneIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 4U,
                                        Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 4U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[3].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));
}

void SequenceAdmissionControllerTester::testAdmissionCopyFailureCleansUpAdmittedPath() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("copy-cleanup.seq", OPCODE_MODE_GET);
    const std::string admittedPhysicalPath =
        expectedAdmittedPhysicalPath(this->m_runtimeRoot.path(), 1U, "copy-cleanup.seq");

    ASSERT_EQ(::symlink(this->m_runtimeRoot.path().c_str(), admittedPhysicalPath.c_str()), 0);

    const SequenceControlResult result =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
    EXPECT_FALSE(result.get_accepted());
    EXPECT_EQ(result.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    EXPECT_TRUE(this->m_internalCommands.empty());

    struct stat pathInfo = {};
    EXPECT_NE(::lstat(admittedPhysicalPath.c_str(), &pathInfo), 0);
    const int statusErrno = errno;
    EXPECT_EQ(statusErrno, ENOENT);
}

void SequenceAdmissionControllerTester::testOwnerMismatchRejectsControl() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("owner.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare = this->invoke_to_controlIn(
        0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(), AuthorityLinkIdentity::SBAND,
                             AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const SequenceControlResult start = this->invoke_to_controlIn(
        0, this->makeRequest(SequenceControlAction::SEQ_START, "", AuthorityLinkIdentity::UHF, AuthorityLinkRole::BACKUP,
                             prepare.get_contextId()));
    EXPECT_FALSE(start.get_accepted());
    EXPECT_EQ(start.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
}

void SequenceAdmissionControllerTester::testStartAndStepRejectAutoRunContext() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("auto-run-control.seq", OPCODE_MODE_GET);

    const SequenceControlResult run =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
    ASSERT_TRUE(run.get_accepted());
    ASSERT_EQ(this->m_internalCommands.size(), 1U);

    const std::string admittedPath =
        expectedAdmittedCommandPath(this->m_runtimeRoot.path(), run.get_contextId(), "auto-run-control.seq");
    this->invoke_to_internalCmdStatusIn(0, OBC_SEQ_DISPATCHER_RUN_OPCODE, 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqStartIn(0, Fw::CmdStringArg(admittedPath.c_str()));
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const std::size_t commandCount = this->m_internalCommands.size();
    const SequenceControlResult start =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_START, "", AuthorityLinkIdentity::DEV_DIRECT,
                                                       AuthorityLinkRole::DEV_FULL, run.get_contextId()));
    EXPECT_FALSE(start.get_accepted());
    EXPECT_EQ(start.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    EXPECT_EQ(this->m_internalCommands.size(), commandCount);

    const SequenceControlResult step =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_STEP, "", AuthorityLinkIdentity::DEV_DIRECT,
                                                       AuthorityLinkRole::DEV_FULL, run.get_contextId()));
    EXPECT_FALSE(step.get_accepted());
    EXPECT_EQ(step.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    EXPECT_EQ(this->m_internalCommands.size(), commandCount);
}

void SequenceAdmissionControllerTester::testManualStartAndStepFailuresRestoreAutoMode() {
    this->clearCaptures();

    const std::string startPath = this->stageSequenceFile("manual-start-fail.seq", OPCODE_MODE_GET);
    const SequenceControlResult prepareStart =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, startPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepareStart.get_accepted());
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const SequenceControlResult start =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_START, "", AuthorityLinkIdentity::SBAND,
                                                       AuthorityLinkRole::PRIMARY, prepareStart.get_contextId()));
    ASSERT_TRUE(start.get_accepted());
    ASSERT_TRUE(start.get_deferred());
    ASSERT_EQ(this->m_internalCommands.size(), 3U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[2].data),
              OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U,
                                        Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 4U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[3].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));
    ASSERT_EQ(this->m_controlStatuses.size(), 2U);
    EXPECT_EQ(this->m_controlStatuses[1].status.get_originalOpcode(), OBC_SEQ_START_OPCODE);
    EXPECT_EQ(this->m_controlStatuses[1].status.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::EXECUTION_ERROR));

    const std::size_t controlStatusCount = this->m_controlStatuses.size();
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 4U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    EXPECT_EQ(this->m_controlStatuses.size(), controlStatusCount);

    const std::string stepPath = this->stageSequenceFile("manual-step-fail.seq", OPCODE_MODE_GET);
    const SequenceControlResult prepareStep =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, stepPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepareStep.get_accepted());
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 5U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 6U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const SequenceControlResult step =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_STEP, "", AuthorityLinkIdentity::SBAND,
                                                       AuthorityLinkRole::PRIMARY, prepareStep.get_contextId()));
    ASSERT_TRUE(step.get_accepted());
    ASSERT_TRUE(step.get_deferred());
    ASSERT_EQ(this->m_internalCommands.size(), 7U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[6].data),
              OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_A_BASE_ID));

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 7U,
                                        Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_EQ(this->m_internalCommands.size(), 8U);
    EXPECT_EQ(this->decodeCommandOpcode(this->m_internalCommands[7].data),
              OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID));
    ASSERT_EQ(this->m_controlStatuses.size(), 4U);
    EXPECT_EQ(this->m_controlStatuses[3].status.get_originalOpcode(), OBC_SEQ_STEP_OPCODE);
    EXPECT_EQ(this->m_controlStatuses[3].status.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::EXECUTION_ERROR));
}

void SequenceAdmissionControllerTester::testRunLifecycleUpdatesFailureState() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("run.seq", OPCODE_MODE_GET);

    const SequenceControlResult run =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
    ASSERT_TRUE(run.get_accepted());
    ASSERT_EQ(this->m_internalCommands.size(), 1U);

    const std::string admittedPath = expectedAdmittedCommandPath(this->m_runtimeRoot.path(), run.get_contextId(), "run.seq");
    this->invoke_to_internalCmdStatusIn(0, OBC_SEQ_DISPATCHER_RUN_OPCODE, 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqStartIn(0, Fw::CmdStringArg(admittedPath.c_str()));
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqDoneIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    EXPECT_EQ(this->decodeCommandPath(this->m_internalCommands[0].data), admittedPath);
    ASSERT_TLM_SEQ_LAST_STATE(3, SequenceContextState::FAILED);
    ASSERT_TLM_SEQ_LAST_SEQUENCER(0, 0U);
}

void SequenceAdmissionControllerTester::testRunFailureBeforeSeqStartUsesPredictedSequencer() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("prestart-fail.seq", OPCODE_MODE_GET);

    const SequenceControlResult run =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
    ASSERT_TRUE(run.get_accepted());
    ASSERT_EQ(this->m_internalCommands.size(), 1U);

    this->invoke_to_internalCmdStatusIn(0, OBC_SEQ_DISPATCHER_RUN_OPCODE, 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_seqDoneIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    ASSERT_TLM_SEQ_LAST_STATE(2, SequenceContextState::FAILED);
    ASSERT_TLM_SEQ_LAST_SEQUENCER(0, 0U);
}

void SequenceAdmissionControllerTester::testTerminalContextsRejectFurtherControl() {
    this->clearCaptures();
    const std::string logicalPath = this->stageSequenceFile("terminal.seq", OPCODE_MODE_GET);

    const SequenceControlResult prepare =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_PREPARE_MANUAL, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY));
    ASSERT_TRUE(prepare.get_accepted());

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 2U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const SequenceControlResult cancel =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_CANCEL, "", AuthorityLinkIdentity::SBAND,
                                                       AuthorityLinkRole::PRIMARY, prepare.get_contextId()));
    ASSERT_TRUE(cancel.get_accepted());
    ASSERT_EQ(this->m_internalCommands.size(), 3U);

    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 3U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    this->invoke_to_internalCmdStatusIn(0, OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID), 4U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->dispatchCurrentMessages(this->component),
              SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);

    const std::size_t commandCount = this->m_internalCommands.size();
    const SequenceControlResult restart =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_START, "", AuthorityLinkIdentity::SBAND,
                                                       AuthorityLinkRole::PRIMARY, prepare.get_contextId()));
    EXPECT_FALSE(restart.get_accepted());
    EXPECT_EQ(restart.get_cmdResponse(), static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    EXPECT_EQ(this->m_internalCommands.size(), commandCount);
}

void SequenceAdmissionControllerTester::testTerminalContextsAreReused() {
    this->clearCaptures();

    for (U32 i = 0U; i < 8U; ++i) {
        const std::string leaf = "reuse-" + std::to_string(i) + ".seq";
        const std::string logicalPath = this->stageSequenceFile(leaf.c_str(), OPCODE_MODE_GET);

        const SequenceControlResult run =
            this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                           AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
        ASSERT_TRUE(run.get_accepted());
        const std::string admittedPath = expectedAdmittedCommandPath(this->m_runtimeRoot.path(), run.get_contextId(), leaf.c_str());
        this->invoke_to_internalCmdStatusIn(0, OBC_SEQ_DISPATCHER_RUN_OPCODE, i + 1U, Fw::CmdResponse::OK);
        ASSERT_EQ(this->dispatchCurrentMessages(this->component),
                  SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
        this->invoke_to_seqStartIn(0, Fw::CmdStringArg(admittedPath.c_str()));
        ASSERT_EQ(this->dispatchCurrentMessages(this->component),
                  SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
        this->invoke_to_seqDoneIn(0, OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID), i + 100U, Fw::CmdResponse::OK);
        ASSERT_EQ(this->dispatchCurrentMessages(this->component),
                  SequenceAdmissionControllerComponentBase::MsgDispatchStatus::MSG_DISPATCH_OK);
    }

    const std::string logicalPath = this->stageSequenceFile("reuse-final.seq", OPCODE_MODE_GET);
    const SequenceControlResult run =
        this->invoke_to_controlIn(0, this->makeRequest(SequenceControlAction::SEQ_RUN, logicalPath.c_str(),
                                                       AuthorityLinkIdentity::DEV_DIRECT, AuthorityLinkRole::DEV_FULL));
    EXPECT_TRUE(run.get_accepted());
    EXPECT_TRUE(run.get_deferred());
}

void SequenceAdmissionControllerTester::clearCaptures() {
    this->clearHistory();
    this->m_internalCommands.clear();
    this->m_controlStatuses.clear();
}

void SequenceAdmissionControllerTester::from_internalCmdOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    this->pushFromPortEntry_internalCmdOut(data, context);
    this->m_internalCommands.push_back({portNum, data, context});
}

void SequenceAdmissionControllerTester::from_controlStatusOut_handler(FwIndexType portNum,
                                                                      const SequenceControlStatus& controlStatusArg) {
    this->pushFromPortEntry_controlStatusOut(controlStatusArg);
    this->m_controlStatuses.push_back({portNum, controlStatusArg});
}

SequenceControlRequest SequenceAdmissionControllerTester::makeRequest(SequenceControlAction action,
                                                                      const char* fileName,
                                                                      AuthorityLinkIdentity identity,
                                                                      AuthorityLinkRole role,
                                                                      U32 contextId,
                                                                      Fw::Wait waitMode) const {
    SequenceControlRequest request;
    request.set_operation(action);
    request.set_ingressPort(0U);
    request.set_linkIdentity(static_cast<U32>(identity));
    request.set_linkRole(static_cast<U32>(role));
    FwOpcodeType originalOpcode = OBC_SEQ_VALIDATE_OPCODE;
    switch (action) {
        case SequenceControlAction::SEQ_PREPARE_MANUAL:
            originalOpcode = OBC_SEQ_PREPARE_MANUAL_OPCODE;
            break;
        case SequenceControlAction::SEQ_RUN:
            originalOpcode = OBC_SEQ_RUN_OPCODE;
            break;
        case SequenceControlAction::SEQ_START:
            originalOpcode = OBC_SEQ_START_OPCODE;
            break;
        case SequenceControlAction::SEQ_STEP:
            originalOpcode = OBC_SEQ_STEP_OPCODE;
            break;
        case SequenceControlAction::SEQ_CANCEL:
            originalOpcode = OBC_SEQ_CANCEL_OPCODE;
            break;
        case SequenceControlAction::SEQ_VALIDATE:
        case SequenceControlAction::SEQ_LOG_STATUS:
            originalOpcode = OBC_SEQ_VALIDATE_OPCODE;
            break;
    }
    request.set_originalOpcode(static_cast<U32>(originalOpcode));
    request.set_originalCmdSeq(99U);
    request.set_fileName(Fw::String(fileName));
    request.set_waitMode(static_cast<U32>(waitMode.e));
    request.set_contextId(contextId);
    return request;
}

std::string SequenceAdmissionControllerTester::stageSequenceFile(const char* leafName, FwOpcodeType opcode) {
    const std::string physicalPath =
        TestSupport::joinPath(this->m_runtimeRoot.path(), TestSupport::joinPath("sequences/staging", leafName));
    EXPECT_TRUE(TestSupport::writeOfficialSequenceFile(
        physicalPath, {{0U, 0U, 0U, TestSupport::makeCommandBytes(opcode)}}));
    return std::string(FileIngressPolicy::LOGICAL_PREFIX) + leafName;
}

std::string SequenceAdmissionControllerTester::decodeCommandPath(const Fw::ComBuffer& buffer) const {
    Fw::ComBuffer copy = buffer;
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(copy), Fw::FW_SERIALIZE_OK);
    Fw::CmdArgBuffer& args = packet.getArgBuffer();
    args.resetDeser();
    Fw::CmdStringArg path;
    if (args.deserializeTo(path) != Fw::FW_SERIALIZE_OK) {
        return std::string();
    }
    return path.toChar();
}

FwOpcodeType SequenceAdmissionControllerTester::decodeCommandOpcode(const Fw::ComBuffer& buffer) const {
    Fw::ComBuffer copy = buffer;
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(copy), Fw::FW_SERIALIZE_OK);
    return packet.getOpCode();
}

Svc::CmdSequencer_BlockState SequenceAdmissionControllerTester::decodeCommandBlockState(const Fw::ComBuffer& buffer) const {
    Fw::ComBuffer copy = buffer;
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(copy), Fw::FW_SERIALIZE_OK);
    Fw::CmdArgBuffer& args = packet.getArgBuffer();
    args.resetDeser();

    Fw::CmdStringArg ignoredPath;
    Svc::CmdSequencer_BlockState blockState;
    EXPECT_EQ(args.deserializeTo(ignoredPath), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(args.deserializeTo(blockState), Fw::FW_SERIALIZE_OK);
    return blockState;
}

}  // namespace OBC
