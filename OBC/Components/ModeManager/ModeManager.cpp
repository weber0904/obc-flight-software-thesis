#include "OBC/Components/ModeManager/ModeManager.hpp"

namespace OBC {

ModeManager::ModeManager(const char* const compName)
    : ModeManagerComponentBase(compName),
      m_bootAnnounced(false),
      m_useManualUptime(false),
      m_currentMode(OBC::SatMode::SAFE),
      m_rebootCount(1),
      m_manualUptimeSec(0),
      m_startTime(std::chrono::steady_clock::now()),
      m_operatorTransitionGuard(nullptr) {}

ModeManager::~ModeManager() = default;

void ModeManager::announceBoot() {
    if (this->m_bootAnnounced) {
        return;
    }

    this->m_bootAnnounced = true;
    this->log_ACTIVITY_HI_SYS_BOOT();
    this->publishState_();
}

void ModeManager::configureOperatorTransitionGuard(const OBC::IModeOperatorTransitionGuard* guard) {
    this->m_operatorTransitionGuard = guard;
}

Fw::CmdResponse ModeManager::requestModeFromOperator(OBC::SatMode mode) {
    this->announceBoot();

    const OBC::SatMode fromMode = this->m_currentMode;
    if (this->m_operatorTransitionGuard == nullptr) {
        this->rejectTransition_(fromMode, mode, OBC::ModeTransitionRejectionReason::GUARD_UNCONFIGURED);
        this->publishState_();
        return Fw::CmdResponse::EXECUTION_ERROR;
    }

    const OBC::OperatorModeTransitionDecision decision =
        this->m_operatorTransitionGuard->evaluateOperatorTransition(fromMode, mode);
    if (!decision.accepted) {
        this->rejectTransition_(fromMode, mode, decision.reason);
        this->publishState_();
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    this->applyMode_(mode);
    this->publishState_();
    return Fw::CmdResponse::OK;
}

void ModeManager::applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) {
    static_cast<void>(source);
    this->announceBoot();
    this->applyMode_(mode);
    this->publishState_();
}

void ModeManager::applyMode_(OBC::SatMode mode) {
    if (this->m_currentMode != mode) {
        this->m_currentMode = mode;
        this->log_ACTIVITY_HI_SYS_MODE_CHANGE(mode);
    }
}

void ModeManager::rejectTransition_(OBC::SatMode fromMode,
                                    OBC::SatMode toMode,
                                    OBC::ModeTransitionRejectionReason reason) {
    this->log_WARNING_HI_SYS_MODE_TRANSITION_REJECTED(fromMode, toMode, static_cast<U32>(reason));
}

void ModeManager::setUptimeForTest(U32 uptimeSec) {
    this->m_useManualUptime = true;
    this->m_manualUptimeSec = uptimeSec;
}

OBC::SatMode ModeManager::getModeForRuntime() const {
    return this->m_currentMode;
}

U16 ModeManager::getRebootCountForRuntime() const {
    return this->m_rebootCount;
}

U32 ModeManager::getUptimeForRuntime() const {
    return this->uptimeSec_();
}

void ModeManager::MODE_SET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::SatMode mode) {
    this->cmdResponse_out(opCode, cmdSeq, this->requestModeFromOperator(mode));
}

void ModeManager::MODE_GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->announceBoot();
    this->publishStateForModeGet_();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void ModeManager::publishState_() {
    this->tlmWrite_SYS_MODE(this->m_currentMode);
    this->tlmWrite_SYS_UPTIME_SEC(this->uptimeSec_());
    this->tlmWrite_SYS_REBOOT_COUNT(this->m_rebootCount);
}

void ModeManager::publishStateForModeGet_() {
    Fw::Time timeTag = this->getTime();
    this->emitModeGetTelemetryValue_(CHANNELID_SYS_MODE, this->m_currentMode, timeTag);
    this->emitModeGetTelemetryValue_(CHANNELID_SYS_UPTIME_SEC, this->uptimeSec_(), timeTag);
    this->emitModeGetTelemetryValue_(CHANNELID_SYS_REBOOT_COUNT, this->m_rebootCount, timeTag);
}

void ModeManager::emitModeGetTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag) {
    this->modeGetTlmOut_out(0, this->getIdBase() + channelId, timeTag, buffer);
}

U32 ModeManager::uptimeSec_() const {
    if (this->m_useManualUptime) {
        return this->m_manualUptimeSec;
    }

    const auto elapsed = std::chrono::steady_clock::now() - this->m_startTime;
    return static_cast<U32>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
}

}  // namespace OBC
