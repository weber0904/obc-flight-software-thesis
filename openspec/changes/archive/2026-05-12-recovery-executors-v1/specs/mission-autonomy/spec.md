## ADDED Requirements

### Requirement: RecoveryExecutor Owns Shared Runtime Recovery
The mission-autonomy capability SHALL provide a dedicated `RecoveryExecutor` owner for shared runtime recovery, and the active baseline SHALL keep that owner separate from `WatchdogSupervisor`, `EpsFdirController`, `ModeSafetyController`, and `BootManager`.

#### Scenario: Shared recovery owner is distinct from detectors and boot truth
- **WHEN** `recovery-executors-v1` is implemented
- **THEN** the active runtime SHALL instantiate and schedule a focused shared recovery owner
- **AND** `WatchdogSupervisor` SHALL remain a watchdog detector
- **AND** `EpsFdirController` SHALL remain an EPS-timeout detector
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy
- **AND** `BootManager` SHALL remain limited to boot/update metadata truth instead of owning runtime recovery progression

### Requirement: RecoveryExecutor Uses Deterministic Incident Progression
The mission-autonomy capability SHALL make `RecoveryExecutor` own bounded incident normalization, relatch tracking, recovery-level progression, and action gating for the active baseline recovery consumers.

#### Scenario: Watchdog and EPS incidents stay distinct
- **WHEN** watchdog and EPS detectors emit recovery requests in this change
- **THEN** `RecoveryExecutor` SHALL track bounded incident keys that distinguish `WATCHDOG_<source>` incidents from `EPS_TIMEOUT`
- **AND** it SHALL keep per-incident epoch, current level, highest level, relatch count, action-issued state, and clear state

#### Scenario: Clear only closes the matching incident epoch
- **WHEN** a detector later reports recovery clear for an active incident
- **THEN** `RecoveryExecutor` SHALL clear only the matching incident epoch
- **AND** it SHALL NOT implicitly clear a different detector source or a later relatch epoch

#### Scenario: Repeated failure escalates instead of repeating the same action forever
- **WHEN** an incident relatches or remains uncleared after its bounded action set has already been issued
- **THEN** `RecoveryExecutor` SHALL advance that incident to the next defined bounded recovery level
- **AND** it SHALL NOT repeatedly reissue the same no-op action forever

## MODIFIED Requirements

### Requirement: EPS Timeout FDIR Owner Is Separate From Mode Safety
The mission-autonomy capability SHALL provide a focused `EpsFdirController` owner for EPS timeout detection, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController` and from the shared `RecoveryExecutor`.

#### Scenario: EPS timeout FDIR does not broaden ModeSafetyController
- **WHEN** `recovery-executors-v1` is implemented
- **THEN** the active runtime SHALL instantiate and schedule a focused EPS timeout FDIR detector
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy instead of taking subsystem timeout, retry, or recovery ownership
- **AND** `RecoveryExecutor` SHALL own later recovery progression instead of `EpsFdirController`

### Requirement: EPS Timeout Escalation Uses Normal Mode Surface
The mission-autonomy capability SHALL make EPS timeout faults enter the shared recovery path while still using the existing internal mode-control path for any resulting `SAFE` fallback instead of inventing a parallel mode store.

#### Scenario: Third consecutive failure enters the shared recovery path
- **WHEN** the EPS timeout FDIR owner first latches fault at consecutive failure count `3`
- **THEN** it SHALL emit one shared recovery request for the active incident epoch
- **AND** `RecoveryExecutor` SHALL treat that incident as entering subsystem recovery progression rather than a detector-owned mode change

#### Scenario: Active modes can still fall back to SAFE through the normal path
- **WHEN** the EPS timeout incident reaches the bounded `SAFE` fallback action
- **AND** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **THEN** `RecoveryExecutor` SHALL request `SAFE` through the normal internal mode-control runtime path
- **AND** it SHALL tag that request with the distinct subsystem-fault internal apply source

#### Scenario: SAFE and HELL remain fault-only mode cases
- **WHEN** the EPS timeout incident reaches the bounded `SAFE` fallback action
- **AND** the current mode is `SAFE` or `HELL`
- **THEN** the runtime SHALL record the incident and later recovery action truth
- **AND** it SHALL NOT request another mode change while the current mode remains `SAFE` or `HELL`

#### Scenario: Repeated EPS timeout does not create a second recovery framework
- **WHEN** the EPS timeout detector remains fault-latched or later relatches in this change
- **THEN** it SHALL continue to use the same `RecoveryExecutor` incident path
- **AND** it SHALL NOT invent a detector-local second escalation framework outside the shared executor

### Requirement: EPS Timeout Recovery Clears On First Success
The mission-autonomy capability SHALL clear the EPS timeout fault latch on the first successful EPS poll after a fault, and shared recovery closure SHALL follow that explicit detector clear rather than an inferred mode restore.

#### Scenario: First success clears latched fault
- **WHEN** the EPS timeout FDIR owner is fault-latched
- **AND** the EPS runtime health provider reports a later successful poll
- **THEN** it SHALL clear the fault latch
- **AND** it SHALL emit recovered or fault-cleared evidence
- **AND** it SHALL notify `RecoveryExecutor` that the active EPS incident can close

#### Scenario: Recovery does not auto-restore prior mode
- **WHEN** the EPS timeout detector clears a latched fault after a successful poll
- **THEN** the active runtime SHALL NOT automatically restore the pre-fault mode
- **AND** it SHALL leave the post-fault mode unchanged unless a separate governed mode request is issued

### Requirement: WatchdogSupervisor Owner Is Separate From Mode Safety And EPS Timeout FDIR
The mission-autonomy capability SHALL provide a focused `WatchdogSupervisor` owner for runtime liveness supervision, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController`, the EPS-timeout detector `EpsFdirController`, and the shared `RecoveryExecutor`.

#### Scenario: Watchdog-v1 does not broaden existing owners
- **WHEN** `recovery-executors-v1` is implemented
- **THEN** the active runtime SHALL instantiate, configure, and schedule a focused watchdog owner
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy
- **AND** `EpsFdirController` SHALL remain limited to EPS timeout retry, fault latch, and recovery-clear behavior
- **AND** `RecoveryExecutor` SHALL own later recovery progression instead of `WatchdogSupervisor`

### Requirement: Watchdog Escalation Uses The Normal Mode Surface
The mission-autonomy capability SHALL make watchdog stale faults enter the shared recovery path while still using the existing internal mode-control path for any resulting `SAFE` fallback instead of inventing a parallel watchdog-owned mode store.

#### Scenario: First watchdog fault crossing enters the shared recovery path
- **WHEN** a supervised source first crosses the configured watchdog fault threshold
- **THEN** `WatchdogSupervisor` SHALL emit one shared recovery request for that source's active incident epoch
- **AND** `RecoveryExecutor` SHALL treat that incident as opening at the bounded watchdog recovery level progression

#### Scenario: Active modes still request SAFE through the normal path
- **WHEN** the watchdog incident reaches the bounded `SAFE` fallback action
- **AND** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **THEN** `RecoveryExecutor` SHALL request `SAFE` through the normal internal mode-control runtime path
- **AND** it SHALL keep that request tagged with the distinct watchdog-fault internal apply source

#### Scenario: SAFE and HELL remain fault-only watchdog cases
- **WHEN** the watchdog incident reaches the bounded `SAFE` fallback action
- **AND** the current mode is `SAFE` or `HELL`
- **THEN** the runtime SHALL record the incident and later recovery action truth
- **AND** it SHALL NOT request another mode change while the current mode remains `SAFE` or `HELL`

#### Scenario: Repeated stale evaluation does not reissue the same watchdog action forever
- **WHEN** a supervised source remains stale after its current watchdog recovery actions have already been issued
- **THEN** `RecoveryExecutor` SHALL advance later escalation according to the bounded watchdog progression
- **AND** it SHALL NOT repeatedly re-request the same watchdog `SAFE` action for the same incident epoch

### Requirement: Watchdog Feed Suppression Remains A Supervisor-Side Claim
The mission-autonomy capability SHALL let watchdog-v1 keep watchdog-feed suppression truth inside `WatchdogSupervisor` while allowing that suppression to drive shared reboot escalation through `RecoveryExecutor`.

#### Scenario: Continued stale source suppresses feed eligibility
- **WHEN** a supervised source continues stale long enough to cross the configured feed-suppression threshold
- **THEN** `WatchdogSupervisor` SHALL mark watchdog feed as ineligible
- **AND** it SHALL emit reviewable feed-suppressed evidence
- **AND** it SHALL notify `RecoveryExecutor` that the active watchdog incident has reached the bounded reboot-escalation trigger

#### Scenario: Recovery-executors-v1 stays bounded
- **WHEN** watchdog feed suppression is recorded in this change
- **THEN** the active baseline SHALL NOT by itself claim Raspberry Pi hardware watchdog stroking, watchdog-caused reset proof, subsystem power-cycling, or a broad multi-subsystem FDIR framework

### Requirement: Watchdog Recovery Clears On First Restored Beat
The mission-autonomy capability SHALL clear watchdog-v1 stale state on the first restored heartbeat according to explicit aggregate rules, and shared recovery closure SHALL follow that explicit detector clear rather than an inferred mode restore.

#### Scenario: Source recovery clears its stale state
- **WHEN** a supervised source has crossed watchdog warning, fault, or suppress thresholds
- **AND** that source later emits a valid heartbeat
- **THEN** `WatchdogSupervisor` SHALL clear that source's stale state immediately
- **AND** it SHALL notify `RecoveryExecutor` that the matching watchdog incident can close

#### Scenario: Recovery does not auto-restore prior mode
- **WHEN** watchdog-v1 clears a latched source fault after heartbeat recovery
- **THEN** the active runtime SHALL NOT automatically restore the pre-fault mode
- **AND** it SHALL leave the post-fault mode unchanged unless a separate governed mode request is issued

#### Scenario: Aggregate recovery waits for all enabled sources
- **WHEN** one stale source recovers while another enabled supervised source remains faulted or suppressed
- **THEN** aggregate watchdog fault or feed-suppressed state SHALL remain active
- **AND** the aggregate state SHALL clear only after no enabled source remains faulted or suppressed
