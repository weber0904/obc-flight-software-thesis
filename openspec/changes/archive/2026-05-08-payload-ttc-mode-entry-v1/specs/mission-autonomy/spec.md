## ADDED Requirements

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
- **THEN** the request SHALL be rejected with reason code `SOC_RECOVERY_GUARD_NOT_MET`

#### Scenario: HELL to SAFE below threshold is rejected
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **AND** cached EPS SoC is less than `15%`
- **THEN** the request SHALL be rejected with reason code `SOC_RECOVERY_GUARD_NOT_MET`

#### Scenario: HELL to SAFE without EPS cache is rejected
- **WHEN** the current mode is `HELL`
- **AND** an operator requests `SAFE`
- **AND** cached EPS status is unavailable
- **THEN** the request SHALL be rejected with reason code `SOC_RECOVERY_GUARD_UNAVAILABLE`

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

## MODIFIED Requirements

### Requirement: Mode Safety Controller Owns V1 SoC Fallback
The mission-autonomy capability SHALL provide a narrow active `ModeSafetyController` component that owns the v1 cached-EPS SoC safety fallback policy for the `SAFE`, `HELL`, `IDLE`, `PAYLOAD`, and `TTC` mode model, and it SHALL also own cached-EPS safety guard decisions needed by operator `HELL -> SAFE` recovery.

#### Scenario: Safety policy is active without reviving MissionExecutive
- **WHEN** the payload-ttc-mode-entry-v1 runtime is built
- **THEN** the active topology SHALL instantiate, configure, and schedule `ModeSafetyController`
- **AND** the active topology SHALL NOT instantiate, configure, schedule, or bind the provisional `MissionExecutive` as the owner of this policy

#### Scenario: Cached EPS status is the only policy input
- **WHEN** `ModeSafetyController` evaluates safety fallback or cached-EPS operator safety guard behavior
- **THEN** it SHALL use cached EPS state of charge from the runtime EPS status provider and the current mode from the runtime mode-control provider
- **AND** it SHALL NOT consume scenario truth directly, command ADCS, command EPS load shedding, command COMM behavior, schedule payload or TTC activity, or perform watchdog, subsystem timeout, retry, reset, or broader FDIR actions

#### Scenario: EPS alarms do not bypass the mode safety policy
- **WHEN** EPS state of charge is in the critical band used by `ModeSafetyController`
- **THEN** the EPS status bridge SHALL continue to publish EPS telemetry and EPS alarm events without aborting the hosted runtime
- **AND** any primary-mode fallback SHALL be requested by `ModeSafetyController` through the normal internal safety mode-control runtime path
