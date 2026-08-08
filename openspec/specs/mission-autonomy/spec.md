# mission-autonomy Specification

## Purpose
Define the first hosted mission-autonomy behaviors that react to cached subsystem state and issue system-level control actions without feeding scenario truth directly into the OBC runtime.
## Requirements
### Requirement: Mission Executive Redesign Is Deferred
The mission-autonomy capability SHALL keep the existing provisional `MissionExecutive` low-battery, sun-safe, and detumble behavior retired from the active runtime baseline unless a later governed change explicitly redesigns and verifies that broader policy.

#### Scenario: Mode safety policy does not restore retired autonomy
- **WHEN** mode-safety-policy-v1 is complete
- **THEN** the active runtime SHALL NOT depend on the provisional `MissionExecutive` to command `HELL`, `SAFE`, `IDLE`, `PAYLOAD`, `TTC`, ADCS pointing, ADCS detumble, load shedding, COMM behavior, scheduler behavior, watchdog behavior, subsystem timeout/retry/reset behavior, or broader FDIR actions
- **AND** later autonomy changes SHALL define their own policy, thresholds, runtime owner, and verification evidence before claiming those behaviors

### Requirement: Mode Safety Controller Owns V1 SoC Fallback
The mission-autonomy capability SHALL provide a narrow active `ModeSafetyController` component that owns the v1 cached-EPS SoC safety fallback policy for the `SAFE`, `HELL`, `IDLE`, `PAYLOAD`, and `TTC` mode model, and TTC pass-window automation SHALL remain outside that SoC-only owner.

#### Scenario: Cached EPS status remains the only safety policy input
- **WHEN** `ModeSafetyController` evaluates safety fallback or cached-EPS operator safety guard behavior
- **THEN** it SHALL use cached EPS state of charge from the runtime EPS status provider and the current mode from the runtime mode-control provider
- **AND** it SHALL NOT consume scenario truth directly, command ADCS, command EPS load shedding, command COMM behavior, schedule payload activity, schedule TTC pass windows, or perform watchdog, subsystem timeout, retry, reset, or broader FDIR actions

#### Scenario: Low-battery TTC handling stays on existing safety path
- **WHEN** current mode is `TTC`
- **AND** cached EPS SoC falls below the existing fallback threshold
- **THEN** `ModeSafetyController` SHALL continue to request the existing safety fallback through the normal internal mode-control runtime path
- **AND** TTC pass-window policy SHALL NOT override that safety action

### Requirement: Strict SoC Fallback Thresholds
The mission-autonomy capability SHALL apply strict, deterministic SoC fallback and payload-exit thresholds without runtime configurability in v1.

#### Scenario: SAFE falls back to HELL below critical SoC
- **WHEN** the current mode is `SAFE`
- **AND** cached EPS SoC is less than `10%`
- **THEN** `ModeSafetyController` SHALL request `HELL` through the normal mode-control runtime path

#### Scenario: SAFE boundary does not fall back to HELL
- **WHEN** the current mode is `SAFE`
- **AND** cached EPS SoC is exactly `10%`
- **THEN** `ModeSafetyController` SHALL NOT request `HELL`

#### Scenario: HELL recovers only to SAFE above hysteresis SoC
- **WHEN** the current mode is `HELL`
- **AND** cached EPS SoC is greater than `15%`
- **THEN** `ModeSafetyController` SHALL request `SAFE` through the normal mode-control runtime path

#### Scenario: HELL boundary does not recover to SAFE
- **WHEN** the current mode is `HELL`
- **AND** cached EPS SoC is exactly `15%`
- **THEN** `ModeSafetyController` SHALL NOT request `SAFE`

#### Scenario: Active modes fall back to SAFE below operating SoC
- **WHEN** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **AND** cached EPS SoC is less than `40%`
- **THEN** `ModeSafetyController` SHALL request `SAFE` through the normal mode-control runtime path

#### Scenario: Active-mode boundary does not fall back to SAFE
- **WHEN** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **AND** cached EPS SoC is exactly `40%`
- **THEN** `ModeSafetyController` SHALL NOT request `SAFE`

### Requirement: Manual Recovery Boundary
The mission-autonomy capability SHALL keep recovery from `SAFE` to `IDLE` manual in v1, but SoC admission SHALL still be enforced for that manual request.

#### Scenario: High SoC does not automatically restore IDLE
- **WHEN** the current mode is `SAFE`
- **AND** cached EPS SoC is above `50%`
- **THEN** `ModeSafetyController` SHALL NOT request `IDLE`
- **AND** SoC recovery above `50%` SHALL remain only an operator admission boundary in this change

#### Scenario: Missing EPS cache is non-authoritative
- **WHEN** cached EPS status is unavailable
- **THEN** `ModeSafetyController` SHALL NOT request any mode transition

#### Scenario: Failed EPS poll makes cached safety input unavailable
- **WHEN** the runtime EPS status provider has a previously cached EPS status
- **AND** a later EPS status poll fails
- **THEN** the cached EPS status SHALL be treated as unavailable for `ModeSafetyController` until a later successful status update
- **AND** `ModeSafetyController` SHALL NOT evaluate fallback policy against the stale SoC value

#### Scenario: Unconfigured runtime dependencies are non-authoritative
- **WHEN** `ModeSafetyController` does not have both runtime mode-control and EPS-status providers configured
- **THEN** it SHALL NOT evaluate fallback policy
- **AND** it SHALL NOT publish decision telemetry based on a default assumed mode

#### Scenario: Already-target mode is not requested again
- **WHEN** the current mode already equals the policy fallback target for the current SoC band
- **THEN** `ModeSafetyController` SHALL NOT issue a duplicate mode transition request

### Requirement: ModeSafetyController Owns Operator Safety Guard
The mission-autonomy capability SHALL keep `ModeSafetyController` as the owner of cached-EPS SoC protection decisions used by the v1 operator transition guard.

#### Scenario: HELL to SAFE operator recovery uses cached EPS
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **THEN** `ModeSafetyController` SHALL evaluate cached EPS SoC before accepting the transition
- **AND** the request SHALL be accepted only when cached EPS SoC is strictly greater than `15%`

#### Scenario: HELL to SAFE boundary is rejected
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **AND** cached EPS SoC is exactly `15%`
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`

#### Scenario: HELL to SAFE below threshold is rejected
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **AND** cached EPS SoC is less than `15%`
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`

#### Scenario: HELL to SAFE without EPS cache is rejected
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **AND** cached EPS status is unavailable
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_UNAVAILABLE`

#### Scenario: Cached EPS semantics match safety fallback
- **WHEN** `ModeSafetyController` evaluates an operator `HELL -> SAFE` recovery request
- **THEN** it SHALL use `OBC::EPS::StatusData::soc` as `F32` percent
- **AND** it SHALL treat `IModeSafetyEpsStatus::getCachedStatusForRuntime(...) == false` as unavailable
- **AND** it SHALL NOT consume scenario truth directly or invent an EPS status fallback

#### Scenario: V1 does not add timestamp freshness
- **WHEN** `ModeSafetyController` evaluates an operator transition guard in this change
- **THEN** it SHALL NOT require a timestamp-based freshness threshold because the current cached EPS status interface has no timestamp
- **AND** later timestamp freshness SHALL require a separate governed interface change

#### Scenario: V1 does not add new EPS numeric validation
- **WHEN** `ModeSafetyController` evaluates cached EPS SoC in this change
- **THEN** it SHALL rely on the EPS provider cache validity contract
- **AND** it SHALL NOT introduce new EPS out-of-range or NaN validity policy unless a later governed change defines that status-validity contract

### Requirement: Internal Safety Apply Path Is Explicit
The mission-autonomy capability SHALL keep internal safety mode application separate from operator-requested transition validation.

#### Scenario: Safety fallback uses internal source
- **WHEN** `ModeSafetyController` autonomously requests `SAFE -> HELL` or active-mode `-> SAFE` because cached EPS SoC crosses the existing fallback thresholds
- **THEN** it SHALL call the explicit internal-source mode apply API with source `SafetyFallback`

#### Scenario: Safety recovery uses internal source
- **WHEN** `ModeSafetyController` autonomously requests `HELL -> SAFE` because cached EPS SoC is greater than `15%`
- **THEN** it SHALL call the explicit internal-source mode apply API with source `SafetyRecovery`

#### Scenario: Operator path does not use internal source
- **WHEN** `MODE_SET` or hosted `mode <...>` processes an operator request
- **THEN** the operator path SHALL NOT call the internal-source mode apply API directly

#### Scenario: Test setup source is not production operator behavior
- **WHEN** tests need to arrange a current mode that cannot be reached through an ordinary operator request
- **THEN** they MAY use a `TestSetup` internal source through test-only or fixture-controlled access
- **AND** production operator paths SHALL NOT use `TestSetup`

### Requirement: ModeSafetyController Owns SoC Admission And Payload Exit
The mission-autonomy capability SHALL keep `ModeSafetyController` as the owner of cached-EPS SoC decisions for operator recovery, operator admission, and automatic PAYLOAD exit in the v1 mode/power-safety slice.

#### Scenario: SAFE to IDLE admission uses cached EPS
- **WHEN** the current mode is `SAFE`
- **AND** an operator requests `IDLE`
- **THEN** `ModeSafetyController` SHALL evaluate cached EPS SoC before accepting the transition
- **AND** the request SHALL be accepted only when cached EPS SoC is strictly greater than `50%`

#### Scenario: IDLE to PAYLOAD admission uses cached EPS
- **WHEN** the current mode is `IDLE`
- **AND** an operator requests `PAYLOAD`
- **THEN** `ModeSafetyController` SHALL evaluate cached EPS SoC before accepting the transition
- **AND** the request SHALL be accepted only when cached EPS SoC is strictly greater than `70%`

#### Scenario: SoC-guarded admissions fail closed without cache
- **WHEN** `ModeSafetyController` evaluates operator `SAFE -> IDLE`, `IDLE -> PAYLOAD`, or `HELL -> SAFE`
- **AND** cached EPS status is unavailable
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_UNAVAILABLE`

#### Scenario: Operator admission boundary is rejected
- **WHEN** the current mode is `SAFE`
- **AND** an operator requests `IDLE`
- **AND** cached EPS SoC is exactly `50%`
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`

#### Scenario: PAYLOAD admission boundary is rejected
- **WHEN** the current mode is `IDLE`
- **AND** an operator requests `PAYLOAD`
- **AND** cached EPS SoC is exactly `70%`
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`

#### Scenario: Payload exits to IDLE below mission band
- **WHEN** the current mode is `PAYLOAD`
- **AND** cached EPS SoC is less than `60%`
- **AND** cached EPS SoC is not less than `40%`
- **THEN** `ModeSafetyController` SHALL request `IDLE` through the normal internal mode-control runtime path

#### Scenario: Payload safe fallback wins below operating floor
- **WHEN** the current mode is `PAYLOAD`
- **AND** cached EPS SoC is less than `40%`
- **THEN** `ModeSafetyController` SHALL request `SAFE` through the normal internal mode-control runtime path
- **AND** it SHALL NOT request `IDLE` first

#### Scenario: TTC remains ungated by new SoC admission threshold
- **WHEN** the current mode is `IDLE`
- **AND** an operator requests `TTC`
- **THEN** `ModeSafetyController` SHALL NOT require a new SoC admission threshold for that transition in this change
- **AND** low-battery handling while in `TTC` SHALL remain the existing fallback to `SAFE` below `40%`

#### Scenario: Cache validity defines stale handling in this slice
- **WHEN** `ModeSafetyController` evaluates SoC-guarded recovery, admission, or automatic PAYLOAD exit in this change
- **THEN** it SHALL treat `IModeSafetyEpsStatus::getCachedStatusForRuntime(...) == false` as unavailable cached EPS input
- **AND** it SHALL NOT add timestamp-based freshness or age-threshold logic in this change

### Requirement: Payload Exit Uses Explicit Internal Source
The mission-autonomy capability SHALL keep automatic PAYLOAD exit distinct from operator requests and other internal safety apply sources.

#### Scenario: Payload exit uses explicit source tag
- **WHEN** `ModeSafetyController` autonomously requests `PAYLOAD -> IDLE` because cached EPS SoC is less than `60%`
- **THEN** it SHALL call the explicit internal-source mode apply API with source `SafetyPayloadExit`

### Requirement: EPS Timeout FDIR Owner Is Separate From Mode Safety
The mission-autonomy capability SHALL provide a focused `EpsFdirController` owner for EPS timeout detection, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController` and from the shared `RecoveryExecutor`.

#### Scenario: EPS timeout FDIR does not broaden ModeSafetyController
- **WHEN** `recovery-executors-v1` is implemented
- **THEN** the active runtime SHALL instantiate and schedule a focused EPS timeout FDIR detector
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy instead of taking subsystem timeout, retry, or recovery ownership
- **AND** `RecoveryExecutor` SHALL own later recovery progression instead of `EpsFdirController`

### Requirement: EPS Timeout FDIR Uses Consecutive Poll Failures Only
The mission-autonomy capability SHALL define EPS timeout FDIR v1 from the EPS runtime poll-health contract using consecutive poll failures only.

#### Scenario: First two failures are retry-only
- **WHEN** the EPS runtime health provider reports consecutive poll-failure count `1` or `2`
- **THEN** the EPS timeout FDIR owner SHALL treat the subsystem as retrying only
- **AND** it SHALL NOT request `SAFE`

#### Scenario: Third consecutive failure latches fault
- **WHEN** the EPS runtime health provider reports consecutive poll-failure count `3`
- **THEN** the EPS timeout FDIR owner SHALL latch an EPS fault
- **AND** it SHALL emit fault or escalation evidence for that first threshold crossing

#### Scenario: V1 does not add timestamp freshness
- **WHEN** the EPS timeout FDIR owner evaluates EPS poll-health state in this change
- **THEN** it SHALL use consecutive poll failures only
- **AND** it SHALL NOT add timestamp freshness, age-threshold logic, or mixed failure-plus-freshness policy in this change

### Requirement: EPS Timeout Escalation Uses Normal Mode Surface
The mission-autonomy capability SHALL keep EPS timeout faults on the shared recovery path while preserving the existing EPS detector-only contract.

#### Scenario: EPS timeout still uses the shared executor as the action owner
- **WHEN** the EPS timeout detector first latches fault at consecutive failure count `3`
- **THEN** `EpsFdirController` SHALL remain responsible only for EPS-local retry, latch, and first-success clear truth
- **AND** `RecoveryExecutor` SHALL remain the only runtime owner allowed to execute the later EPS reset, `SAFE`, or reboot-intent actions

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
The mission-autonomy capability SHALL provide a focused `WatchdogSupervisor` owner for runtime liveness supervision, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController`, the subsystem FDIR detectors, and the shared `RecoveryExecutor`.

#### Scenario: ADCS FDIR joins the bounded watchdog supervised set
- **WHEN** `multi-subsystem-fdir-v1` is implemented
- **THEN** the active runtime SHALL instantiate, configure, and schedule `AdcsFdirController`
- **AND** the watchdog supervised source set SHALL include `ADCS_FDIR` in addition to the previously governed sources
- **AND** `RecoveryExecutor` SHALL normalize that stale source as `WATCHDOG_ADCS_FDIR`

### Requirement: Watchdog-v1 Uses Explicit Heartbeats For A Bounded Source Set
The mission-autonomy capability SHALL define watchdog-v1 from explicit per-source heartbeats emitted by a bounded active-baseline source set.

#### Scenario: Supervised set is explicit
- **WHEN** watchdog-v1 is implemented
- **THEN** the supervised source set SHALL include `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`
- **AND** it SHALL NOT implicitly claim `ModeManager`, slow-group owners, or data-group owners are supervised in this change

#### Scenario: V1 heartbeat is explicit rather than inferred
- **WHEN** `WatchdogSupervisor` evaluates liveness in this change
- **THEN** it SHALL use explicit heartbeat submission from the supervised owners
- **AND** it SHALL NOT infer source freshness only from expected schedule cadence or upstream active-component ping alone

### Requirement: Watchdog-v1 Uses Deterministic Tick-Based Freshness
The mission-autonomy capability SHALL define watchdog-v1 freshness from deterministic tick-based age thresholds per supervised source.

#### Scenario: Threshold crossings are ordered
- **WHEN** a supervised source ages past its configured warning, safe, and suppress thresholds
- **THEN** the watchdog owner SHALL evaluate those thresholds in that order
- **AND** it SHALL treat warning as less severe than latched fault
- **AND** it SHALL treat feed suppression as more severe than latched fault

#### Scenario: V1 does not add persistent watchdog config
- **WHEN** `SET_WATCHDOG_CONFIG` changes watchdog-v1 thresholds or enable flags
- **THEN** the updated config SHALL apply to the current runtime only
- **AND** it SHALL NOT by itself claim reboot-persistent watchdog config storage in this change

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

### Requirement: RecoveryExecutor Owns Bounded Multi-Subsystem Recovery
The mission-autonomy capability SHALL use one shared `RecoveryExecutor` owner for bounded `EPS`, `ADCS`, and `COMM` detector incidents on the active baseline.

#### Scenario: Three bounded subsystem lines share one recovery owner
- **WHEN** `multi-subsystem-fdir-v1` is implemented
- **THEN** `RecoveryExecutor` SHALL remain distinct from `EpsFdirController`, `AdcsFdirController`, `CommController`, `WatchdogSupervisor`, `ModeSafetyController`, and `BootManager`
- **AND** the active baseline SHALL limit shared subsystem recovery consumers to `EPS`, `ADCS`, and `COMM`
- **AND** it SHALL NOT claim GPS, payload, TTC, scheduler, or storage-health recovery ownership in this change

### Requirement: RecoveryExecutor Uses Short-Lock Decision And Lock-Free Action Execution
The mission-autonomy capability SHALL keep `RecoveryExecutor` incident normalization and progression decisions under a bounded internal lock while executing external recovery actions after that lock is released.

#### Scenario: External recovery action does not run under the executor lock
- **WHEN** `RecoveryExecutor` issues `SAFE`, EPS reset, COMM failover, or reboot intent
- **THEN** it SHALL decide the next action while holding its internal recovery mutex
- **AND** it SHALL execute the external runtime action only after releasing that mutex
- **AND** it SHALL later record the action outcome without requiring external runtime code to reenter the same locked region

### Requirement: Recovery Progression Is Deterministic Per Incident
The mission-autonomy capability SHALL keep explicit incident epoch, relatch, level, and clear state for each active shared recovery source.

#### Scenario: Repeated failure escalates instead of looping forever
- **WHEN** a subsystem incident relatches after clear or remains uncleared for `3` executor ticks after its first bounded action
- **THEN** `RecoveryExecutor` SHALL advance that incident to `R6_OBC_REBOOT`
- **AND** it SHALL NOT reissue the same bounded action forever for the same incident epoch

### Requirement: ADCS FDIR Uses Scheduled Poll Health Only
The mission-autonomy capability SHALL treat ADCS shared recovery incidents as detector outputs derived only from scheduled ADCS poll health.

#### Scenario: ADCS command-path failures do not become FDIR incidents
- **WHEN** `ADCS_GET_ATTITUDE`, `ADCS_SET_MODE`, `ADCS_SET_TARGET`, or `ADCS_CALIBRATE` encounters transport failure outside the scheduled poll
- **THEN** the active runtime MAY emit local ADCS comm-error evidence
- **AND** it SHALL NOT advance the scheduled ADCS FDIR counters or latch a shared ADCS recovery incident from that command-path failure alone

#### Scenario: ADCS scheduled poll thresholds are fixed
- **WHEN** the active baseline evaluates scheduled ADCS poll health in this change
- **THEN** it SHALL latch `ADCS_POLL_TRANSPORT` after `3` consecutive scheduled transport failures
- **AND** it SHALL latch `ADCS_POLL_FRESHNESS` after `3` consecutive scheduled no-valid-refresh cycles
- **AND** it SHALL clear either ADCS incident on the first scheduled healthy valid cycle

### Requirement: COMM Detector Is Separate From COMM Recovery Actuation
The mission-autonomy capability SHALL keep COMM fault detection in `CommController` while making fault-driven COMM recovery actuation executor-owned.

#### Scenario: COMM detector does not switch primary links directly
- **WHEN** the current primary COMM link becomes faulted by repeated unavailability or repeated transport-error growth
- **THEN** `CommController` SHALL emit reviewable COMM detector fault truth
- **AND** it SHALL NOT directly switch primary links, revoke the current primary session, or clear downlink ownership as part of detector-side fault handling

#### Scenario: COMM scheduled thresholds are configuration-driven
- **WHEN** the active baseline evaluates COMM detector health in this change
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE` after the configured `unavailableFailureThreshold` consecutive scheduled detector cycles where the current primary link is unavailable
- **AND** it SHALL latch `COMM_PRIMARY_TRANSPORT` after the configured `unavailableFailureThreshold` consecutive scheduled detector cycles with primary-link `tx/rx` error growth
- **AND** it SHALL clear the COMM fault on the first scheduled cycle where the current primary link is available and has no new `tx/rx` error growth

#### Scenario: COMM recovery action stays bounded
- **WHEN** `RecoveryExecutor` performs COMM recovery in this change
- **THEN** the real action SHALL be bounded to failover, session revoke, and downlink-owner clear as needed
- **AND** it SHALL NOT claim a new COMM hardware reset plane, RF recovery, or automatic nominal-link restore in this change

### Requirement: TtcPassManager Owns TTC Pass Policy
The mission-autonomy capability SHALL provide a focused `TtcPassManager` owner for TTC pass-window automation, and the active runtime SHALL keep that owner separate from `ModeManager`, `ModeSafetyController`, `CommController`, and `RecoveryExecutor`.

#### Scenario: Active topology instantiates a focused TTC policy owner
- **WHEN** the ttc-pass-window-mode-v1 runtime is built
- **THEN** the active topology SHALL instantiate, configure, and schedule `TtcPassManager`
- **AND** it SHALL NOT move TTC pass-window ownership into `ModeManager`, `ModeSafetyController`, `CommController`, or the retired `MissionExecutive`

#### Scenario: TTC policy owner consumes bounded provider truth only
- **WHEN** `TtcPassManager` evaluates TTC policy
- **THEN** it SHALL use current mode, cached GPS state, COMM availability, configured TTC policy state, and component time for bounded freshness/timeout age
- **AND** it SHALL NOT consume scenario truth directly
- **AND** it SHALL NOT command payload execution, generic scheduler actions, ADCS pointing, COMM session redesign, or broader FDIR behavior

### Requirement: TTC Pass Policy Uses Normal Mode Infrastructure
The mission-autonomy capability SHALL keep TTC pass policy on the normal runtime mode path instead of introducing a parallel TTC mode store.

#### Scenario: TTC policy requests mode changes through the normal internal path
- **WHEN** `TtcPassManager` decides that TTC should enter or exit
- **THEN** it SHALL request mode changes through the existing internal mode-control apply path
- **AND** it SHALL use a distinct TTC policy source tag

#### Scenario: Safety and recovery remain authoritative
- **WHEN** `ModeSafetyController` or `RecoveryExecutor` has already moved the runtime out of `TTC`
- **THEN** `TtcPassManager` SHALL observe that current mode change and clear its internal TTC policy bookkeeping
- **AND** it SHALL NOT restore `TTC` unless the normal TTC entry guards are later satisfied from `IDLE`

### Requirement: TTC Pass Policy Boundaries Stay Narrow
The mission-autonomy capability SHALL keep TTC pass policy bounded to one pass window and explicit exit/entry guards.

#### Scenario: TTC policy is not a generic scheduler
- **WHEN** the ttc-pass-window-mode-v1 change is implemented
- **THEN** `TtcPassManager` SHALL manage at most one configured pass window at a time
- **AND** it SHALL NOT claim queued activities, command payload execution, generalized time-tagged scheduling, or broad mission planning

#### Scenario: TTC policy defers ADCS pointing
- **WHEN** the ttc-pass-window-mode-v1 change is implemented
- **THEN** `TtcPassManager` SHALL NOT claim ADCS ground-tracking ownership or emit ADCS pointing requests in this change

### Requirement: RecoveryExecutor Persists Shared Recovery Lifecycle Breadcrumbs
The mission-autonomy capability SHALL make `RecoveryExecutor` append persistent
fault ring breadcrumbs for shared recovery incident open, action request,
action execution, reboot pending, reboot issued, and incident clear lifecycle
transitions.

#### Scenario: First shared incident opening records lifecycle start
- **WHEN** `RecoveryExecutor` accepts a new shared recovery incident for an
  active source in this change
- **THEN** it SHALL append an incident-open breadcrumb with the bounded source,
  level, and incident epoch context
- **AND** later action selection SHALL append action-requested and
  action-executed breadcrumbs as that incident progresses

#### Scenario: Reboot escalation records pending and issued transitions
- **WHEN** shared recovery reaches reboot intent and later issues the reboot-
  equivalent action
- **THEN** `RecoveryExecutor` SHALL append distinct reboot-pending and reboot-
  issued breadcrumbs
- **AND** those breadcrumbs SHALL remain separate from `BootManager`'s later
  boot-observed truth after relaunch

#### Scenario: Explicit clear records incident closure
- **WHEN** a detector later clears an active incident through the shared
  recovery path
- **THEN** `RecoveryExecutor` SHALL append an incident-cleared breadcrumb for
  that incident epoch
- **AND** it SHALL NOT infer closure only from mode changes or reboot outcomes

### Requirement: Detector-Local Owners Do Not Duplicate Persistent Writes
The mission-autonomy capability SHALL keep detector-local fault ownership in
`WatchdogSupervisor`, `EpsFdirController`, `AdcsFdirController`, and
`CommController` while routing shared persistent breadcrumb writes through
`RecoveryExecutor` only.

#### Scenario: Detector surfaces stay detector-local
- **WHEN** watchdog, EPS, ADCS, or COMM detector logic raises or clears a shared
  recovery incident
- **THEN** the detector SHALL continue to use its existing detector-local public
  contract
- **AND** it SHALL NOT append a parallel detector-owned persistent fault record
  directly in v1

### Requirement: RecoveryExecutor Executes R2 Process Restart
The mission-autonomy capability SHALL make `RecoveryExecutor` execute a real managed OBC process restart for current `R2_RESTART_SOFTWARE_COMPONENT` recovery sources instead of stopping at process-restart intent reporting.

#### Scenario: Current R2 sources request managed process restart
- **WHEN** a watchdog-source stale fault enters shared recovery at `R2_RESTART_SOFTWARE_COMPONENT`
- **THEN** `RecoveryExecutor` SHALL persist recovery metadata for that source and level before requesting runtime exit
- **AND** it SHALL mark the active recovery action as `PROCESS_RESTART`
- **AND** it SHALL NOT mark that R2 action as `OBC_REBOOT`

#### Scenario: ADCS scheduled-poll faults start at subsystem reset
- **WHEN** an ADCS scheduled-poll transport or freshness fault first enters shared recovery
- **THEN** `RecoveryExecutor` SHALL start the incident at `R3_RESET_SUBSYSTEM_INTERFACE`
- **AND** it SHALL request the ADCS subsystem-interface reset action before any reboot-equivalent escalation

#### Scenario: Runtime exit distinguishes R2 from R6
- **WHEN** the active runtime consumes a pending `PROCESS_RESTART` request from `RecoveryExecutor`
- **THEN** the runtime SHALL exit with the bounded process-restart exit code
- **AND** R6 reboot-equivalent requests SHALL continue to use the existing reboot-equivalent exit code

#### Scenario: Repeated-recovery clamp prevents R2 restart loops
- **WHEN** boot metadata reports that boot-after-recovery safe fallback is required
- **AND** a current watchdog R2 source opens or relatches a shared recovery incident
- **THEN** `RecoveryExecutor` SHALL record the incident and hold or request the safe fallback path
- **AND** it SHALL NOT queue another R2 process restart before the runtime has been acknowledged stable

#### Scenario: R2 state is reported separately from reboot state
- **WHEN** `RecoveryExecutor` reports runtime recovery status
- **THEN** process-restart pending and process-restart count SHALL be observable separately from reboot pending and reboot count
- **AND** R2 process restart SHALL NOT increment the R6 reboot counter

### Requirement: Target Hardware Watchdog Feed Uses Existing WatchdogSupervisor Truth

The mission-autonomy capability SHALL keep `WatchdogSupervisor` as the single
owner of watchdog feed eligibility, and the Raspberry Pi hardware watchdog path
SHALL consume that existing truth instead of introducing a second target-only
watchdog policy owner.

#### Scenario: Target hardware stroking follows supervisor feed eligibility
- **WHEN** the active Raspberry Pi baseline enables hardware watchdog mode
- **THEN** only `WatchdogSupervisor` feed-eligible cycles SHALL stroke the
  hardware watchdog device
- **AND** target integration SHALL NOT bypass `WatchdogSupervisor` with an
  independent timer or separate feed policy

### Requirement: Watchdog-Source R6 Uses Hardware Reset On Enabled Target Mode

The mission-autonomy capability SHALL allow watchdog-source `R6_OBC_REBOOT` to
use Raspberry Pi hardware watchdog timeout on the governed enabled target mode
while preserving existing process-restart and non-watchdog reboot semantics.

#### Scenario: Watchdog-source R6 does not exit 32 in hardware-watchdog mode
- **WHEN** hardware watchdog mode is enabled on the active Raspberry Pi target
- **AND** a watchdog-source incident reaches `R6_OBC_REBOOT`
- **THEN** `RecoveryExecutor` SHALL persist reboot intent and recovery metadata
- **AND** it SHALL stop further watchdog stroking
- **AND** it SHALL NOT request runtime exit code `32` for that watchdog-source
  incident

#### Scenario: Existing R2 and non-watchdog R6 semantics remain intact
- **WHEN** an `R2_RESTART_SOFTWARE_COMPONENT` incident is raised
- **THEN** the runtime SHALL still request exit `31`
- **AND** watchdog hardware mode SHALL NOT change that behavior
- **WHEN** a non-watchdog incident reaches `R6_OBC_REBOOT`
- **THEN** the runtime SHALL still request exit `32`

### Requirement: Proof Trigger For Target Hardware Reset Remains Bounded

The mission-autonomy capability SHALL allow a bounded proof-only watchdog
suppression trigger for repository-owned target evidence when the active target
topology lacks a clean natural stale-source trigger.

#### Scenario: Proof trigger is not generalized into operator recovery control
- **WHEN** the repository records target hardware watchdog reset evidence
- **THEN** it MAY use a bounded proof-only trigger to suppress a watchdog source
  heartbeat path
- **AND** that trigger SHALL remain scoped to repository-owned proof workflows
- **AND** it SHALL NOT become a generic operator-driven restart or reboot
  command surface in this change

### Requirement: TTC Policy Owns One-Way ADCS Entry Trigger Only

The mission-autonomy capability SHALL allow `TtcPassManager` to issue one
best-effort internal ADCS entry trigger on TTC auto-entry while keeping TTC
pass policy separate from broader ADCS or mission-execution ownership.

#### Scenario: TTC auto-entry sends one ADCS entry trigger
- **WHEN** `TtcPassManager` transitions from non-`TTC` into `TTC` through its
  pass-window policy path
- **THEN** it SHALL send at most one internal ADCS entry trigger for that TTC
  entry edge
- **AND** repeated scheduler ticks while already in `TTC` SHALL NOT resend that
  trigger

#### Scenario: ADCS failure does not block TTC policy entry
- **WHEN** `TtcPassManager` determines that TTC entry should occur
- **AND** the ADCS entry trigger fails
- **THEN** `TtcPassManager` SHALL still request TTC through its owned internal
  mode-control path
- **AND** it SHALL keep the ADCS failure as best-effort observability only

#### Scenario: TTC exit does not restore prior ADCS mode in this slice
- **WHEN** `TtcPassManager` later exits `TTC`
- **THEN** it SHALL NOT automatically restore a previous ADCS mode as part of
  this change
- **AND** any later ADCS restore policy SHALL require a separate governed
  design

### Requirement: Current Route Closure May Reuse COMM Primary-Unavailable Recovery As Autonomous UHF Promotion

The mission-autonomy baseline SHALL treat the existing
`COMM_PRIMARY_UNAVAILABLE` detector plus executor-owned COMM recovery action as
the maintained autonomous promotion path for current target route closure.

#### Scenario: Detector truth remains separate from action ownership
- **WHEN** the current primary COMM link becomes unavailable
- **THEN** `CommController` SHALL continue to own only the detector fault truth
- **AND** the current route closure SHALL cite `RecoveryExecutor` as the owner
  of the actual failover actuation.

#### Scenario: Current route closure does not replace autonomous failover with manual switch
- **WHEN** current target evidence is recorded for Route 2, Route 3, or the
  maintained failover proof family
- **THEN** it SHALL use the detector-triggered recovery path as the current
  autonomous UHF promotion truth
- **AND** it SHALL NOT restate a manual `COMM_SET_ACTIVE(UHF)` operator step as
  the maintained failover boundary.
