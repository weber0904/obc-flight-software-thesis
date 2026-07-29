#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    struct SourceCase {
        OBC::RecoveryIncidentSource source;
        OBC::RecoveryLevel initialLevel;
        OBC::ResetCause rebootCause;
    };

    static const SourceCase sourceCases[] = {
        {OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
         OBC::ResetCause::RECOVERY_WATCHDOG},
        {OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
         OBC::ResetCause::RECOVERY_WATCHDOG},
        {OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
         OBC::ResetCause::RECOVERY_WATCHDOG},
        {OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
         OBC::ResetCause::RECOVERY_WATCHDOG},
        {OBC::RecoveryIncidentSource::EPS_TIMEOUT, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE,
         OBC::ResetCause::RECOVERY_EPS_TIMEOUT},
        {OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
         OBC::ResetCause::RECOVERY_WATCHDOG},
        {OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE,
         OBC::ResetCause::RECOVERY_ADCS_FDIR},
        {OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE,
         OBC::ResetCause::RECOVERY_ADCS_FDIR},
        {OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE,
         OBC::ResetCause::RECOVERY_COMM_FDIR},
        {OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE,
         OBC::ResetCause::RECOVERY_COMM_FDIR},
    };

    ok = check(sizeof(sourceCases) / sizeof(sourceCases[0]) == OBC::RecoveryIncidentSourceCount,
               "sourceCases length must match RecoveryIncidentSourceCount") &&
         ok;

    for (const auto& sourceCase : sourceCases) {
        ok = check(OBC::recoveryIncidentSourceIsIndexed(sourceCase.source), "recovery source must be indexed") && ok;
        ok = check(OBC::recoveryIncidentSourceName(sourceCase.source)[0] != '\0', "recovery source must have a name") && ok;
        ok = check(OBC::recoveryInitialLevelForSource(sourceCase.source) == sourceCase.initialLevel,
                   "recovery source must map to expected initial level") &&
             ok;
        ok = check(OBC::recoveryResetCauseForSource(sourceCase.source) == sourceCase.rebootCause,
                   "recovery source must map to expected reboot cause") &&
             ok;
    }

    OBC::RecoveryIncidentSource mapped = OBC::RecoveryIncidentSource::NONE;
    ok = check(OBC::recoveryIncidentSourceFromWatchdog(OBC::WatchdogSource::ADCS_FDIR, mapped),
               "ADCS_FDIR watchdog source must normalize into recovery source") &&
         ok;
    ok = check(mapped == OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR,
               "ADCS_FDIR watchdog source must normalize to WATCHDOG_ADCS_FDIR") &&
         ok;
    ok = check(std::string(OBC::recoveryActionName(OBC::RecoveryAction::COMM_LINK_FAILOVER)) == "COMM_LINK_FAILOVER",
               "COMM_LINK_FAILOVER action name must be stable") &&
         ok;

    return ok ? 0 : 1;
}
