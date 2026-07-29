## ADDED Requirements

### Requirement: Mode Safety Controller Owns V1 SoC Fallback
The mission-autonomy capability SHALL provide a narrow active `ModeSafetyController` component that owns the v1 cached-EPS SoC safety fallback policy for the `SAFE`, `HELL`, `IDLE`, `PAYLOAD`, and `TTC` mode model.

#### Scenario: Safety policy is active without reviving MissionExecutive
- **WHEN** the mode-safety-policy-v1 runtime is built
- **THEN** the active topology SHALL instantiate, configure, and schedule `ModeSafetyController`
- **AND** the active topology SHALL NOT instantiate, configure, schedule, or bind the provisional `MissionExecutive` as the owner of this policy

#### Scenario: Cached EPS status is the only policy input
- **WHEN** `ModeSafetyController` evaluates safety fallback
- **THEN** it SHALL use cached EPS state of charge from the runtime EPS status provider and the current mode from the runtime mode-control provider
- **AND** it SHALL NOT consume scenario truth directly, command ADCS, command EPS load shedding, command COMM behavior, schedule payload or TTC activity, or perform watchdog, subsystem timeout, retry, reset, or broader FDIR actions

#### Scenario: EPS alarms do not bypass the mode safety policy
- **WHEN** EPS state of charge is in the critical band used by `ModeSafetyController`
- **THEN** the EPS status bridge SHALL continue to publish EPS telemetry and EPS alarm events without aborting the hosted runtime
- **AND** any primary-mode fallback SHALL be requested by `ModeSafetyController` through the normal mode-control runtime path

### Requirement: Strict SoC Fallback Thresholds
The mission-autonomy capability SHALL apply strict, deterministic SoC fallback thresholds without runtime configurability in v1.

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
The mission-autonomy capability SHALL keep recovery from `SAFE` to `IDLE` manual in v1.

#### Scenario: High SoC does not automatically restore IDLE
- **WHEN** the current mode is `SAFE`
- **AND** cached EPS SoC is above `50%`
- **THEN** `ModeSafetyController` SHALL NOT request `IDLE`
- **AND** SoC recovery above `50%` SHALL remain only a future configurability boundary

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

## MODIFIED Requirements

### Requirement: Mission Executive Redesign Is Deferred
The mission-autonomy capability SHALL keep the existing provisional `MissionExecutive` low-battery, sun-safe, and detumble behavior retired from the active runtime baseline unless a later governed change explicitly redesigns and verifies that broader policy.

#### Scenario: Mode safety policy does not restore retired autonomy
- **WHEN** mode-safety-policy-v1 is complete
- **THEN** the active runtime SHALL NOT depend on the provisional `MissionExecutive` to command `HELL`, `SAFE`, `IDLE`, `PAYLOAD`, `TTC`, ADCS pointing, ADCS detumble, load shedding, COMM behavior, scheduler behavior, watchdog behavior, subsystem timeout/retry/reset behavior, or broader FDIR actions
- **AND** later autonomy changes SHALL define their own policy, thresholds, runtime owner, and verification evidence before claiming those behaviors
