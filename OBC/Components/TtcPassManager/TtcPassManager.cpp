#include "OBC/Components/TtcPassManager/TtcPassManager.hpp"

namespace OBC {

TtcPassManager::TtcPassManager(const char* const compName)
    : TtcPassManagerComponentBase(compName),
      m_mutex(),
      m_modeControl(nullptr),
      m_gpsProvider(nullptr),
      m_commProvider(nullptr),
      m_adcsControl(nullptr),
      m_config(),
      m_window(),
      m_suppressedWindow(),
      m_gpsFreshness(),
      m_commLoss(),
      m_status() {}

TtcPassManager::~TtcPassManager() = default;

void TtcPassManager::configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                                      const OBC::ITtcPassGpsProvider* gpsProvider,
                                      const OBC::ITtcPassCommStateProvider* commProvider,
                                      OBC::ITtcPassAdcsControl* adcsControl) {
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_modeControl = modeControl;
        this->m_gpsProvider = gpsProvider;
        this->m_commProvider = commProvider;
        this->m_adcsControl = adcsControl;
    }
}

Fw::CmdResponse TtcPassManager::setPolicyForRuntime(const bool enabled, const U32 lossOfLockTimeoutSec) {
    if (enabled && lossOfLockTimeoutSec == 0U) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_config.enabled = enabled;
        this->m_config.lossOfLockTimeoutSec = lossOfLockTimeoutSec;
        this->m_status.enabled = enabled;
        this->m_status.lossOfLockTimeoutSec = lossOfLockTimeoutSec;
    }

    this->log_ACTIVITY_LO_TTC_POLICY_CONFIG_UPDATED(enabled ? 1U : 0U, lossOfLockTimeoutSec);
    this->publishExplicitRefreshStatus_(this->getStatusForRuntime());
    return Fw::CmdResponse::OK;
}

Fw::CmdResponse TtcPassManager::setPassWindowForRuntime(const U64 startUnixSec, const U64 endUnixSec) {
    const OBC::TtcPassWindow requested{startUnixSec, endUnixSec};
    if (!OBC::TtcPassPolicy::isValidPassWindow(requested)) {
        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            clearWindow_(this->m_window);
            this->m_status.windowConfigured = false;
            this->m_status.windowStartUnixSec = 0U;
            this->m_status.windowEndUnixSec = 0U;
            this->m_status.windowActive = false;
        }
    this->log_WARNING_HI_TTC_PASS_WINDOW_REJECTED(startUnixSec, endUnixSec);
    this->publishExplicitRefreshStatus_(this->getStatusForRuntime());
    return Fw::CmdResponse::VALIDATION_ERROR;
}

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (!windowsEqual_(requested, this->m_suppressedWindow)) {
            clearWindow_(this->m_suppressedWindow);
        }
        this->m_window = requested;
        this->m_status.windowConfigured = true;
        this->m_status.windowStartUnixSec = startUnixSec;
        this->m_status.windowEndUnixSec = endUnixSec;
    }

    this->log_ACTIVITY_LO_TTC_PASS_WINDOW_SET(startUnixSec, endUnixSec);
    this->publishExplicitRefreshStatus_(this->getStatusForRuntime());
    return Fw::CmdResponse::OK;
}

void TtcPassManager::clearPassWindowForRuntime() {
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        clearWindow_(this->m_window);
        clearWindow_(this->m_suppressedWindow);
        this->m_status.windowConfigured = false;
        this->m_status.windowStartUnixSec = 0U;
        this->m_status.windowEndUnixSec = 0U;
        this->m_status.windowActive = false;
    }

    this->log_ACTIVITY_LO_TTC_PASS_WINDOW_CLEARED();
    this->publishExplicitRefreshStatus_(this->getStatusForRuntime());
}

OBC::TtcPassRuntimeStatus TtcPassManager::getStatusForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_status;
}

void TtcPassManager::tickForTest() {
    this->schedIn_handler(0, 0U);
}

void TtcPassManager::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    const OBC::TtcPassRuntimeStatus previousStatus = this->getStatusForRuntime();

    const U32 wallclockSec = this->getTime().getSeconds();
    OBC::IModeSafetyModeControl* modeControl = nullptr;
    const OBC::ITtcPassGpsProvider* gpsProvider = nullptr;
    const OBC::ITtcPassCommStateProvider* commProvider = nullptr;
    OBC::ITtcPassAdcsControl* adcsControl = nullptr;
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        modeControl = this->m_modeControl;
        gpsProvider = this->m_gpsProvider;
        commProvider = this->m_commProvider;
        adcsControl = this->m_adcsControl;
    }

    OBC::SatMode currentMode = OBC::SatMode::SAFE;
    if (modeControl != nullptr) {
        currentMode = modeControl->getModeForRuntime();
    }

    OBC::GPS::StateData gpsState = {};
    const bool haveGps = gpsProvider != nullptr && gpsProvider->getCachedStateForRuntime(gpsState);

    OBC::CommRuntimeState commState = {};
    const bool haveComm = commProvider != nullptr;
    if (haveComm) {
        commState = commProvider->getStateForRuntime();
    }

    bool requestEntry = false;
    bool requestExit = false;
    OBC::TtcPassPolicyReason transitionReason = OBC::TtcPassPolicyReason::NONE;

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        const bool previouslyTtcActive = this->m_status.ttcActive;
        this->m_status.currentMode = currentMode;
        this->m_status.enabled = this->m_config.enabled;
        this->m_status.lossOfLockTimeoutSec = this->m_config.lossOfLockTimeoutSec;
        this->m_status.windowConfigured = OBC::TtcPassPolicy::isValidPassWindow(this->m_window);
        this->m_status.windowStartUnixSec = this->m_status.windowConfigured ? this->m_window.startUnixSec : 0U;
        this->m_status.windowEndUnixSec = this->m_status.windowConfigured ? this->m_window.endUnixSec : 0U;

        U64 currentGpsUnixSec = 0U;
        bool gpsTimeValid = false;
        if (haveGps) {
            OBC::TtcPassPolicy::updateGpsFreshnessTracker(
                this->m_gpsFreshness, gpsState.acceptedSentenceCount, wallclockSec);
            gpsTimeValid = gpsState.hasSample && gpsState.fixValid && gpsState.utcDateYmd != 0U &&
                           OBC::TtcPassPolicy::isGpsFresh(
                               this->m_gpsFreshness, wallclockSec, OBC::TtcPassPolicy::GPS_STALE_THRESHOLD_SEC) &&
                           OBC::TtcPassPolicy::convertGpsUtcToUnixSec(
                               gpsState.utcDateYmd, gpsState.utcSecondsOfDay, currentGpsUnixSec);
        }

        this->m_status.gpsTimeValid = gpsTimeValid;
        this->m_status.currentGpsUnixSec = gpsTimeValid ? currentGpsUnixSec : 0U;
        this->m_status.windowActive =
            gpsTimeValid && this->m_status.windowConfigured &&
            OBC::TtcPassPolicy::isWindowActive(currentGpsUnixSec, this->m_window);
        this->m_status.ttcActive = currentMode == OBC::SatMode::TTC;

        if (!this->m_status.windowConfigured || !windowsEqual_(this->m_window, this->m_suppressedWindow) ||
            !this->m_status.windowActive) {
            clearWindow_(this->m_suppressedWindow);
        }

        this->m_status.lossOfLockTimerSec = OBC::TtcPassPolicy::updateLossOfLockTimer(
            this->m_commLoss, wallclockSec, this->m_status.ttcActive, haveComm && OBC::ttcPassAnyLinkAvailable(commState));

        if (currentMode == OBC::SatMode::IDLE) {
            if (previouslyTtcActive && this->m_status.windowActive) {
                this->m_suppressedWindow = this->m_window;
            }

            const bool entrySuppressed =
                this->m_status.windowActive && windowsEqual_(this->m_window, this->m_suppressedWindow);
            if (this->m_config.enabled && this->m_status.windowActive && this->m_status.gpsTimeValid &&
                !entrySuppressed) {
                requestEntry = modeControl != nullptr;
                transitionReason = OBC::TtcPassPolicyReason::WINDOW_ACTIVE;
                if (requestEntry) {
                    this->m_status.lastEntryReason = transitionReason;
                    this->m_status.entryCount++;
                }
            } else {
                this->m_commLoss.active = false;
                this->m_commLoss.noLinkStartWallclockSec = 0U;
                this->m_status.lossOfLockTimerSec = 0U;
            }
        } else if (currentMode == OBC::SatMode::TTC) {
            if (!this->m_config.enabled) {
                transitionReason = OBC::TtcPassPolicyReason::TTC_DISABLED;
            } else if (!this->m_status.windowConfigured) {
                transitionReason = OBC::TtcPassPolicyReason::WINDOW_CLEARED;
            } else if (!this->m_status.gpsTimeValid) {
                transitionReason = OBC::TtcPassPolicyReason::GPS_INVALID;
            } else if (!this->m_status.windowActive) {
                transitionReason = OBC::TtcPassPolicyReason::WINDOW_INACTIVE;
            } else if (this->m_config.lossOfLockTimeoutSec > 0U &&
                       this->m_status.lossOfLockTimerSec > this->m_config.lossOfLockTimeoutSec) {
                transitionReason = OBC::TtcPassPolicyReason::COMM_LOSS_TIMEOUT;
            }

            requestExit = transitionReason != OBC::TtcPassPolicyReason::NONE && modeControl != nullptr;
            if (requestExit) {
                if (transitionReason == OBC::TtcPassPolicyReason::COMM_LOSS_TIMEOUT && this->m_status.windowActive) {
                    this->m_suppressedWindow = this->m_window;
                }
                this->m_status.lastExitReason = transitionReason;
                this->m_status.exitCount++;
            }
        } else {
            this->m_commLoss.active = false;
            this->m_commLoss.noLinkStartWallclockSec = 0U;
            this->m_status.lossOfLockTimerSec = 0U;
        }
    }

    if (requestEntry) {
        modeControl->applyModeForInternalSource(OBC::SatMode::TTC, OBC::ModeApplySource::TtcPassPolicy);
        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_status.currentMode = OBC::SatMode::TTC;
            this->m_status.ttcActive = true;
        }
        if (adcsControl != nullptr && !adcsControl->requestPointingForTtcEntry()) {
            this->log_WARNING_HI_TTC_POLICY_ADCS_POINTING_REQUEST_FAILED();
        }
        this->log_ACTIVITY_HI_TTC_POLICY_ENTRY_REQUEST(static_cast<U32>(transitionReason));
    } else if (requestExit) {
        modeControl->applyModeForInternalSource(OBC::SatMode::IDLE, OBC::ModeApplySource::TtcPassPolicy);
        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_status.currentMode = OBC::SatMode::IDLE;
            this->m_status.ttcActive = false;
            this->m_commLoss.active = false;
            this->m_commLoss.noLinkStartWallclockSec = 0U;
            this->m_status.lossOfLockTimerSec = 0U;
        }
        this->log_ACTIVITY_HI_TTC_POLICY_EXIT_REQUEST(static_cast<U32>(transitionReason));
    }

    this->publishChangeDrivenStatus_(previousStatus, this->getStatusForRuntime());
}

void TtcPassManager::TTC_SET_POLICY_cmdHandler(FwOpcodeType opCode,
                                               U32 cmdSeq,
                                               bool enabled,
                                               U32 lossOfLockTimeoutSec) {
    this->cmdResponse_out(opCode, cmdSeq, this->setPolicyForRuntime(enabled, lossOfLockTimeoutSec));
}

void TtcPassManager::TTC_SET_PASS_WINDOW_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    U64 startUnixSec,
                                                    U64 endUnixSec) {
    this->cmdResponse_out(opCode, cmdSeq, this->setPassWindowForRuntime(startUnixSec, endUnixSec));
}

void TtcPassManager::TTC_CLEAR_PASS_WINDOW_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->clearPassWindowForRuntime();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void TtcPassManager::TTC_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->publishExplicitRefreshStatus_(this->getStatusForRuntime());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void TtcPassManager::publishChangeDrivenStatus_(const OBC::TtcPassRuntimeStatus& previous,
                                                const OBC::TtcPassRuntimeStatus& current) {
    if (previous.enabled != current.enabled) {
        this->tlmWrite_TTC_POLICY_ENABLED(current.enabled);
    }
    if (previous.windowConfigured != current.windowConfigured) {
        this->tlmWrite_TTC_POLICY_WINDOW_CONFIGURED(current.windowConfigured);
    }
    if (previous.windowActive != current.windowActive) {
        this->tlmWrite_TTC_POLICY_WINDOW_ACTIVE(current.windowActive);
    }
    if (previous.gpsTimeValid != current.gpsTimeValid) {
        this->tlmWrite_TTC_POLICY_GPS_TIME_VALID(current.gpsTimeValid);
    }
    if (previous.ttcActive != current.ttcActive) {
        this->tlmWrite_TTC_POLICY_TTC_ACTIVE(current.ttcActive);
    }
    if (previous.entryCount != current.entryCount) {
        this->tlmWrite_TTC_POLICY_ENTRY_COUNT(current.entryCount);
    }
    if (previous.exitCount != current.exitCount) {
        this->tlmWrite_TTC_POLICY_EXIT_COUNT(current.exitCount);
    }
}

void TtcPassManager::publishExplicitRefreshStatus_(const OBC::TtcPassRuntimeStatus& status) {
    this->tlmWrite_TTC_POLICY_ENABLED(status.enabled);
    this->tlmWrite_TTC_POLICY_LOSS_TIMEOUT_SEC(status.lossOfLockTimeoutSec);
    this->tlmWrite_TTC_POLICY_WINDOW_CONFIGURED(status.windowConfigured);
    this->tlmWrite_TTC_POLICY_WINDOW_START_UNIX_SEC(status.windowStartUnixSec);
    this->tlmWrite_TTC_POLICY_WINDOW_END_UNIX_SEC(status.windowEndUnixSec);
    this->tlmWrite_TTC_POLICY_WINDOW_ACTIVE(status.windowActive);
    this->tlmWrite_TTC_POLICY_GPS_TIME_VALID(status.gpsTimeValid);
    this->tlmWrite_TTC_POLICY_CURRENT_GPS_UNIX_SEC(status.currentGpsUnixSec);
    this->tlmWrite_TTC_POLICY_TTC_ACTIVE(status.ttcActive);
    this->tlmWrite_TTC_POLICY_LOSS_TIMER_SEC(status.lossOfLockTimerSec);
    this->tlmWrite_TTC_POLICY_LAST_ENTRY_REASON(static_cast<U32>(status.lastEntryReason));
    this->tlmWrite_TTC_POLICY_LAST_EXIT_REASON(static_cast<U32>(status.lastExitReason));
    this->tlmWrite_TTC_POLICY_ENTRY_COUNT(status.entryCount);
    this->tlmWrite_TTC_POLICY_EXIT_COUNT(status.exitCount);
}

void TtcPassManager::clearWindow_(OBC::TtcPassWindow& window) {
    window.startUnixSec = 0U;
    window.endUnixSec = 0U;
}

bool TtcPassManager::windowsEqual_(const OBC::TtcPassWindow& lhs, const OBC::TtcPassWindow& rhs) {
    return lhs.startUnixSec == rhs.startUnixSec && lhs.endUnixSec == rhs.endUnixSec;
}

}  // namespace OBC
