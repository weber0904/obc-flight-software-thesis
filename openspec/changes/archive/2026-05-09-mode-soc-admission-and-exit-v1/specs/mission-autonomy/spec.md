## ADDED Requirements

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

## MODIFIED Requirements

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
