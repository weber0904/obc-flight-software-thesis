#ifndef OBC_SEQUENCEADMISSIONCONTROLLER_TESTER_HPP
#define OBC_SEQUENCEADMISSIONCONTROLLER_TESTER_HPP

#include <vector>

#include "Svc/CmdSequencer/CmdSequencer_BlockStateEnumAc.hpp"
#include "OBC/Components/SequenceAdmissionController/OfficialSequenceOpcodes.hpp"
#include "OBC/Components/SequenceAdmissionController/SequenceAdmissionController.hpp"
#include "OBC/Components/SequenceAdmissionController/SequenceAdmissionControllerGTestBase.hpp"
#include "OBC/Components/test/TestSupport.hpp"

namespace OBC {

class SequenceAdmissionControllerTester final : public SequenceAdmissionControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct InternalCommand {
        FwIndexType portNum;
        Fw::ComBuffer data;
        U32 context;
    };

    struct ControlStatus {
        FwIndexType portNum;
        SequenceControlStatus status;
    };

    SequenceAdmissionControllerTester();
    ~SequenceAdmissionControllerTester() override;

    void testValidateAcceptsBackupReadableSequence();
    void testRunRejectsBackupHighAuthoritySequence();
    void testPrepareManualUsesAdmittedCopyAndAllowsOwnedStart();
    void testManualCompletionRestoresAutoMode();
    void testManualCancelRestoresAutoMode();
    void testManualLoadFailureRestoresAutoMode();
    void testAutoRestoreFailureIsRetried();
    void testAdmissionCopyFailureCleansUpAdmittedPath();
    void testOwnerMismatchRejectsControl();
    void testStartAndStepRejectAutoRunContext();
    void testManualStartAndStepFailuresRestoreAutoMode();
    void testRunLifecycleUpdatesFailureState();
    void testRunFailureBeforeSeqStartUsesPredictedSequencer();
    void testTerminalContextsRejectFurtherControl();
    void testTerminalContextsAreReused();

  private:
    void connectPorts();
    void initComponents();
    void clearCaptures();

    void from_internalCmdOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;
    void from_controlStatusOut_handler(FwIndexType portNum, const SequenceControlStatus& controlStatusArg) override;

    SequenceControlRequest makeRequest(SequenceControlAction action,
                                       const char* fileName,
                                       AuthorityLinkIdentity identity,
                                       AuthorityLinkRole role,
                                       U32 contextId = 0U,
                                       Fw::Wait waitMode = Fw::Wait::NO_WAIT) const;

    std::string stageSequenceFile(const char* leafName, FwOpcodeType opcode);
    std::string decodeCommandPath(const Fw::ComBuffer& buffer) const;
    FwOpcodeType decodeCommandOpcode(const Fw::ComBuffer& buffer) const;
    Svc::CmdSequencer_BlockState decodeCommandBlockState(const Fw::ComBuffer& buffer) const;

  private:
    static constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
    static constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;

    TestSupport::TempDirectory m_runtimeRoot;
    SequenceAdmissionController component;
    std::vector<InternalCommand> m_internalCommands;
    std::vector<ControlStatus> m_controlStatuses;
};

}  // namespace OBC

#endif
