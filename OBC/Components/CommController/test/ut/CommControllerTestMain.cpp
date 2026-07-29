#include "CommControllerTester.hpp"

#include <cstdlib>
#include <string>

TEST(CommController, PrimarySwitchRequiresAvailableLink) {
    OBC::CommControllerTester tester;
    tester.testPrimarySwitchRequiresAvailableLink();
}

TEST(CommController, PassLifecycleIsObserveOnly) {
    OBC::CommControllerTester tester;
    tester.testPassLifecycleIsObserveOnly();
}

TEST(CommController, ZeroLengthPassRejected) {
    OBC::CommControllerTester tester;
    tester.testZeroLengthPassRejected();
}

TEST(CommController, GatewayDetachDoesNotFaultHealthyPrimarySubsystem) {
    OBC::CommControllerTester tester;
    tester.testGatewayDetachDoesNotFaultHealthyPrimarySubsystem();
}

TEST(CommController, SbandLossFailsOverAndRestoreReturnsPrimary) {
    OBC::CommControllerTester tester;
    tester.testSbandLossFailsOverAndRestoreReturnsPrimary();
}

TEST(CommController, SimultaneousSbandLossAndUhfRestoreFailsOver) {
    OBC::CommControllerTester tester;
    tester.testSimultaneousSbandLossAndUhfRestoreFailsOver();
}

TEST(CommController, OperatorSelectedUhfPrimaryDoesNotAutoRestoreOnSbandRecovery) {
    OBC::CommControllerTester tester;
    tester.testOperatorSelectedUhfPrimaryDoesNotAutoRestoreOnSbandRecovery();
}

TEST(CommController, UnavailableFaultClearsOnHealthyReconnectDespiteTransportNoise) {
    OBC::CommControllerTester tester;
    tester.testUnavailableFaultClearsOnHealthyReconnectDespiteTransportNoise();
}

TEST(CommController, UhfPrimaryDemotesSbandIngressAuthority) {
    OBC::CommControllerTester tester;
    tester.testUhfPrimaryDemotesSbandIngressAuthority();
}

TEST(CommController, DpActiveRejectsAdditionalDp) {
    OBC::CommControllerTester tester;
    tester.testDpActiveRejectsAdditionalDp();
}

TEST(CommController, InlineDpLaunchFailureDoesNotEmitCompletion) {
    OBC::CommControllerTester tester;
    tester.testInlineDpLaunchFailureDoesNotEmitCompletion();
}

TEST(CommController, DpCompletionClearsOwner) {
    OBC::CommControllerTester tester;
    tester.testDpCompletionClearsOwner();
}

TEST(CommController, PrimarySwitchDropsActiveDownlinkAndNotifiesDp) {
    OBC::CommControllerTester tester;
    tester.testPrimarySwitchDropsActiveDownlinkAndNotifiesDp();
}

TEST(CommController, UhfPrimaryActiveDownlinkSuspendsPacketEgress) {
    OBC::CommControllerTester tester;
    tester.testUhfPrimaryActiveDownlinkSuspendsPacketEgress();
}

TEST(CommController, UhfPrimaryDeferredLaunchDropsOnLinkLoss) {
    OBC::CommControllerTester tester;
    tester.testUhfPrimaryDeferredLaunchDropsOnLinkLoss();
}

TEST(CommController, PrimarySubsystemPingFailureTriggersUnavailableFault) {
    OBC::CommControllerTester tester;
    tester.testPrimarySubsystemPingFailureTriggersUnavailableFault();
}

TEST(CommController, DirectTcpPrimaryDoesNotGoStale) {
    OBC::CommControllerTester tester;
    tester.testDirectTcpPrimaryDoesNotGoStale();
}

TEST(CommController, SubsystemPrimaryIgnoresGroundTransportGrowth) {
    OBC::CommControllerTester tester;
    tester.testSubsystemPrimaryIgnoresGroundTransportGrowth();
}

TEST(CommController, ConnectedOnlyCompatibilityDoesNotTriggerTransportFault) {
    OBC::CommControllerTester tester;
    tester.testConnectedOnlyCompatibilityDoesNotTriggerTransportFault();
}

TEST(CommController, SecondarySubsystemProbeIsOnDemandOnly) {
    OBC::CommControllerTester tester;
    tester.testSecondarySubsystemProbeIsOnDemandOnly();
}

TEST(CommController, PrimarySubsystemProbeUsesCadence) {
    OBC::CommControllerTester tester;
    tester.testPrimarySubsystemProbeUsesCadence();
}

TEST(CommController, PrimarySubsystemSingleProbeFailureIsDebounced) {
    OBC::CommControllerTester tester;
    tester.testPrimarySubsystemSingleProbeFailureIsDebounced();
}

TEST(CommController, PrimaryFailbackForceProbeClearsDebounceState) {
    OBC::CommControllerTester tester;
    tester.testPrimaryFailbackForceProbeClearsDebounceState();
}

TEST(CommController, ForcedProbeDiscardsStaleAsyncCompletion) {
    OBC::CommControllerTester tester;
    tester.testForcedProbeDiscardsStaleAsyncCompletion();
}

TEST(CommController, DisabledGroundLinkConfigDoesNotLatchUnavailableFault) {
    OBC::CommControllerTester tester;
    tester.testDisabledGroundLinkConfigDoesNotLatchUnavailableFault();
}

TEST(CommController, SbandLiveObservabilityStartsQuietUntilAuth) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityStartsQuietUntilAuth();
}

TEST(CommController, SbandLiveObservabilityOpensAfterAcceptedSession) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityOpensAfterAcceptedSession();
}

TEST(CommController, SbandLiveObservabilityActivityDoesNotRepublishFullCommState) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityActivityDoesNotRepublishFullCommState();
}

TEST(CommController, SbandLiveObservabilityIgnoresLegacySessionOpen) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityIgnoresLegacySessionOpen();
}

TEST(CommController, SbandLiveObservabilityClosesWhenSecureSessionIsReplacedByLegacySession) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityClosesWhenSecureSessionIsReplacedByLegacySession();
}

TEST(CommController, SbandLiveObservabilityClosesOnSessionRevoke) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityClosesOnSessionRevoke();
}

TEST(CommController, SbandLiveObservabilityClosesOnSilentRecoveryRevoke) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityClosesOnSilentRecoveryRevoke();
}

TEST(CommController, SbandLiveObservabilityClosesOnBandSwitch) {
    OBC::CommControllerTester tester;
    tester.testSbandLiveObservabilityClosesOnBandSwitch();
}

TEST(CommController, GetStatusRepublishesCorePostureWhenValuesUnchanged) {
    OBC::CommControllerTester tester;
    tester.testGetStatusRepublishesCorePostureWhenValuesUnchanged();
}

TEST(CommController, GetStatusDoesNotRepublishDeferredOrDiagnosticTelemetry) {
    OBC::CommControllerTester tester;
    tester.testGetStatusDoesNotRepublishDeferredOrDiagnosticTelemetry();
}

TEST(CommController, GetStatusDoesNotAdvancePassTimerOrForceSyncProbe) {
    OBC::CommControllerTester tester;
    tester.testGetStatusDoesNotAdvancePassTimerOrForceSyncProbe();
}

TEST(CommController, GetStatusDoesNotAdvanceCommFdirDebounce) {
    OBC::CommControllerTester tester;
    tester.testGetStatusDoesNotAdvanceCommFdirDebounce();
}

TEST(CommController, ReliableTransferAdmissionUsesHelper) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferAdmissionUsesHelper();
}

TEST(CommController, ReliableTransferSwitchedUhfPrimaryUsesHelper) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferSwitchedUhfPrimaryUsesHelper();
}

TEST(CommController, ReliableTransferFallsBackWhenReceiverDisabled) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferFallsBackWhenReceiverDisabled();
}

TEST(CommController, ReliableTransferBootstrapUhfPrimaryFallsBackToStockDownlink) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferBootstrapUhfPrimaryFallsBackToStockDownlink();
}

TEST(CommController, ReliableTransferFailoverToUhfFallsBackToStockDownlink) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferFailoverToUhfFallsBackToStockDownlink();
}

TEST(CommController, ReliableTransferBusyRejectsWhileActive) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferBusyRejectsWhileActive();
}

TEST(CommController, ReliableTransferPrimarySwitchAbortsActiveTransfer) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferPrimarySwitchAbortsActiveTransfer();
}

TEST(CommController, ReliableTransferFailoverAbortsActiveTransfer) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferFailoverAbortsActiveTransfer();
}

TEST(CommController, ReliableTransferUhfFailoverAbortsActiveTransfer) {
    OBC::CommControllerTester tester;
    tester.testReliableTransferUhfFailoverAbortsActiveTransfer();
}

TEST(CommController, UhfBeaconSuppressStartsAfterAcceptedSessionOpen) {
    OBC::CommControllerTester tester;
    tester.testUhfBeaconSuppressStartsAfterAcceptedSessionOpen();
}

TEST(CommController, UhfBeaconSuppressRefreshesAndTimesOut) {
    OBC::CommControllerTester tester;
    tester.testUhfBeaconSuppressRefreshesAndTimesOut();
}

TEST(CommController, UhfBeaconSuppressRoleSwitchClearsImmediately) {
    OBC::CommControllerTester tester;
    tester.testUhfBeaconSuppressRoleSwitchClearsImmediately();
}

TEST(CommController, UhfBeaconSuppressSessionReplaceRebindsOwner) {
    OBC::CommControllerTester tester;
    tester.testUhfBeaconSuppressSessionReplaceRebindsOwner();
}

TEST(CommController, UhfBeaconSuppressIgnoresNonQualifyingActivity) {
    OBC::CommControllerTester tester;
    tester.testUhfBeaconSuppressIgnoresNonQualifyingActivity();
}

TEST(CommController, UhfPrimaryBandLeavesPacketEgressNonQuiet) {
    OBC::CommControllerTester tester;
    tester.testUhfPrimaryBandLeavesPacketEgressNonQuiet();
}

TEST(CommController, UhfBackupSuppressDoesNotEnablePacketQuietWhileSbandPrimary) {
    OBC::CommControllerTester tester;
    tester.testUhfBackupSuppressDoesNotEnablePacketQuietWhileSbandPrimary();
}
