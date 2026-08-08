# eps-subsystem Specification

## Purpose
Define the EPS simulator scope, `EpsBridge` behavior, and the owned EPS public contracts for the first version.
## Requirements
### Requirement: EPS Simulator Scope
The EPS subsystem SHALL model battery, solar, and 8-channel PDU state, SHALL operate as CSP node `2`, and SHALL expose status, PDU, config, and reset services over EPS-owned CSP application ports `10` through `13`.

#### Scenario: EPS status fits a single response
- **WHEN** `EpsBridge` requests EPS status
- **THEN** the simulator SHALL return the required summary state in a single CSP response payload that includes a sequence field

### Requirement: EPS Public Contract
The EPS subsystem SHALL own the `EPS_*` command, telemetry, and event families, including `EPS_GET_STATUS`, `EPS_SET_PDU`, `EPS_SET_HEATER`, `EPS_RESET`, the required telemetry fields for battery/solar/PDU state, and the low-battery, over-temperature, PDU-change, and comm-error events.

#### Scenario: EPS command updates observable state
- **WHEN** an operator issues `EPS_SET_PDU`
- **THEN** the subsystem SHALL update the PDU state and surface the change through the owned telemetry and event contracts

### Requirement: EPS Bridge Fallback Behavior
`EpsBridge` SHALL poll for state on a schedule, SHALL update telemetry only from valid replies, and SHALL preserve the last valid state or explicit defaults during comms failure instead of generating random replacement values.

#### Scenario: EPS comms timeout
- **WHEN** the simulator does not respond within the configured timeout window
- **THEN** `EpsBridge` SHALL raise the comms error path and SHALL NOT replace EPS telemetry with arbitrary data

### Requirement: EPS CSP Host Protocol
The EPS subsystem SHALL define EPS-owned CSP service request and response payloads for the hosted simulator and OBC-side bridge, and those payloads SHALL preserve the formal node `2` contract while avoiding libcsp reserved service ports `0` through `3`. EPS runtime DTO and result semantics SHALL remain EPS-owned support types and SHALL NOT be treated as a shared cross-subsystem on-wire protocol.

#### Scenario: EPS request includes a sequence and returns one status payload
- **WHEN** the hosted bridge requests EPS status
- **THEN** the protocol SHALL carry a sequence field and SHALL return the full EPS status in a single response payload

#### Scenario: EPS does not retain a direct ZMQ fallback
- **WHEN** the EPS hosted simulator or `EpsBridge` default transport is inspected
- **THEN** the active implementation SHALL use the EPS-owned CSP protocol over libcsp and SHALL NOT retain a direct ZMQ request/reply compatibility transport for EPS business traffic

### Requirement: Hosted EPS Simulator
The hosted EPS implementation SHALL provide a simulator executable that serves the EPS status, PDU, config, and reset services as libcsp node `2` over the hosted ZMQHUB-backed internal CSP substrate while maintaining seeded deterministic pseudo-noise, time-driven battery and solar evolution, heater state, and 8-channel PDU state.

#### Scenario: PDU command updates simulator state
- **WHEN** a client issues `EPS_SET_PDU`
- **THEN** the simulator SHALL update the requested PDU channel
- **AND** it SHALL reflect the new aggregate PDU state in the next status response
- **AND** mapped channels `0`, `1`, `3`, `5`, and `6` SHALL contribute their configured load weights while unmapped channels `2`, `4`, and `7` SHALL remain zero-load spare channels

### Requirement: EpsBridge Public Behavior
`EpsBridge` SHALL own the `EPS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, SHALL preserve the last valid state on comms failure while raising the EPS comms error path, and SHALL maintain deterministic runtime poll-health state for repeated poll failure handling.

#### Scenario: Scheduled EPS timeout preserves last valid telemetry but invalidates cache
- **WHEN** `EpsBridge` fails to receive a valid simulator response during the scheduled EPS status poll timeout window
- **THEN** it SHALL emit the comms error behavior
- **AND** it SHALL NOT overwrite EPS telemetry with arbitrary replacement values
- **AND** it SHALL invalidate cached EPS status for runtime cache consumers
- **AND** it SHALL advance the owned poll-health counters for that failure

#### Scenario: Deployed EPS CSP transport uses owner-managed runtime access
- **WHEN** the deployed `EpsBridge` binds its owned CSP transport
- **THEN** it SHALL use the topology-owned runtime owner rather than directly binding to the global shared runtime
- **AND** that ownership change SHALL NOT alter the existing `EPS_*` public contract or poll-health semantics by itself

### Requirement: EPS Threshold Event Mapping
`EpsBridge` SHALL map simulator status into the subsystem-owned alarm events, including low battery, critical battery, over-temperature, and PDU-change reporting.

#### Scenario: Low battery status raises the subsystem event
- **WHEN** a valid EPS status reports state-of-charge below `20%`
- **THEN** `EpsBridge` SHALL raise `EPS_LOW_BATTERY`

### Requirement: Scenario-Driven EPS Replay Inputs
The hosted EPS simulator SHALL accept scenario-driven sunlight and battery state-of-charge inputs from the repository-owned scenario bridge while preserving the existing PDU, heater, status-command, and runtime load-mode behavior.

#### Scenario: Scenario replay updates EPS environment state
- **WHEN** the scenario bridge applies a replay sample with new sunlight or battery state-of-charge values
- **THEN** the hosted EPS simulator SHALL reflect those values in subsequent status responses
- **AND** it SHALL preserve the current commandable PDU and heater behavior
- **AND** it SHALL continue evolving the remaining load-derived fields from its time-driven model

### Requirement: EPS Runtime Poll Health Contract
The EPS subsystem SHALL provide a deterministic runtime poll-health contract alongside the existing cached EPS status behavior so downstream runtime owners can distinguish healthy polling, transient poll failure, and repeated poll failure without inventing transport-side policy.

#### Scenario: Successful poll resets EPS poll-health state
- **WHEN** `EpsBridge` completes a scheduled EPS status poll successfully
- **THEN** it SHALL mark the last poll result as successful
- **AND** it SHALL reset the consecutive poll-failure count to `0`
- **AND** it SHALL keep the cumulative poll comm-error count unchanged for that cycle

#### Scenario: Failed poll advances EPS poll-health state
- **WHEN** `EpsBridge` fails to complete a scheduled EPS status poll because of timeout or equivalent transport failure
- **THEN** it SHALL mark the last poll result as unsuccessful
- **AND** it SHALL increment the consecutive poll-failure count
- **AND** it SHALL increment the cumulative poll comm-error count

#### Scenario: Failed poll still invalidates cached EPS status
- **WHEN** `EpsBridge` fails to complete a scheduled EPS status poll
- **THEN** it SHALL treat the cached EPS status as unavailable until a later successful status update
- **AND** it SHALL NOT leave the cache marked valid for runtime consumers

#### Scenario: Live status timeout does not perturb poll-health counters
- **WHEN** a runtime consumer performs a live EPS status read and that transport request times out
- **THEN** `EpsBridge` SHALL still emit the comms error behavior
- **AND** it SHALL invalidate cached EPS status for runtime cache consumers
- **AND** it SHALL leave the owned poll-health counters unchanged because v1 timeout FDIR is driven only by scheduled poll outcomes so operator or probe reads cannot perturb the deterministic retry and escalation boundary

#### Scenario: Runtime health snapshot is available after any poll attempt
- **WHEN** a runtime consumer requests EPS poll-health state from `EpsBridge`
- **THEN** it SHALL receive a project-owned health snapshot that includes cache-validity, last-poll-success, consecutive poll failures, and cumulative poll comm errors

### Requirement: EPS Timeout Recovery Ownership Stays Shared
The EPS subsystem SHALL keep `EpsBridge` as the EPS poll-health owner and `EpsFdirController` as the EPS timeout detector while leaving later recovery action ownership inside the shared executor.

#### Scenario: EPS timeout detector does not regain direct recovery ownership
- **WHEN** `multi-subsystem-fdir-v1` broadens the shared recovery path to ADCS and COMM
- **THEN** `EpsBridge` SHALL continue to own EPS poll-health truth
- **AND** `EpsFdirController` SHALL continue to own EPS timeout retry, latch, and first-success clear truth
- **AND** `RecoveryExecutor` SHALL remain the only owner allowed to execute the bounded EPS reset, `SAFE`, or reboot-intent actions

#### Scenario: EPS timeout thresholds remain unchanged
- **WHEN** reviewers inspect the active EPS timeout detector after this change
- **THEN** the detector SHALL still treat failures `1-2` as retry-only
- **AND** it SHALL still latch EPS timeout fault at consecutive failure count `3`
- **AND** it SHALL still clear the latched EPS timeout on the first successful EPS poll

### Requirement: Scheduled EPS Live Visibility Is Summary-Oriented

The EPS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit EPS status or
control commands.

#### Scenario: Scheduled EPS refresh keeps only summary telemetry live
- **WHEN** `EpsBridge` performs a scheduled status refresh
- **THEN** it SHALL keep only the selected EPS summary telemetry as baseline
  live visibility
- **AND** it SHALL keep low-battery, critical-battery, over-temperature,
  PDU-change, and comm-error events reviewable on that same path.

#### Scenario: Explicit EPS readback remains fresh and detailed
- **WHEN** `EPS_GET_STATUS`, `EPS_SET_PDU`, `EPS_SET_HEATER`, or `EPS_RESET`
  obtains a fresh EPS reply
- **THEN** the subsystem SHALL update the owned EPS cache and threshold/latch
  state through the normal apply path
- **AND** it SHALL make the detailed EPS telemetry reviewable as bounded
  readback for that explicit interaction.

### Requirement: EPS Simulator Supports External Runtime SoC Control

The hosted and subsystem EPS simulator SHALL support an optional external
runtime SoC control surface that lets repository-owned probes change simulator
state-of-charge while the simulator remains online.

#### Scenario: External immediate SoC set updates subsequent status
- **WHEN** a repo-owned helper sends a valid external SoC control request with
  `transition-sec = 0` or omitted
- **THEN** the simulator SHALL keep serving on the same EPS CSP node
- **AND** subsequent EPS status responses SHALL reflect the requested SoC
- **AND** later load-model evolution SHALL continue from that updated SoC state

#### Scenario: Timed SoC ramp resolves lazily from monotonic time
- **WHEN** a repo-owned helper sends a valid external SoC control request with
  a positive `transition-sec`
- **THEN** the simulator SHALL preserve the current SoC, target SoC, and
  transition window internally
- **AND** later status responses SHALL report the SoC implied by monotonic time
  across that window without requiring a separate always-running ramp thread
- **AND** the post-ramp SoC SHALL continue evolving under the active load model

#### Scenario: Invalid control request does not perturb simulator service
- **WHEN** the external control surface receives an invalid SoC control request
- **THEN** the simulator SHALL reject that request
- **AND** it SHALL keep the prior valid SoC trajectory unchanged
- **AND** it SHALL continue serving EPS CSP requests

#### Scenario: Control surface remains separate from OBC command ownership
- **WHEN** reviewers inspect the active runtime SoC stimulation path
- **THEN** the SoC control surface SHALL be simulator-owned and externally
  injected
- **AND** the OBC SHALL NOT gain a new public EPS command for directly setting
  simulator SoC

### Requirement: EPS Runtime Load Modes Stay Simulator-Owned

The hosted EPS simulator SHALL provide simulator-owned runtime load modes
`normal` and `high-draw` that alter only the internally generated power curve
and SHALL NOT create a new OBC public command family.

#### Scenario: External load-mode change updates subsequent power curves
- **WHEN** a repo-owned helper sends a valid runtime load-mode control request
  for `normal` or `high-draw`
- **THEN** the simulator SHALL keep serving on the same EPS CSP node
- **AND** subsequent status responses SHALL reflect the selected mode's load
  overlay and derived current, voltage, power, temperature, and SoC behavior

#### Scenario: Load mode remains sticky across later PDU activity
- **WHEN** the simulator is already in `high-draw` mode and a later PDU or
  heater command is applied
- **THEN** the selected load mode SHALL remain active until another valid
  load-mode control request changes it
- **AND** the simulator SHALL combine the selected mode with the current PDU
  and heater state when deriving load current

### Requirement: EPS PDU Mapping Drives Weighted Demo Loads

The hosted EPS simulator SHALL treat PDU channels `0`, `1`, `3`, `5`, and `6`
as named weighted loads for `OBC`, `ADCS`, `Payload`, `S-band`, and `UHF`,
while channels `2`, `4`, and `7` remain zero-load spare channels.

#### Scenario: Weighted mapped channel changes derived load
- **WHEN** a mapped PDU channel transitions between disabled and enabled
- **THEN** the next status response SHALL change the derived load current
  according to that channel's configured weight
- **AND** it SHALL keep the owned `pdu_status` bitmask contract unchanged

#### Scenario: Unmapped spare channel stays zero-load
- **WHEN** an unmapped spare channel `2`, `4`, or `7` transitions between
  disabled and enabled
- **THEN** the simulator SHALL update the owned `pdu_status` bitmask
- **AND** it SHALL NOT add any new load contribution for that channel

### Requirement: EPS Summary Fields Follow Seeded Time-Continuous Dynamics

The hosted EPS simulator SHALL evolve its summary power fields from monotonic
time using seeded deterministic pseudo-noise and derived load integration
rather than returning fixed values for each request.

#### Scenario: Normal mode produces bounded non-flat telemetry
- **WHEN** the simulator remains in `normal` mode over successive
  time-separated status reads
- **THEN** the reported `vbat`, `ibat`, `isolar`, `vsolar`, and `temp_bat`
  values SHALL vary within bounded configured amplitudes
- **AND** the same seed and time progression SHALL reproduce the same value
  sequence

#### Scenario: High-draw mode creates a visible discharge step
- **WHEN** the simulator transitions from `normal` to `high-draw`
- **THEN** the next status responses SHALL show a more negative battery current
  and a lower battery voltage than the immediately preceding `normal` state
- **AND** the simulator SHALL discharge state-of-charge faster in that mode
  until the selected load mode changes or the available charge is exhausted
