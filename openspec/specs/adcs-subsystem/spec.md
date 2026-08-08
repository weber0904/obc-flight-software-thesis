# adcs-subsystem Specification

## Purpose
Define the ADCS simulator scope, `AdcsBridge` behavior, acceptance constants, and the owned ADCS public contracts for the first version.
## Requirements
### Requirement: ADCS Simulator Scope
The ADCS subsystem SHALL model quaternion attitude, angular velocity, the first-version sensor set, the first-version actuator set, and the `IDLE`, `DETUMBLE`, and `POINTING` control modes while operating as CSP node `3`.

#### Scenario: ADCS control modes remain available
- **WHEN** a user selects `IDLE`, `DETUMBLE`, or `POINTING`
- **THEN** the subsystem SHALL expose that mode through its control interface and state reporting

### Requirement: ADCS Public Contract
The ADCS subsystem SHALL own the `ADCS_*` command, telemetry, and event families, including the mode, target, attitude query, calibration commands, quaternion and angular-rate telemetry, and mode-change, detumble-complete, pointing-acquired, sensor-fault, and comm-error events.

#### Scenario: ADCS attitude is observable
- **WHEN** an operator issues `ADCS_GET_ATTITUDE` or the bridge performs scheduled polling
- **THEN** the subsystem SHALL make the owned quaternion, angular-rate, and pointing telemetry available for observation

### Requirement: ADCS Acceptance Constants
The ADCS subsystem SHALL treat detumble and pointing thresholds as mission constants, with first-version defaults of angular-velocity norm `< 0.05 rad/s` for detumble and pointing error `< 5 deg` for pointing acquisition.

#### Scenario: Default ADCS thresholds are applied
- **WHEN** the first-version acceptance baseline is evaluated without an overriding change
- **THEN** the subsystem SHALL use the documented default detumble and pointing thresholds

### Requirement: ADCS Bridge Fallback Behavior
`AdcsBridge` SHALL poll on a schedule, SHALL retain the last valid state when replies are invalid or absent, SHALL raise the appropriate comms or sensor-fault path instead of emitting random replacement values, and SHALL keep shared ADCS FDIR input limited to scheduled poll health.

#### Scenario: Invalid ADCS reply during scheduled polling preserves FDIR boundary
- **WHEN** `AdcsBridge` receives an invalid or incomplete ADCS reply during scheduled polling
- **THEN** it SHALL preserve the last valid state and signal the degraded condition through the owned fault path
- **AND** it SHALL update only the scheduled ADCS poll-health state that the bounded ADCS FDIR detector consumes

### Requirement: ADCS CSP Host Protocol
The ADCS subsystem SHALL define ADCS-owned CSP service request and response payloads for the hosted simulator and the OBC-side bridge, and those payloads SHALL preserve the formal node `3` contract while avoiding libcsp reserved service ports `0` through `3`. ADCS runtime DTO and result semantics SHALL remain ADCS-owned support types and SHALL NOT be treated as a shared cross-subsystem on-wire protocol.

#### Scenario: ADCS request includes a sequence and returns one state payload
- **WHEN** the hosted bridge requests ADCS state
- **THEN** the protocol SHALL carry a sequence field and SHALL return the required ADCS state in a single response payload

#### Scenario: ADCS reset uses an owned non-reserved service port
- **WHEN** the active hosted ADCS CSP protocol exposes a recovery reset service
- **THEN** that service SHALL use an ADCS-owned application port outside libcsp reserved ports `0` through `3`
- **AND** it SHALL remain distinct from the existing ADCS state, mode, target, and calibration services

#### Scenario: ADCS does not retain a direct ZMQ fallback
- **WHEN** the ADCS hosted simulator or `AdcsBridge` default transport is inspected
- **THEN** the active implementation SHALL use the ADCS-owned CSP protocol over libcsp and SHALL NOT retain a direct ZMQ request/reply compatibility transport for ADCS business traffic

### Requirement: Hosted ADCS Simulator
The hosted ADCS implementation SHALL provide a simulator executable that serves state, mode, target, calibration, and recovery-reset services as libcsp node `3` over the hosted ZMQHUB-backed internal CSP substrate while maintaining seeded deterministic pseudo-noise and time-continuous quaternion, angular-rate, pointing-error, and sensor-validity state.

#### Scenario: Mode command changes simulator behavior
- **WHEN** a client issues `ADCS_SET_MODE`
- **THEN** the simulator SHALL update the commanded mode
- **AND** it SHALL reflect that new mode's time-continuous dynamics in subsequent status responses

#### Scenario: Target command changes simulator target state
- **WHEN** a client issues `ADCS_SET_TARGET`
- **THEN** the simulator SHALL update the owned target quaternion used for
  pointing-error evaluation
- **AND** it SHALL keep the synthetic pointing-pass profile as the current
  attitude-motion owner for the hosted demo path

#### Scenario: Calibration command is served over internal CSP
- **WHEN** a client issues an ADCS calibration request over the hosted internal CSP substrate
- **THEN** the simulator SHALL acknowledge the request
- **AND** it SHALL preserve deterministic bounded state reporting after the calibration service returns

#### Scenario: Reset command restores the deterministic default ADCS state
- **WHEN** a client issues an ADCS reset request over the hosted internal CSP substrate
- **THEN** the simulator SHALL restore the same default mode, quaternion, target, angular-rate, and derived state that `loadDefaults_()` provides on startup
- **AND** the restored state SHALL include its sensor-validity state and derived pointing/magnetometer fields
- **AND** it SHALL return that restored state through the normal ADCS state reply shape

### Requirement: AdcsBridge Public Behavior
`AdcsBridge` SHALL own the `ADCS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, and SHALL preserve the last valid state when replies are invalid or absent while raising the owned comms or sensor-fault paths.

#### Scenario: Invalid sensor reply preserves the last valid state
- **WHEN** the simulator reports an invalid sensor sample
- **THEN** `AdcsBridge` SHALL emit the sensor-fault behavior and SHALL NOT overwrite the last valid ADCS telemetry with invalid values

#### Scenario: Deployed ADCS CSP transport uses owner-managed runtime access
- **WHEN** the deployed `AdcsBridge` binds its owned CSP transport
- **THEN** it SHALL use the topology-owned runtime owner rather than directly binding to the global shared runtime
- **AND** that ownership change SHALL NOT alter the existing `ADCS_*` public contract or scheduled poll-health semantics by itself

### Requirement: ADCS Convergence Signaling
The first ADCS implementation slice SHALL evaluate the mission-constant detumble and pointing thresholds and SHALL surface the owned completion/acquired events when those thresholds are met.

#### Scenario: Detumble threshold is reached
- **WHEN** the simulated angular-velocity norm drops below the detumble threshold
- **THEN** `AdcsBridge` SHALL raise `ADCS_DETUMBLE_COMPLETE`

### Requirement: Scenario-Seeded ADCS Deployment Rates
The hosted ADCS simulator SHALL accept scenario-seeded deployment-rate angular velocity inputs during scenario initialization, and the simulator SHALL preserve its own control-responsive dynamics after that initialization instead of being continuously overwritten by replay.

#### Scenario: Detumble remains control-responsive after scenario seeding
- **WHEN** the scenario bridge seeds the hosted ADCS simulator with deployment-rate angular velocity and the OBC later commands `DETUMBLE`
- **THEN** the hosted ADCS simulator SHALL converge according to its own time-continuous control-response model rather than being forced back to the seeded angular rate on each replay step

### Requirement: ADCS Mode Dynamics Stay Time-Continuous

The hosted ADCS simulator SHALL evolve `IDLE`, `DETUMBLE`, and `POINTING`
behavior from monotonic time rather than by applying one fixed state jump per
request.

#### Scenario: Idle mode remains bounded but non-static
- **WHEN** the simulator remains in `IDLE` mode over successive
  time-separated state reads
- **THEN** quaternion and angular-rate values SHALL remain bounded near the
  idle baseline
- **AND** they SHALL exhibit small seeded deterministic variation instead of
  staying exactly fixed

#### Scenario: Detumble mode decays toward the mission threshold
- **WHEN** the simulator remains in `DETUMBLE` mode over successive
  time-separated state reads
- **THEN** the angular-rate norm SHALL decay toward the configured mission
  threshold
- **AND** the simulator SHALL preserve bounded quaternion normalization while
  that decay occurs

### Requirement: Pointing Mode Uses A Synthetic Pass Profile

The hosted ADCS simulator SHALL model `POINTING` as a repeating synthetic
pointing pass that sweeps a bounded roll, pitch, and yaw trajectory over a
fixed `60 s` cycle.

#### Scenario: Entering pointing starts the synthetic pass
- **WHEN** the simulator receives `ADCS_SET_MODE(POINTING)` from any
  non-pointing mode
- **THEN** it SHALL reset the pointing-pass phase to the start of the fixed
  `60 s` synthetic pass
- **AND** subsequent state replies SHALL track the synthetic pass attitude
  with bounded angular-rate output

#### Scenario: Ongoing pointing does not reset on redundant mode command
- **WHEN** the simulator is already in `POINTING` mode and receives another
  pointing-mode command without leaving that mode first
- **THEN** it SHALL keep the current synthetic pass phase
- **AND** it SHALL continue the existing pass instead of rewinding
  automatically

### Requirement: ADCS Pointing Pass Can Be Restarted Externally

The hosted ADCS simulator SHALL expose a simulator-owned external control that
restarts the synthetic pointing-pass phase without creating a new OBC command.

#### Scenario: Restart control rewinds the pointing pass
- **WHEN** a repo-owned helper sends a valid `restart-pointing-pass` control
  request while the simulator remains online
- **THEN** the simulator SHALL keep serving on the same ADCS CSP node
- **AND** it SHALL reset the synthetic pointing-pass phase to the start of the
  fixed `60 s` cycle for subsequent state replies

#### Scenario: Control surface remains separate from OBC command ownership
- **WHEN** reviewers inspect the active hosted pointing-pass replay path
- **THEN** the restart control SHALL remain simulator-owned and externally
  injected
- **AND** the OBC SHALL NOT gain a new public ADCS command for replaying the
  pointing pass

### Requirement: ADCS Scheduled Poll Health Is Reviewable
The ADCS subsystem SHALL expose bounded scheduled-poll health state separate from local command-path outcomes.

#### Scenario: Scheduled ADCS poll health tracks transport and valid-refresh separately
- **WHEN** `AdcsBridge.schedIn` performs the active scheduled poll
- **THEN** it SHALL update bounded ADCS poll-health truth that distinguishes scheduled transport failure from scheduled no-valid-refresh behavior
- **AND** it SHALL preserve the last valid state when the scheduled poll does not produce a valid refresh

#### Scenario: Local command-path failures stay local
- **WHEN** an operator command drives ADCS transport traffic outside the scheduled poll path
- **THEN** `AdcsBridge` MAY emit local `ADCS_COMM_ERROR` or `ADCS_SENSOR_FAULT`
- **AND** it SHALL NOT treat that command-path failure as a scheduled ADCS poll-health transition

### Requirement: ADCS Shared Recovery Incidents Are Bounded
The ADCS subsystem SHALL support one bounded ADCS FDIR detector that normalizes scheduled poll-health failures into shared recovery incidents without making every local sensor fault a shared FDIR fault.

#### Scenario: Single invalid sample remains local
- **WHEN** a scheduled ADCS poll returns one invalid sensor sample with `sensor_valid = 0`
- **THEN** the active runtime SHALL preserve the last valid ADCS state
- **AND** it SHALL emit the owned local sensor-fault observability
- **AND** it SHALL NOT enter shared ADCS recovery until the scheduled no-valid-refresh threshold is met

#### Scenario: Scheduled thresholds drive shared recovery
- **WHEN** the ADCS FDIR detector evaluates scheduled poll health in this change
- **THEN** it SHALL latch `ADCS_POLL_TRANSPORT` after `3` consecutive scheduled transport failures
- **AND** it SHALL latch `ADCS_POLL_FRESHNESS` after `3` consecutive scheduled no-valid-refresh cycles
- **AND** it SHALL clear the active ADCS detector fault on the first scheduled healthy valid cycle

#### Scenario: Shared ADCS recovery starts with subsystem reset
- **WHEN** the active runtime opens an `ADCS_POLL_TRANSPORT` or `ADCS_POLL_FRESHNESS` shared recovery incident for the first time
- **THEN** the incident SHALL begin at `R3_RESET_SUBSYSTEM_INTERFACE`
- **AND** the first shared recovery action SHALL be the ADCS subsystem CSP reset service rather than process restart persistence
- **AND** higher-level relatch escalation MAY continue through the existing shared recovery logic after that first ADCS reset action

### Requirement: Scheduled ADCS Live Visibility Is Summary-Oriented

The ADCS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit ADCS state or
control commands.

#### Scenario: Scheduled ADCS refresh keeps summary telemetry and transition events
- **WHEN** `AdcsBridge` performs a scheduled ADCS state refresh
- **THEN** it SHALL keep only the selected ADCS summary telemetry as baseline
  live visibility
- **AND** it SHALL keep mode-change, detumble-complete, pointing-acquired,
  sensor-fault, and comm-error events reviewable.

#### Scenario: Explicit ADCS readback remains fresh and detailed
- **WHEN** `ADCS_GET_ATTITUDE`, `ADCS_SET_MODE`, `ADCS_SET_TARGET`, or
  `ADCS_CALIBRATE` obtains a fresh ADCS reply
- **THEN** the subsystem SHALL update the owned ADCS cache and transition
  logic through the normal apply path
- **AND** it SHALL make the detailed ADCS telemetry reviewable as bounded
  readback for that explicit interaction.

### Requirement: TTC Auto-Entry Can Trigger ADCS Pointing

The ADCS subsystem SHALL support a best-effort internal runtime mode-set path
that `TtcPassManager` can use to request `POINTING` when TTC auto-entry occurs.

#### Scenario: TTC entry requests ADCS pointing through existing runtime mode surface
- **WHEN** `TtcPassManager` completes a TTC auto-entry request on the active
  runtime
- **THEN** it SHALL be able to request ADCS mode `POINTING` through the
  existing runtime mode-set path owned by `AdcsBridge`

#### Scenario: ADCS request failure does not redefine TTC entry success
- **WHEN** the TTC-triggered ADCS `POINTING` request cannot be completed
- **THEN** the active runtime SHALL keep TTC entry ownership and verdict on the
  TTC policy path
- **AND** the ADCS failure SHALL remain reviewable as a bounded warning or
  degraded condition rather than forcing TTC entry failure by itself
