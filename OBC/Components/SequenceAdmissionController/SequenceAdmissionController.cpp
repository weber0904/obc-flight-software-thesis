#include "OBC/Components/SequenceAdmissionController/SequenceAdmissionController.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "Fw/Com/ComBuffer.hpp"
#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/String.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"
#include "OBC/Components/FileIngressAuthority/FileIngressPolicy.hpp"
#include "OBC/Components/SequenceAdmissionController/OfficialSequenceOpcodes.hpp"
#include "Svc/CmdSequencer/CmdSequencer_BlockStateEnumAc.hpp"

namespace OBC {

namespace {

enum class SequenceControlRejectReason : U32 {
    NONE = 0,
    NOT_CONFIGURED = 1,
    INTERNAL_BUSY = 2,
    STAGED_PATH_INVALID = 3,
    INSPECTION_FAILED = 4,
    UNKNOWN_OPCODE = 5,
    AUTHORITY_DENIED = 6,
    ADMIN_OPCODE_FORBIDDEN = 7,
    CONTEXT_NOT_FOUND = 8,
    OWNER_MISMATCH = 9,
    NO_SEQUENCER_AVAILABLE = 10,
    INTERNAL_COMMAND_BUILD_FAILED = 11,
    CONTEXT_TABLE_FULL = 12,
    COMMAND_PATH_TOO_LONG = 13,
    CONTEXT_STATE_INVALID = 14,
};

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

U32 fnv1a32(const std::string& value) {
    U32 hash = 2166136261U;
    for (const unsigned char byte : value) {
        hash ^= static_cast<U32>(byte);
        hash *= 16777619U;
    }
    return hash;
}

std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == '/') {
        return left + right;
    }
    return left + "/" + right;
}

bool pathIsDirectory(const std::string& path) {
    struct stat info = {};
    return ::stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

bool makeDirectories(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path == "/") {
        return true;
    }

    std::string current;
    if (path.front() == '/') {
        current = "/";
    }

    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty()) {
            current = current.empty() || current == "/" ? current + part : current + "/" + part;
            if (::mkdir(current.c_str(), 0775) != 0) {
                if (errno != EEXIST || !pathIsDirectory(current)) {
                    return false;
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return true;
}

bool canonicalizeExistingPath(const std::string& path, std::string& canonicalPath) {
    char resolved[PATH_MAX] = {};
    if (::realpath(path.c_str(), resolved) == nullptr) {
        canonicalPath.clear();
        return false;
    }
    canonicalPath = resolved;
    return true;
}

bool getWorkingDirectory(std::string& workingDirectory) {
    char buffer[PATH_MAX] = {};
    if (::getcwd(buffer, sizeof(buffer)) == nullptr) {
        workingDirectory.clear();
        return false;
    }
    workingDirectory = buffer;
    return true;
}

bool pathExists(const std::string& path) {
    struct stat info = {};
    return ::lstat(path.c_str(), &info) == 0;
}

std::string parentDirectory(const std::string& path) {
    const std::size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0U) {
        return "/";
    }
    return path.substr(0, pos);
}

bool readSymlinkTarget(const std::string& linkPath, std::string& targetPath) {
    char buffer[PATH_MAX] = {};
    const ssize_t bytes = ::readlink(linkPath.c_str(), buffer, sizeof(buffer) - 1);
    if (bytes < 0) {
        targetPath.clear();
        return false;
    }
    buffer[bytes] = '\0';
    targetPath = buffer;
    return true;
}

bool copyFileContents(const std::string& sourcePath, const std::string& destinationPath) {
    std::ifstream source(sourcePath, std::ios::binary);
    if (!source.is_open()) {
        return false;
    }
    std::ofstream destination(destinationPath, std::ios::binary | std::ios::trunc);
    if (!destination.is_open()) {
        return false;
    }
    destination << source.rdbuf();
    destination.flush();
    return destination.good() && (source.good() || source.eof());
}

void removeFileIfPresent(const std::string& path) {
    if (!path.empty()) {
        static_cast<void>(::unlink(path.c_str()));
    }
}

FwOpcodeType sequencerBaseForIndex(U32 index) {
    return index == 0U ? OBC_CMD_SEQ_A_BASE_ID : OBC_CMD_SEQ_B_BASE_ID;
}

bool isForbiddenNestedSequencingOpcode(FwOpcodeType opcode) {
    if (opcode >= OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID && opcode <= OBC_SEQ_LOG_STATUS_OPCODE) {
        return true;
    }
    return opcode == OBC_SEQ_DISPATCHER_RUN_OPCODE || opcode == OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_VALIDATE_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_VALIDATE_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_JOIN_WAIT_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_JOIN_WAIT_OPCODE(OBC_CMD_SEQ_B_BASE_ID);
}

AuthorityConfig configFromRequest(const SequenceControlRequest& request) {
    AuthorityConfig config;
    config.valid = true;
    config.identity = static_cast<AuthorityLinkIdentity>(request.get_linkIdentity());
    config.role = static_cast<AuthorityLinkRole>(request.get_linkRole());
    return config;
}

}  // namespace

SequenceAdmissionController::SequenceAdmissionController(const char* compName)
    : SequenceAdmissionControllerComponentBase(compName) {
    this->tlmWrite_SEQ_CONTEXTS_ACTIVE(0U);
    this->tlmWrite_SEQ_LAST_CONTEXT_ID(0U);
    this->tlmWrite_SEQ_LAST_STATE(SequenceContextState::UNUSED);
    this->tlmWrite_SEQ_LAST_REASON(0U);
    this->tlmWrite_SEQ_LAST_SEQUENCER(NO_SEQUENCER);
    this->tlmWrite_SEQ_REJECT_TOTAL(0U);
}

SequenceAdmissionController::~SequenceAdmissionController() = default;

bool SequenceAdmissionController::configureRuntime(const std::string& runtimeRoot, const std::string& workingRoot) {
    const std::string root = runtimeRoot.empty() ? "runtime" : runtimeRoot;
    const std::string admittedRoot = joinPath(joinPath(root, "sequences"), "admitted");
    std::string resolvedWorkingRoot = workingRoot;
    if (resolvedWorkingRoot.empty() && !getWorkingDirectory(resolvedWorkingRoot)) {
        return false;
    }
    if (!makeDirectories(admittedRoot)) {
        return false;
    }
    if (!canonicalizeExistingPath(root, this->m_runtimeRoot)) {
        return false;
    }
    if (!canonicalizeExistingPath(admittedRoot, this->m_admittedRoot)) {
        return false;
    }

    char aliasName[32] = {};
    std::snprintf(aliasName, sizeof(aliasName), ".adm-%08x", fnv1a32(this->m_admittedRoot));
    const std::string aliasPath = joinPath(resolvedWorkingRoot, aliasName);
    struct stat aliasStatus = {};
    if (::lstat(aliasPath.c_str(), &aliasStatus) == 0) {
        if (!S_ISLNK(aliasStatus.st_mode)) {
            return false;
        }

        std::string targetPath;
        if (!readSymlinkTarget(aliasPath, targetPath)) {
            return false;
        }
        if (!targetPath.empty() && targetPath.front() != '/') {
            targetPath = joinPath(parentDirectory(aliasPath), targetPath);
        }

        std::string canonicalTargetPath;
        if (!canonicalizeExistingPath(targetPath, canonicalTargetPath)) {
            return false;
        }
        if (canonicalTargetPath != this->m_admittedRoot) {
            if (::unlink(aliasPath.c_str()) != 0) {
                return false;
            }
        }
    } else if (errno != ENOENT) {
        return false;
    }

    if (!pathExists(aliasPath) && ::symlink(this->m_admittedRoot.c_str(), aliasPath.c_str()) != 0) {
        return false;
    }

    this->m_admittedAliasPath = aliasPath;
    this->m_admittedCommandPrefix = std::string(aliasName) + "/";
    this->m_runtimeConfigured = true;
    return true;
}

SequenceControlResult SequenceAdmissionController::controlIn_handler(FwIndexType portNum,
                                                                     const SequenceControlRequest& request) {
    static_cast<void>(portNum);
    switch (request.get_operation()) {
        case SequenceControlAction::SEQ_VALIDATE:
            return this->handleValidate(request);
        case SequenceControlAction::SEQ_RUN:
            return this->handleRun(request);
        case SequenceControlAction::SEQ_PREPARE_MANUAL:
            return this->handlePrepareManual(request);
        case SequenceControlAction::SEQ_START:
            return this->handleStartLike(request, InternalOpKind::START);
        case SequenceControlAction::SEQ_STEP:
            return this->handleStartLike(request, InternalOpKind::STEP);
        case SequenceControlAction::SEQ_CANCEL:
            return this->handleCancel(request);
        case SequenceControlAction::SEQ_LOG_STATUS:
            return this->handleLogStatus(request);
    }
    return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_COMMAND_BUILD_FAILED));
}

void SequenceAdmissionController::internalCmdStatusIn_handler(FwIndexType portNum,
                                                              FwOpcodeType opCode,
                                                              U32 cmdSeq,
                                                              const Fw::CmdResponse& response) {
    static_cast<void>(portNum);
    static_cast<void>(opCode);
    static_cast<void>(cmdSeq);
    this->finalizeInternalOperation(response);
}

void SequenceAdmissionController::seqStartIn_handler(FwIndexType portNum, const Fw::StringBase& filename) {
    const ContextEntry* found = this->findContextByAdmittedPath(filename.toChar());
    if (found == nullptr) {
        return;
    }
    const U32 index = static_cast<U32>(found - this->m_contexts);
    this->updateContextState(index, SequenceContextState::RUNNING, portNum);
}

void SequenceAdmissionController::seqDoneIn_handler(FwIndexType portNum,
                                                    FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    const Fw::CmdResponse& response) {
    static_cast<void>(opCode);
    static_cast<void>(cmdSeq);

    for (U32 i = 0U; i < MAX_CONTEXTS; ++i) {
        ContextEntry& context = this->m_contexts[i];
        if (!context.used || context.sequencer != portNum) {
            continue;
        }
        if (context.state.e != SequenceContextState::RUNNING && context.state.e != SequenceContextState::AUTO_DISPATCHED &&
            context.state.e != SequenceContextState::MANUAL_PREPARED) {
            continue;
        }
        if (context.state.e == SequenceContextState::CANCELED) {
            return;
        }
        this->updateContextState(i,
                                 response.e == Fw::CmdResponse::OK ? SequenceContextState::SUCCEEDED
                                                                   : SequenceContextState::FAILED,
                                 portNum);
        if (context.manualModeContext) {
            context.manualModeContext = false;
            this->requestSequencerAutoRestore(portNum);
        }
        return;
    }
}

void SequenceAdmissionController::SEQ_VALIDATE_cmdHandler(FwOpcodeType opCode,
                                                          U32 cmdSeq,
                                                          const Fw::CmdStringArg& fileName) {
    static_cast<void>(fileName);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_RUN_cmdHandler(FwOpcodeType opCode,
                                                     U32 cmdSeq,
                                                     const Fw::CmdStringArg& fileName,
                                                     Fw::Wait waitMode) {
    static_cast<void>(fileName);
    static_cast<void>(waitMode);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_PREPARE_MANUAL_cmdHandler(FwOpcodeType opCode,
                                                                U32 cmdSeq,
                                                                const Fw::CmdStringArg& fileName) {
    static_cast<void>(fileName);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) {
    static_cast<void>(contextId);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_STEP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) {
    static_cast<void>(contextId);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_CANCEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 contextId) {
    static_cast<void>(contextId);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

void SequenceAdmissionController::SEQ_LOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
}

SequenceControlResult SequenceAdmissionController::rejectControl(const SequenceControlRequest& request, U32 reason) {
    this->m_rejectTotal++;
    this->tlmWrite_SEQ_REJECT_TOTAL(this->m_rejectTotal);
    this->tlmWrite_SEQ_LAST_REASON(reason);
    this->log_WARNING_HI_SEQUENCE_CONTROL_REJECTED(request.get_operation(),
                                                   reason,
                                                   request.get_ingressPort(),
                                                   request.get_linkIdentity(),
                                                   request.get_linkRole());

    SequenceControlResult result;
    result.set_accepted(false);
    result.set_deferred(false);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::VALIDATION_ERROR));
    result.set_reason(reason);
    result.set_contextId(0U);
    return result;
}

SequenceControlResult SequenceAdmissionController::handleValidate(const SequenceControlRequest& request) {
    if (!this->m_runtimeConfigured) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::NOT_CONFIGURED));
    }

    U32 dummyIndex = 0U;
    U32 rejectReason = 0U;
    if (!this->inspectAndAdmit(request, dummyIndex, rejectReason)) {
        return this->rejectControl(request, rejectReason);
    }

    SequenceControlResult result;
    result.set_accepted(true);
    result.set_deferred(false);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::OK));
    result.set_reason(0U);
    result.set_contextId(0U);
    return result;
}

SequenceControlResult SequenceAdmissionController::handleRun(const SequenceControlRequest& request) {
    if (this->m_internalOp.active) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_BUSY));
    }

    U32 contextIndex = 0U;
    U32 rejectReason = 0U;
    if (!this->inspectAndAdmit(request, contextIndex, rejectReason)) {
        return this->rejectControl(request, rejectReason);
    }

    ContextEntry& context = this->m_contexts[contextIndex];
    context.block = Fw::Wait(static_cast<Fw::Wait::T>(request.get_waitMode()));
    const U32 predictedSequencer = this->chooseAvailableSequencer();
    this->updateContextState(contextIndex, SequenceContextState::AUTO_DISPATCHED, predictedSequencer);

    this->m_internalOp.active = true;
    this->m_internalOp.kind = InternalOpKind::DISPATCH_RUN;
    this->m_internalOp.contextIndex = contextIndex;
    this->m_internalOp.ingressPort = request.get_ingressPort();
    this->m_internalOp.originalOpcode = request.get_originalOpcode();
    this->m_internalOp.originalCmdSeq = request.get_originalCmdSeq();
    this->m_internalOp.targetSequencer = predictedSequencer;
    if (!this->buildAndSendInternalCommand(InternalOpKind::DISPATCH_RUN, contextIndex, predictedSequencer)) {
        this->releaseContextResources(context);
        this->m_internalOp = InternalOp();
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_COMMAND_BUILD_FAILED));
    }

    SequenceControlResult result;
    result.set_accepted(true);
    result.set_deferred(true);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::OK));
    result.set_reason(0U);
    result.set_contextId(context.id);
    return result;
}

SequenceControlResult SequenceAdmissionController::handlePrepareManual(const SequenceControlRequest& request) {
    if (this->m_internalOp.active) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_BUSY));
    }

    const U32 sequencer = this->chooseAvailableSequencer();
    if (sequencer == NO_SEQUENCER) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::NO_SEQUENCER_AVAILABLE));
    }

    U32 contextIndex = 0U;
    U32 rejectReason = 0U;
    if (!this->inspectAndAdmit(request, contextIndex, rejectReason)) {
        return this->rejectControl(request, rejectReason);
    }

    this->m_contexts[contextIndex].sequencer = sequencer;

    this->m_internalOp.active = true;
    this->m_internalOp.kind = InternalOpKind::SET_MANUAL;
    this->m_internalOp.contextIndex = contextIndex;
    this->m_internalOp.ingressPort = request.get_ingressPort();
    this->m_internalOp.originalOpcode = request.get_originalOpcode();
    this->m_internalOp.originalCmdSeq = request.get_originalCmdSeq();
    this->m_internalOp.targetSequencer = sequencer;
    if (!this->buildAndSendInternalCommand(InternalOpKind::SET_MANUAL, contextIndex, sequencer)) {
        this->releaseContextResources(this->m_contexts[contextIndex]);
        this->m_internalOp = InternalOp();
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_COMMAND_BUILD_FAILED));
    }

    SequenceControlResult result;
    result.set_accepted(true);
    result.set_deferred(true);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::OK));
    result.set_reason(0U);
    result.set_contextId(this->m_contexts[contextIndex].id);
    return result;
}

SequenceControlResult SequenceAdmissionController::handleStartLike(const SequenceControlRequest& request, InternalOpKind kind) {
    if (this->m_internalOp.active) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_BUSY));
    }

    ContextEntry* context = this->findContextById(request.get_contextId());
    if (context == nullptr) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::CONTEXT_NOT_FOUND));
    }
    if (!this->ownerMatches(*context, request)) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::OWNER_MISMATCH));
    }
    if (this->isTerminalState(context->state)) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::CONTEXT_STATE_INVALID));
    }
    if ((kind == InternalOpKind::START || kind == InternalOpKind::STEP) && !context->manualModeContext) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::CONTEXT_STATE_INVALID));
    }
    if (context->sequencer == NO_SEQUENCER) {
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::NO_SEQUENCER_AVAILABLE));
    }

    const U32 contextIndex = static_cast<U32>(context - this->m_contexts);
    this->m_internalOp.active = true;
    this->m_internalOp.kind = kind;
    this->m_internalOp.contextIndex = contextIndex;
    this->m_internalOp.ingressPort = request.get_ingressPort();
    this->m_internalOp.originalOpcode = request.get_originalOpcode();
    this->m_internalOp.originalCmdSeq = request.get_originalCmdSeq();
    this->m_internalOp.targetSequencer = context->sequencer;
    if (!this->buildAndSendInternalCommand(kind, contextIndex, context->sequencer)) {
        this->m_internalOp = InternalOp();
        return this->rejectControl(request, static_cast<U32>(SequenceControlRejectReason::INTERNAL_COMMAND_BUILD_FAILED));
    }

    SequenceControlResult result;
    result.set_accepted(true);
    result.set_deferred(true);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::OK));
    result.set_reason(0U);
    result.set_contextId(context->id);
    return result;
}

SequenceControlResult SequenceAdmissionController::handleCancel(const SequenceControlRequest& request) {
    return this->handleStartLike(request, InternalOpKind::CANCEL);
}

SequenceControlResult SequenceAdmissionController::handleLogStatus(const SequenceControlRequest& request) {
    static_cast<void>(request);
    this->logContexts();
    SequenceControlResult result;
    result.set_accepted(true);
    result.set_deferred(false);
    result.set_cmdResponse(static_cast<U32>(Fw::CmdResponse::OK));
    result.set_reason(0U);
    result.set_contextId(0U);
    return result;
}

bool SequenceAdmissionController::inspectAndAdmit(const SequenceControlRequest& request,
                                                  U32& contextIndex,
                                                  U32& rejectReason) {
    contextIndex = 0U;
    rejectReason = static_cast<U32>(SequenceControlRejectReason::NONE);
    if (!this->m_runtimeConfigured) {
        rejectReason = static_cast<U32>(SequenceControlRejectReason::NOT_CONFIGURED);
        return false;
    }

    const std::string requestedPath = request.get_fileName().toChar();
    if (!startsWith(requestedPath, FileIngressPolicy::LOGICAL_PREFIX)) {
        rejectReason = static_cast<U32>(SequenceControlRejectReason::STAGED_PATH_INVALID);
        return false;
    }

    const std::string leaf = requestedPath.substr(std::strlen(FileIngressPolicy::LOGICAL_PREFIX));
    if (leaf.empty() || leaf.find('/') != std::string::npos || leaf.find('\\') != std::string::npos) {
        rejectReason = static_cast<U32>(SequenceControlRejectReason::STAGED_PATH_INVALID);
        return false;
    }

    const std::string stagedPath = joinPath(joinPath(joinPath(this->m_runtimeRoot, "sequences"), "staging"), leaf);
    const Fw::Time validTime = this->getTime();
    SequenceInspectionResult inspection = this->m_inspector.inspect(stagedPath, validTime);
    if (!inspection.valid) {
        rejectReason = static_cast<U32>(SequenceControlRejectReason::INSPECTION_FAILED);
        return false;
    }

    const AuthorityConfig config = configFromRequest(request);
    for (const SequenceRecordInfo& record : inspection.records) {
        if (isForbiddenNestedSequencingOpcode(static_cast<FwOpcodeType>(record.opcode))) {
            rejectReason = static_cast<U32>(SequenceControlRejectReason::ADMIN_OPCODE_FORBIDDEN);
            return false;
        }
        const CommandAuthorityCatalogEntry* entry = findCommandAuthorityCatalogEntry(static_cast<FwOpcodeType>(record.opcode));
        if (entry == nullptr) {
            rejectReason = static_cast<U32>(SequenceControlRejectReason::UNKNOWN_OPCODE);
            return false;
        }
        const AuthorityDecision decision = evaluateCommandAuthority(config, static_cast<FwOpcodeType>(record.opcode));
        if (!decision.allow) {
            rejectReason = static_cast<U32>(SequenceControlRejectReason::AUTHORITY_DENIED);
            return false;
        }
    }

    if (request.get_operation() == SequenceControlAction::SEQ_VALIDATE) {
        return true;
    }

    contextIndex = this->allocateContextSlot();
    if (contextIndex == MAX_CONTEXTS) {
        rejectReason = static_cast<U32>(SequenceControlRejectReason::CONTEXT_TABLE_FULL);
        return false;
    }

    ContextEntry& context = this->m_contexts[contextIndex];
    context = ContextEntry();
    context.used = true;
    context.id = this->m_nextContextId++;
    context.state = SequenceContextState::ADMITTED;
    context.identity = static_cast<AuthorityLinkIdentity>(request.get_linkIdentity());
    context.role = static_cast<AuthorityLinkRole>(request.get_linkRole());
    context.ingressPort = request.get_ingressPort();
    context.sourcePath = stagedPath;
    context.admittedPhysicalPath = this->admittedPhysicalPathForContext(context.id, leaf);
    context.admittedCommandPath = this->admittedCommandPathForContext(context.id, leaf);
    context.crc = inspection.crc;
    context.block = Fw::Wait(static_cast<Fw::Wait::T>(request.get_waitMode()));
    context.manualModeContext = request.get_operation() == SequenceControlAction::SEQ_PREPARE_MANUAL;

    if (context.admittedCommandPath.size() >= Fw::CmdStringArg::STRING_SIZE) {
        this->releaseContextResources(context);
        rejectReason = static_cast<U32>(SequenceControlRejectReason::COMMAND_PATH_TOO_LONG);
        return false;
    }

    if (!copyFileContents(context.sourcePath, context.admittedPhysicalPath)) {
        this->releaseContextResources(context);
        rejectReason = static_cast<U32>(SequenceControlRejectReason::INSPECTION_FAILED);
        return false;
    }

    const SequenceInspectionResult admittedInspection = this->m_inspector.inspect(context.admittedPhysicalPath, validTime);
    if (!admittedInspection.valid || admittedInspection.crc != inspection.crc) {
        this->releaseContextResources(context);
        rejectReason = static_cast<U32>(SequenceControlRejectReason::INSPECTION_FAILED);
        return false;
    }

    this->updateContextState(contextIndex, SequenceContextState::ADMITTED, context.sequencer);
    return true;
}

bool SequenceAdmissionController::buildAndSendInternalCommand(InternalOpKind kind,
                                                              U32 contextIndex,
                                                              U32 targetSequencer) {
    if (kind != InternalOpKind::SET_AUTO && (contextIndex >= MAX_CONTEXTS || !this->m_contexts[contextIndex].used)) {
        return false;
    }

    const ContextEntry* context = kind == InternalOpKind::SET_AUTO ? nullptr : &this->m_contexts[contextIndex];
    Fw::ComBuffer packet;
    packet.resetSer();
    if (packet.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)) !=
        Fw::FW_SERIALIZE_OK) {
        return false;
    }

    switch (kind) {
        case InternalOpKind::DISPATCH_RUN:
            if (packet.serializeFrom(OBC_SEQ_DISPATCHER_RUN_OPCODE) != Fw::FW_SERIALIZE_OK ||
                packet.serializeFrom(Fw::CmdStringArg(context->admittedCommandPath.c_str())) != Fw::FW_SERIALIZE_OK ||
                packet.serializeFrom(context->block) != Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::SET_MANUAL:
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_MANUAL_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::LOAD_MANUAL:
            {
            const Svc::CmdSequencer_BlockState blockState(Svc::CmdSequencer_BlockState::NO_BLOCK);
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_RUN_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                    Fw::FW_SERIALIZE_OK ||
                packet.serializeFrom(Fw::CmdStringArg(context->admittedCommandPath.c_str())) != Fw::FW_SERIALIZE_OK ||
                packet.serializeFrom(blockState) != Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
            }
        case InternalOpKind::SET_AUTO:
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_AUTO_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::START:
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_START_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::STEP:
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_STEP_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::CANCEL:
            if (packet.serializeFrom(OBC_CMD_SEQ_CS_CANCEL_OPCODE(sequencerBaseForIndex(targetSequencer))) !=
                Fw::FW_SERIALIZE_OK) {
                return false;
            }
            break;
        case InternalOpKind::NONE:
            return false;
    }

    this->internalCmdOut_out(0, packet, 0U);
    return true;
}

void SequenceAdmissionController::finalizeInternalOperation(const Fw::CmdResponse& response) {
    if (!this->m_internalOp.active) {
        return;
    }

    const InternalOp op = this->m_internalOp;
    if (response.e != Fw::CmdResponse::OK) {
        if (op.publishStatus && op.contextIndex < MAX_CONTEXTS && this->m_contexts[op.contextIndex].used) {
            const U32 failedSequencer =
                op.targetSequencer == NO_SEQUENCER ? this->m_contexts[op.contextIndex].sequencer : op.targetSequencer;
            this->updateContextState(op.contextIndex, SequenceContextState::FAILED, failedSequencer);
        }
        bool shouldRestoreAuto = false;
        if (op.targetSequencer < SEQUENCER_COUNT && this->m_sequencerManualMode[op.targetSequencer]) {
            if (op.kind == InternalOpKind::LOAD_MANUAL || op.kind == InternalOpKind::CANCEL) {
                if (op.contextIndex < MAX_CONTEXTS && this->m_contexts[op.contextIndex].used) {
                    this->m_contexts[op.contextIndex].manualModeContext = false;
                }
                shouldRestoreAuto = true;
            } else if (op.kind == InternalOpKind::START || op.kind == InternalOpKind::STEP) {
                if (op.contextIndex < MAX_CONTEXTS && this->m_contexts[op.contextIndex].used &&
                    this->m_contexts[op.contextIndex].manualModeContext) {
                    this->m_contexts[op.contextIndex].manualModeContext = false;
                    shouldRestoreAuto = true;
                }
            } else if (op.kind == InternalOpKind::SET_AUTO) {
                shouldRestoreAuto = true;
            }
        }
        if (shouldRestoreAuto) {
            this->requestSequencerAutoRestore(op.targetSequencer);
        }
        if (op.publishStatus) {
            this->publishControlStatus(op, response);
        }
        this->m_internalOp = InternalOp();
        this->processPendingAutoRestore();
        return;
    }

    switch (op.kind) {
        case InternalOpKind::SET_MANUAL:
            if (op.targetSequencer < SEQUENCER_COUNT) {
                this->m_sequencerManualMode[op.targetSequencer] = true;
            }
            this->m_internalOp.kind = InternalOpKind::LOAD_MANUAL;
            if (!this->buildAndSendInternalCommand(InternalOpKind::LOAD_MANUAL, op.contextIndex, op.targetSequencer)) {
                this->requestSequencerAutoRestore(op.targetSequencer);
                this->publishControlStatus(op, Fw::CmdResponse::EXECUTION_ERROR);
                this->m_internalOp = InternalOp();
                this->processPendingAutoRestore();
            }
            return;
        case InternalOpKind::LOAD_MANUAL:
            this->updateContextState(op.contextIndex, SequenceContextState::MANUAL_PREPARED, op.targetSequencer);
            break;
        case InternalOpKind::SET_AUTO:
            if (op.targetSequencer < SEQUENCER_COUNT) {
                this->m_sequencerManualMode[op.targetSequencer] = false;
            }
            break;
        case InternalOpKind::START:
            this->updateContextState(op.contextIndex, SequenceContextState::RUNNING, op.targetSequencer);
            break;
        case InternalOpKind::STEP:
            break;
        case InternalOpKind::CANCEL:
            this->updateContextState(op.contextIndex, SequenceContextState::CANCELED, op.targetSequencer);
            if (this->m_contexts[op.contextIndex].manualModeContext) {
                this->m_contexts[op.contextIndex].manualModeContext = false;
                this->requestSequencerAutoRestore(op.targetSequencer);
            }
            break;
        case InternalOpKind::DISPATCH_RUN:
        case InternalOpKind::NONE:
            break;
    }

    if (op.publishStatus) {
        this->publishControlStatus(op, response);
    }
    this->m_internalOp = InternalOp();
    this->processPendingAutoRestore();
}

void SequenceAdmissionController::publishControlStatus(const InternalOp& op, const Fw::CmdResponse& response) {
    SequenceControlStatus status;
    status.set_ingressPort(op.ingressPort);
    status.set_originalOpcode(op.originalOpcode);
    status.set_originalCmdSeq(op.originalCmdSeq);
    status.set_cmdResponse(static_cast<U32>(response.e));
    this->controlStatusOut_out(0, status);
}

void SequenceAdmissionController::updateContextState(U32 contextIndex, SequenceContextState state, U32 sequencer) {
    FW_ASSERT(contextIndex < MAX_CONTEXTS);
    ContextEntry& context = this->m_contexts[contextIndex];
    context.state = state;
    if (sequencer != NO_SEQUENCER) {
        context.sequencer = sequencer;
    }
    this->tlmWrite_SEQ_CONTEXTS_ACTIVE(this->countActiveContexts());
    this->tlmWrite_SEQ_LAST_CONTEXT_ID(context.id);
    this->tlmWrite_SEQ_LAST_STATE(state);
    this->tlmWrite_SEQ_LAST_SEQUENCER(context.sequencer);
    this->log_ACTIVITY_HI_SEQUENCE_CONTEXT_UPDATED(
        context.id, state, context.sequencer, static_cast<U32>(context.identity), static_cast<U32>(context.role));
}

void SequenceAdmissionController::logContexts() {
    for (const ContextEntry& context : this->m_contexts) {
        if (!context.used) {
            continue;
        }
        this->log_ACTIVITY_LO_SEQUENCE_STATUS_LOG(context.id,
                                                  context.state,
                                                  context.sequencer,
                                                  static_cast<U32>(context.identity),
                                                  static_cast<U32>(context.role),
                                                  context.block,
                                                  Fw::String(context.admittedCommandPath.c_str()));
    }
}

bool SequenceAdmissionController::requestSequencerAutoRestore(U32 sequencer) {
    if (sequencer >= SEQUENCER_COUNT || !this->m_sequencerManualMode[sequencer]) {
        return true;
    }

    if (this->m_internalOp.active) {
        this->m_pendingAutoRestore[sequencer] = true;
        return true;
    }

    this->m_pendingAutoRestore[sequencer] = false;
    this->m_internalOp = InternalOp();
    this->m_internalOp.active = true;
    this->m_internalOp.kind = InternalOpKind::SET_AUTO;
    this->m_internalOp.publishStatus = false;
    this->m_internalOp.targetSequencer = sequencer;
    if (!this->buildAndSendInternalCommand(InternalOpKind::SET_AUTO, 0U, sequencer)) {
        this->m_internalOp = InternalOp();
        this->m_pendingAutoRestore[sequencer] = true;
        return false;
    }
    return true;
}

void SequenceAdmissionController::processPendingAutoRestore() {
    if (this->m_internalOp.active) {
        return;
    }

    for (U32 sequencer = 0U; sequencer < SEQUENCER_COUNT; ++sequencer) {
        if (!this->m_pendingAutoRestore[sequencer] || !this->m_sequencerManualMode[sequencer]) {
            continue;
        }
        this->requestSequencerAutoRestore(sequencer);
        return;
    }
}

SequenceAdmissionController::ContextEntry* SequenceAdmissionController::findContextById(U32 contextId) {
    for (ContextEntry& context : this->m_contexts) {
        if (context.used && context.id == contextId) {
            return &context;
        }
    }
    return nullptr;
}

const SequenceAdmissionController::ContextEntry* SequenceAdmissionController::findContextByAdmittedPath(
    const std::string& path) const {
    for (const ContextEntry& context : this->m_contexts) {
        if (context.used && context.admittedCommandPath == path) {
            return &context;
        }
    }
    return nullptr;
}

U32 SequenceAdmissionController::countActiveContexts() const {
    U32 count = 0U;
    for (const ContextEntry& context : this->m_contexts) {
        if (!context.used) {
            continue;
        }
        if (this->isTerminalState(context.state)) {
            continue;
        }
        count++;
    }
    return count;
}

U32 SequenceAdmissionController::allocateContextSlot() {
    for (U32 i = 0U; i < MAX_CONTEXTS; ++i) {
        if (!this->m_contexts[i].used) {
            return i;
        }
    }
    for (U32 i = 0U; i < MAX_CONTEXTS; ++i) {
        if (this->m_contexts[i].used && this->isTerminalState(this->m_contexts[i].state)) {
            this->releaseContextResources(this->m_contexts[i]);
            return i;
        }
    }
    return MAX_CONTEXTS;
}

U32 SequenceAdmissionController::chooseAvailableSequencer() const {
    for (U32 candidate = 0U; candidate < SEQUENCER_COUNT; ++candidate) {
        if (this->m_sequencerManualMode[candidate] || this->m_pendingAutoRestore[candidate]) {
            continue;
        }
        bool busy = false;
        for (const ContextEntry& context : this->m_contexts) {
            if (!context.used || context.sequencer != candidate) {
                continue;
            }
            if (context.state.e == SequenceContextState::MANUAL_PREPARED || context.state.e == SequenceContextState::RUNNING ||
                context.state.e == SequenceContextState::AUTO_DISPATCHED || context.state.e == SequenceContextState::ADMITTED) {
                busy = true;
                break;
            }
        }
        if (!busy) {
            return candidate;
        }
    }
    return NO_SEQUENCER;
}

bool SequenceAdmissionController::ownerMatches(const ContextEntry& context, const SequenceControlRequest& request) const {
    return context.ingressPort == request.get_ingressPort() &&
           context.identity == static_cast<AuthorityLinkIdentity>(request.get_linkIdentity()) &&
           context.role == static_cast<AuthorityLinkRole>(request.get_linkRole());
}

void SequenceAdmissionController::releaseContextResources(ContextEntry& context) {
    removeFileIfPresent(context.admittedPhysicalPath);
    context = ContextEntry();
}

bool SequenceAdmissionController::isTerminalState(SequenceContextState state) const {
    return state.e == SequenceContextState::SUCCEEDED || state.e == SequenceContextState::FAILED ||
           state.e == SequenceContextState::CANCELED;
}

std::string SequenceAdmissionController::admittedPhysicalPathForContext(U32 contextId, const std::string& sourcePath) const {
    std::ostringstream stream;
    stream << this->m_admittedRoot << "/ctx-" << contextId << "-" << sourcePath;
    return stream.str();
}

std::string SequenceAdmissionController::admittedCommandPathForContext(U32 contextId, const std::string& sourcePath) const {
    std::ostringstream stream;
    stream << this->m_admittedCommandPrefix << "ctx-" << contextId << "-" << sourcePath;
    return stream.str();
}

}  // namespace OBC
