## MODIFIED Requirements

### Requirement: Hosted ADCS Simulator
The hosted ADCS implementation SHALL provide a simulator executable that serves state, mode, target, calibration, and recovery-reset services as libcsp node `3` over the hosted ZMQHUB-backed internal CSP substrate while maintaining seeded deterministic pseudo-noise and time-continuous quaternion, angular-rate, pointing-error, and sensor-validity state.

#### Scenario: Mode command changes simulator behavior
- **WHEN** a client issues `ADCS_SET_MODE`
- **THEN** the simulator SHALL update the commanded mode
- **AND** it SHALL reflect that new mode's time-continuous dynamics in subsequent status responses

#### Scenario: Target command changes simulator target state
- **WHEN** a client issues `ADCS_SET_TARGET`
- **THEN** the simulator SHALL update the owned target quaternion used for pointing-error evaluation
- **AND** it SHALL keep the synthetic pointing-pass profile as the current attitude-motion owner for the hosted demo path

#### Scenario: Calibration command is served over internal CSP
- **WHEN** a client issues an ADCS calibration request over the hosted internal CSP substrate
- **THEN** the simulator SHALL acknowledge the request
- **AND** it SHALL preserve deterministic bounded state reporting after the calibration service returns

#### Scenario: Reset command restores the deterministic default ADCS state
- **WHEN** a client issues an ADCS reset request over the hosted internal CSP substrate
- **THEN** the simulator SHALL restore the same default mode, quaternion, target, angular-rate, and derived state that `loadDefaults_()` provides on startup
- **AND** the restored state SHALL include its sensor-validity state and derived pointing and magnetometer fields
- **AND** it SHALL return that restored state through the normal ADCS state reply shape

### Requirement: Scenario-Seeded ADCS Deployment Rates
The hosted ADCS simulator SHALL accept scenario-seeded deployment-rate angular velocity inputs during scenario initialization, and the simulator SHALL preserve its own time-continuous control-responsive dynamics after that initialization instead of being continuously overwritten by replay.

#### Scenario: Detumble remains control-responsive after scenario seeding
- **WHEN** the scenario bridge seeds the hosted ADCS simulator with deployment-rate angular velocity and the OBC later commands `DETUMBLE`
- **THEN** the hosted ADCS simulator SHALL converge according to its own time-continuous control-response model rather than being forced back to the seeded angular rate on each replay step

## ADDED Requirements

### Requirement: ADCS Mode Dynamics Stay Time-Continuous
The hosted ADCS simulator SHALL evolve `IDLE`, `DETUMBLE`, and `POINTING` behavior from monotonic time rather than by applying one fixed state jump per request.

#### Scenario: Idle mode remains bounded but non-static
- **WHEN** the simulator remains in `IDLE` mode over successive time-separated state reads
- **THEN** quaternion and angular-rate values SHALL remain bounded near the idle baseline
- **AND** they SHALL exhibit small seeded deterministic variation instead of staying exactly fixed

#### Scenario: Detumble mode decays toward the mission threshold
- **WHEN** the simulator remains in `DETUMBLE` mode over successive time-separated state reads
- **THEN** the angular-rate norm SHALL decay toward the configured mission threshold
- **AND** the simulator SHALL preserve bounded quaternion normalization while that decay occurs

### Requirement: Pointing Mode Uses A Synthetic Pass Profile
The hosted ADCS simulator SHALL model `POINTING` as a repeating synthetic pointing pass that sweeps a bounded roll, pitch, and yaw trajectory over a fixed `60 s` cycle.

#### Scenario: Entering pointing starts the synthetic pass
- **WHEN** the simulator receives `ADCS_SET_MODE(POINTING)` from any non-pointing mode
- **THEN** it SHALL reset the pointing-pass phase to the start of the fixed `60 s` synthetic pass
- **AND** subsequent state replies SHALL track the synthetic pass attitude with bounded angular-rate output

#### Scenario: Ongoing pointing does not reset on redundant mode command
- **WHEN** the simulator is already in `POINTING` mode and receives another pointing-mode command without leaving that mode first
- **THEN** it SHALL keep the current synthetic pass phase
- **AND** it SHALL continue the existing pass instead of rewinding automatically

### Requirement: ADCS Pointing Pass Can Be Restarted Externally
The hosted ADCS simulator SHALL expose a simulator-owned external control that restarts the synthetic pointing-pass phase without creating a new OBC command.

#### Scenario: Restart control rewinds the pointing pass
- **WHEN** a repo-owned helper sends a valid `restart-pointing-pass` control request while the simulator remains online
- **THEN** the simulator SHALL keep serving on the same ADCS CSP node
- **AND** it SHALL reset the synthetic pointing-pass phase to the start of the fixed `60 s` cycle for subsequent state replies

#### Scenario: Control surface remains separate from OBC command ownership
- **WHEN** reviewers inspect the active hosted pointing-pass replay path
- **THEN** the restart control SHALL remain simulator-owned and externally injected
- **AND** the OBC SHALL NOT gain a new public ADCS command for replaying the pointing pass
