## ADDED Requirements

### Requirement: SoC-Guarded Operator Admissions
The core system contracts capability SHALL keep the existing guarded operator transition matrix while letting specific accepted transitions require cached-EPS SoC approval from the transition guard.

#### Scenario: Operator transition matrix still defines topology
- **WHEN** an operator requests a transition through `MODE_SET` or hosted `mode <target>`
- **THEN** the runtime SHALL evaluate the request against this v1 matrix:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | guarded allow | reject | reject | reject |
| IDLE | allow | no-op | guarded allow | allow | reject |
| PAYLOAD | allow | allow | no-op | reject | reject |
| TTC | allow | allow | reject | no-op | reject |
| HELL | guarded allow | reject | reject | reject | no-op |

#### Scenario: SAFE to IDLE denial maps to validation error
- **WHEN** an operator requests `SAFE -> IDLE`
- **AND** the transition guard rejects the request because cached EPS SoC is unavailable or not strictly greater than `50%`
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: IDLE to PAYLOAD denial maps to validation error
- **WHEN** an operator requests `IDLE -> PAYLOAD`
- **AND** the transition guard rejects the request because cached EPS SoC is unavailable or not strictly greater than `70%`
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: IDLE to TTC remains free of new SoC gate
- **WHEN** an operator requests `IDLE -> TTC`
- **THEN** the transition guard SHALL evaluate only the existing topology/manual shell rule in this change
- **AND** it SHALL NOT deny that request due to a new SoC admission threshold introduced by this change

## MODIFIED Requirements

### Requirement: Guarded Operator Mode Transitions
The core system contracts capability SHALL route every operator-requested primary mode transition through the v1 operator transition guard instead of treating `MODE_SET` or the hosted mode shell as arbitrary mode setters.

#### Scenario: Operator transition matrix is enforced
- **WHEN** an operator requests a transition through `MODE_SET` or hosted `mode <target>`
- **THEN** the runtime SHALL evaluate the request against this v1 matrix:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | guarded allow | reject | reject | reject |
| IDLE | allow | no-op | guarded allow | allow | reject |
| PAYLOAD | allow | allow | no-op | reject | reject |
| TTC | allow | allow | reject | no-op | reject |
| HELL | guarded allow | reject | reject | reject | no-op |

#### Scenario: MODE_SET uses the guarded operator path
- **WHEN** an operator sends `ModeManager.MODE_SET` with a valid `SatMode` argument
- **THEN** `ModeManager` SHALL use the guarded operator request path before changing the runtime mode
- **AND** `ModeManager.MODE_SET` SHALL NOT call the internal safety/test apply path directly

#### Scenario: Hosted shell uses the guarded operator path
- **WHEN** an operator sends `mode safe`, `mode idle`, `mode payload`, `mode ttc`, or `mode hell` through the hosted runtime shell
- **THEN** the hosted runtime SHALL use the same guarded operator request path as `MODE_SET`
- **AND** the hosted runtime shell SHALL NOT call the internal safety/test apply path directly

#### Scenario: Invalid F Prime enum payload is rejected before the guard
- **WHEN** a `MODE_SET` command payload contains an out-of-range `SatMode` serialized value
- **THEN** generated F Prime deserialization SHALL reject the payload with `FORMAT_ERROR`
- **AND** `ModeManager` SHALL NOT invoke the transition guard or emit transition events for that malformed command

### Requirement: Operator Mode Transition Outcomes
The core system contracts capability SHALL define deterministic response, event, and telemetry semantics for accepted, no-op, rejected, and parser-rejected operator mode requests.

#### Scenario: Accepted mode-changing transition publishes the existing mode-change surface
- **WHEN** an operator transition is accepted and the target mode differs from the current mode
- **THEN** the runtime SHALL update the current mode to the target mode
- **AND** it SHALL emit exactly one existing `SYS_MODE_CHANGE(mode)` event using the target mode
- **AND** it SHALL eventually publish `SYS_MODE` telemetry reflecting the target mode
- **AND** it SHALL return `Fw::CmdResponse::OK` on the F Prime command path

#### Scenario: Same-mode request is an OK no-op
- **WHEN** an operator requests the current mode
- **THEN** the runtime SHALL leave the current mode unchanged
- **AND** it SHALL emit zero `SYS_MODE_CHANGE` events
- **AND** it SHALL emit zero `SYS_MODE_TRANSITION_REJECTED` events
- **AND** it SHALL return `Fw::CmdResponse::OK` on the F Prime command path

#### Scenario: Rejected transition preserves current mode
- **WHEN** an operator transition is rejected by the v1 transition guard
- **THEN** the runtime SHALL leave the current mode unchanged
- **AND** it SHALL emit zero `SYS_MODE_CHANGE` events
- **AND** it SHALL emit exactly one `SYS_MODE_TRANSITION_REJECTED(fromMode, toMode, reasonCode)` event
- **AND** it SHALL NOT publish `SYS_MODE` telemetry with the rejected target mode

#### Scenario: Guard-denied transition maps to validation error
- **WHEN** an operator transition is rejected because the requested transition is not allowed or because a configured SoC guard denies the request
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: Missing guard wiring maps to execution error
- **WHEN** an operator transition request reaches `ModeManager` without a configured transition guard
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::EXECUTION_ERROR`
- **AND** it SHALL emit `SYS_MODE_TRANSITION_REJECTED` with reason code `GUARD_UNCONFIGURED`

### Requirement: Mode Transition Rejection Reasons
The core system contracts capability SHALL publish stable `U32` reason codes for `SYS_MODE_TRANSITION_REJECTED`.

#### Scenario: Reason codes are stable
- **WHEN** a mode transition is rejected by the v1 operator transition guard
- **THEN** `SYS_MODE_TRANSITION_REJECTED.reasonCode` SHALL use these stable values:

| Code | Name |
|---:|---|
| 1 | `DISALLOWED_OPERATOR_TRANSITION` |
| 2 | `INTERNAL_ONLY_TARGET` |
| 3 | `SOC_GUARD_UNAVAILABLE` |
| 4 | `SOC_GUARD_NOT_MET` |
| 5 | `GUARD_UNCONFIGURED` |

#### Scenario: Internal-only HELL target is distinguishable
- **WHEN** an operator requests `SAFE -> HELL`, `IDLE -> HELL`, `PAYLOAD -> HELL`, or `TTC -> HELL`
- **THEN** the request SHALL be rejected with reason code `INTERNAL_ONLY_TARGET`

#### Scenario: Disallowed topology transitions are distinguishable
- **WHEN** an operator requests `SAFE -> PAYLOAD`, `SAFE -> TTC`, `PAYLOAD -> TTC`, `TTC -> PAYLOAD`, `HELL -> IDLE`, `HELL -> PAYLOAD`, or `HELL -> TTC`
- **THEN** the request SHALL be rejected with reason code `DISALLOWED_OPERATOR_TRANSITION`

#### Scenario: Shared SoC guard codes cover recovery and admission
- **WHEN** an operator `HELL -> SAFE`, `SAFE -> IDLE`, or `IDLE -> PAYLOAD` request is rejected because cached EPS status is unavailable
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_UNAVAILABLE`

#### Scenario: Shared SoC guard threshold failures are distinguishable
- **WHEN** an operator `HELL -> SAFE`, `SAFE -> IDLE`, or `IDLE -> PAYLOAD` request is rejected because cached EPS SoC does not meet the required strict threshold
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`
