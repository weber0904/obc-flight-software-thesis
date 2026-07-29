#ifndef OBC_SEQUENCE_ADMISSION_CONTROLLER_HPP
#define OBC_SEQUENCE_ADMISSION_CONTROLLER_HPP

#include <string>

#include "Fw/Cmd/CmdString.hpp"
#include "Fw/Time/Time.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/SequenceAdmissionController/SequenceAdmissionControllerComponentAc.hpp"
#include "OBC/Components/SequenceAdmissionController/SequenceFileInspector.hpp"

namespace OBC {

class SequenceAdmissionController final : public SequenceAdmissionControllerComponentBase {
  public:
    explicit SequenceAdmissionController(const char* compName);
    ~SequenceAdmissionController() override;

    bool configureRuntime(const std::string& runtimeRoot, const std::string& workingRoot);

  private:
    static constexpr U32 MAX_CONTEXTS = 8U;
    static constexpr U32 NO_SEQUENCER = 0xFFFFFFFFU;
    static constexpr U32 SEQUENCER_COUNT =
        static_cast<U32>(SequenceAdmissionControllerComponentBase::getNum_seqStartIn_InputPorts());

    enum class InternalOpKind : U8 {
        NONE = 0,
        DISPATCH_RUN = 1,
        SET_MANUAL = 2,
        LOAD_MANUAL = 3,
        SET_AUTO = 4,
        START = 5,
        STEP = 6,
        CANCEL = 7,
    };

    struct ContextEntry {
        bool used = false;
        U32 id = 0U;
        SequenceContextState state = SequenceContextState::UNUSED;
        AuthorityLinkIdentity identity = AuthorityLinkIdentity::UNKNOWN;
        AuthorityLinkRole role = AuthorityLinkRole::UNKNOWN;
        U32 ingressPort = 0U;
        U32 sequencer = NO_SEQUENCER;
        Fw::Wait block = Fw::Wait::NO_WAIT;
        std::string sourcePath;
        std::string admittedPhysicalPath;
        std::string admittedCommandPath;
        U32 crc = 0U;
        bool manualModeContext = false;
    };

    struct InternalOp {
        InternalOpKind kind = InternalOpKind::NONE;
        bool active = false;
        bool publishStatus = true;
        U32 contextIndex = 0U;
        U32 ingressPort = 0U;
        U32 originalOpcode = 0U;
        U32 originalCmdSeq = 0U;
        U32 targetSequencer = NO_SEQUENCER;
    };

  private:
    SequenceControlResult controlIn_handler(FwIndexType portNum, const SequenceControlRequest& request) override;
    void internalCmdStatusIn_handler(FwIndexType portNum,
                                     FwOpcodeType opCode,
                                     U32 cmdSeq,
                                     const Fw::CmdResponse& response) override;
    void seqStartIn_handler(FwIndexType portNum, const Fw::StringBase& filename) override;
    void seqDoneIn_handler(FwIndexType portNum,
                           FwOpcodeType opCode,
                           U32 cmdSeq,
                           const Fw::CmdResponse& response) override;

    void SEQ_VALIDATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& fileName) override;
    void SEQ_RUN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& fileName, Fw::Wait block) override;
    void SEQ_PREPARE_MANUAL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& fileName) override;
    void SEQ_START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) override;
    void SEQ_STEP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) override;
    void SEQ_CANCEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) override;
    void SEQ_LOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

  private:
    SequenceControlResult rejectControl(const SequenceControlRequest& request, U32 reason);
    SequenceControlResult handleValidate(const SequenceControlRequest& request);
    SequenceControlResult handleRun(const SequenceControlRequest& request);
    SequenceControlResult handlePrepareManual(const SequenceControlRequest& request);
    SequenceControlResult handleStartLike(const SequenceControlRequest& request, InternalOpKind kind);
    SequenceControlResult handleCancel(const SequenceControlRequest& request);
    SequenceControlResult handleLogStatus(const SequenceControlRequest& request);

    bool inspectAndAdmit(const SequenceControlRequest& request, U32& contextIndex, U32& rejectReason);
    bool buildAndSendInternalCommand(InternalOpKind kind, U32 contextIndex, U32 targetSequencer);
    void finalizeInternalOperation(const Fw::CmdResponse& response);
    void publishControlStatus(const InternalOp& op, const Fw::CmdResponse& response);
    void updateContextState(U32 contextIndex, SequenceContextState state, U32 sequencer);
    void logContexts();
    bool requestSequencerAutoRestore(U32 sequencer);
    void processPendingAutoRestore();
    void releaseContextResources(ContextEntry& context);
    bool isTerminalState(SequenceContextState state) const;

    ContextEntry* findContextById(U32 contextId);
    const ContextEntry* findContextByAdmittedPath(const std::string& path) const;
    U32 countActiveContexts() const;
    U32 allocateContextSlot();
    U32 chooseAvailableSequencer() const;
    bool ownerMatches(const ContextEntry& context, const SequenceControlRequest& request) const;
    std::string admittedPhysicalPathForContext(U32 contextId, const std::string& sourcePath) const;
    std::string admittedCommandPathForContext(U32 contextId, const std::string& sourcePath) const;

  private:
    SequenceFileInspector m_inspector;
    std::string m_runtimeRoot;
    std::string m_admittedRoot;
    std::string m_admittedAliasPath;
    std::string m_admittedCommandPrefix;
    bool m_runtimeConfigured = false;
    U32 m_nextContextId = 1U;
    U32 m_rejectTotal = 0U;
    ContextEntry m_contexts[MAX_CONTEXTS];
    InternalOp m_internalOp;
    bool m_sequencerManualMode[SEQUENCER_COUNT] = {};
    bool m_pendingAutoRestore[SEQUENCER_COUNT] = {};
};

}  // namespace OBC

#endif
