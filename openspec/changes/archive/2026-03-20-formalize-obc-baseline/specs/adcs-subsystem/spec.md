## ADDED Requirements

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
`AdcsBridge` SHALL poll on a schedule, SHALL retain the last valid state when replies are invalid or absent, and SHALL raise the appropriate comms or sensor-fault path instead of emitting random replacement values.

#### Scenario: Invalid ADCS reply
- **WHEN** `AdcsBridge` receives an invalid or incomplete ADCS reply
- **THEN** it SHALL preserve the last valid state and signal the degraded condition through the owned fault path
