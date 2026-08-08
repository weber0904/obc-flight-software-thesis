## MODIFIED Requirements

### Requirement: Hosted EPS Simulator
The hosted EPS implementation SHALL provide a simulator executable that serves the EPS status, PDU, config, and reset services as libcsp node `2` over the hosted ZMQHUB-backed internal CSP substrate while maintaining seeded deterministic pseudo-noise, time-driven battery and solar evolution, heater state, and 8-channel PDU state.

#### Scenario: PDU command updates simulator state
- **WHEN** a client issues `EPS_SET_PDU`
- **THEN** the simulator SHALL update the requested PDU channel
- **AND** it SHALL reflect the new aggregate PDU state in the next status response
- **AND** mapped channels `0`, `1`, `3`, `5`, and `6` SHALL contribute their configured load weights while unmapped channels `2`, `4`, and `7` SHALL remain zero-load spare channels

### Requirement: Scenario-Driven EPS Replay Inputs
The hosted EPS simulator SHALL accept scenario-driven sunlight and battery state-of-charge inputs from the repository-owned scenario bridge while preserving the existing PDU, heater, status-command, and runtime load-mode behavior.

#### Scenario: Scenario replay updates EPS environment state
- **WHEN** the scenario bridge applies a replay sample with new sunlight or battery state-of-charge values
- **THEN** the hosted EPS simulator SHALL reflect those values in subsequent status responses
- **AND** it SHALL preserve the current commandable PDU and heater behavior
- **AND** it SHALL continue evolving the remaining load-derived fields from its time-driven model

### Requirement: EPS Simulator Supports External Runtime SoC Control
The hosted and subsystem EPS simulator SHALL support an optional external runtime control surface that lets repository-owned probes change simulator state-of-charge while the simulator remains online.

#### Scenario: External immediate SoC set updates subsequent status
- **WHEN** a repo-owned helper sends a valid external SoC control request with `transition-sec = 0` or omitted
- **THEN** the simulator SHALL keep serving on the same EPS CSP node
- **AND** subsequent EPS status responses SHALL reflect the requested SoC before later load-model evolution continues

#### Scenario: Timed SoC ramp resolves lazily from monotonic time
- **WHEN** a repo-owned helper sends a valid external SoC control request with a positive `transition-sec`
- **THEN** the simulator SHALL preserve the current SoC, target SoC, and transition window internally
- **AND** later status responses SHALL report the SoC implied by monotonic time across that window without requiring a separate always-running ramp thread
- **AND** the post-ramp SoC SHALL continue evolving under the active load model

#### Scenario: Invalid control request does not perturb simulator service
- **WHEN** the external control surface receives an invalid SoC control request
- **THEN** the simulator SHALL reject that request
- **AND** it SHALL keep the prior valid SoC trajectory unchanged
- **AND** it SHALL continue serving EPS CSP requests

#### Scenario: Control surface remains separate from OBC command ownership
- **WHEN** reviewers inspect the active runtime SoC stimulation path
- **THEN** the SoC control surface SHALL be simulator-owned and externally injected
- **AND** the OBC SHALL NOT gain a new public EPS command for directly setting simulator SoC

## ADDED Requirements

### Requirement: EPS Runtime Load Modes Stay Simulator-Owned
The hosted EPS simulator SHALL provide simulator-owned runtime load modes `normal` and `high-draw` that alter only the internally generated power curve and SHALL NOT create a new OBC public command family.

#### Scenario: External load-mode change updates subsequent power curves
- **WHEN** a repo-owned helper sends a valid runtime load-mode control request for `normal` or `high-draw`
- **THEN** the simulator SHALL keep serving on the same EPS CSP node
- **AND** subsequent status responses SHALL reflect the selected mode's load overlay and derived current, voltage, power, temperature, and SoC behavior

#### Scenario: Load mode remains sticky across later PDU activity
- **WHEN** the simulator is already in `high-draw` mode and a later PDU or heater command is applied
- **THEN** the selected load mode SHALL remain active until another valid load-mode control request changes it
- **AND** the simulator SHALL combine the selected mode with the current PDU and heater state when deriving load current

### Requirement: EPS PDU Mapping Drives Weighted Demo Loads
The hosted EPS simulator SHALL treat PDU channels `0`, `1`, `3`, `5`, and `6` as named weighted loads for `OBC`, `ADCS`, `Payload`, `S-band`, and `UHF`, while channels `2`, `4`, and `7` remain zero-load spare channels.

#### Scenario: Weighted mapped channel changes derived load
- **WHEN** a mapped PDU channel transitions between disabled and enabled
- **THEN** the next status response SHALL change the derived load current according to that channel's configured weight
- **AND** it SHALL keep the owned `pdu_status` bitmask contract unchanged

#### Scenario: Unmapped spare channel stays zero-load
- **WHEN** an unmapped spare channel `2`, `4`, or `7` transitions between disabled and enabled
- **THEN** the simulator SHALL update the owned `pdu_status` bitmask
- **AND** it SHALL NOT add any new load contribution for that channel

### Requirement: EPS Summary Fields Follow Seeded Time-Continuous Dynamics
The hosted EPS simulator SHALL evolve its summary power fields from monotonic time using seeded deterministic pseudo-noise and derived load integration rather than returning fixed values for each request.

#### Scenario: Normal mode produces bounded non-flat telemetry
- **WHEN** the simulator remains in `normal` mode over successive time-separated status reads
- **THEN** the reported `vbat`, `ibat`, `isolar`, `vsolar`, and `temp_bat` values SHALL vary within bounded configured amplitudes
- **AND** the same seed and time progression SHALL reproduce the same value sequence

#### Scenario: High-draw mode creates a visible discharge step
- **WHEN** the simulator transitions from `normal` to `high-draw`
- **THEN** the next status responses SHALL show a more negative battery current and a lower battery voltage than the immediately preceding `normal` state
- **AND** the simulator SHALL discharge state-of-charge faster in that mode until the selected load mode changes or the available charge is exhausted
