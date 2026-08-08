## ADDED Requirements

### Requirement: Guarded Operator Mode Transitions
The core system contracts capability SHALL route every operator-requested primary mode transition through the v1 operator transition guard instead of treating `MODE_SET` or the hosted mode shell as arbitrary mode setters.

#### Scenario: Operator transition matrix is enforced
- **WHEN** an operator requests a transition through `MODE_SET` or hosted `mode <target>`
- **THEN** the runtime SHALL evaluate the request against this v1 matrix:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | allow | reject | reject | reject |
| IDLE | allow | no-op | allow | allow | reject |
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
- **WHEN** an operator transition is rejected because the requested transition is not allowed or because a configured SoC recovery guard denies the request
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
| 3 | `SOC_RECOVERY_GUARD_UNAVAILABLE` |
| 4 | `SOC_RECOVERY_GUARD_NOT_MET` |
| 5 | `GUARD_UNCONFIGURED` |

#### Scenario: Internal-only HELL target is distinguishable
- **WHEN** an operator requests `SAFE -> HELL`, `IDLE -> HELL`, `PAYLOAD -> HELL`, or `TTC -> HELL`
- **THEN** the request SHALL be rejected with reason code `INTERNAL_ONLY_TARGET`

#### Scenario: Disallowed topology transitions are distinguishable
- **WHEN** an operator requests `SAFE -> PAYLOAD`, `SAFE -> TTC`, `PAYLOAD -> TTC`, `TTC -> PAYLOAD`, `HELL -> IDLE`, `HELL -> PAYLOAD`, or `HELL -> TTC`
- **THEN** the request SHALL be rejected with reason code `DISALLOWED_OPERATOR_TRANSITION`

### Requirement: Hosted Mode Shell Parser Boundary
The core system contracts capability SHALL keep hosted mode shell parsing separate from transition-guard rejection.

#### Scenario: Canonical hosted mode spellings parse
- **WHEN** an operator enters `mode safe`, `mode idle`, `mode payload`, `mode ttc`, or `mode hell`
- **THEN** the hosted parser SHALL parse the target as the corresponding `SatMode`
- **AND** the transition guard SHALL determine whether the parsed request is accepted or rejected

#### Scenario: Retired mode spellings stay rejected by the parser
- **WHEN** an operator enters `mode nominal`, `mode low-power`, `mode debug`, or `mode update`
- **THEN** the hosted parser SHALL reject the input as an unknown mode
- **AND** the runtime SHALL emit zero transition events for that parser error

#### Scenario: Unknown or mixed-case mode spellings stay parser errors
- **WHEN** an operator enters an unknown spelling or a mixed-case spelling such as `mode PAYLOAD`
- **THEN** the hosted parser SHALL reject the input as an unknown mode
- **AND** the runtime SHALL emit zero transition events for that parser error

## MODIFIED Requirements

### Requirement: Mode Shell Boundary
The first mode-model-v2 implementation SHALL make `HELL`, `PAYLOAD`, and `TTC` primary modes without adding the later behaviors associated with those modes, and the payload-ttc-mode-entry-v1 implementation SHALL restrict operator entry to those modes through the v1 guarded operator transition matrix.

#### Scenario: Mode shells do not imply deferred subsystems
- **WHEN** an operator command or internal safety path changes the current mode to `HELL`, `PAYLOAD`, or `TTC`
- **THEN** the system SHALL update the public mode state
- **AND** it SHALL NOT claim load shedding, payload execution, pass scheduling, COMM split-link behavior, CCSDS behavior, storage-policy behavior, or FDIR behavior from that mode transition alone

#### Scenario: PAYLOAD shell has no payload side effects in v1
- **WHEN** an operator enters `PAYLOAD` through the v1 guarded transition matrix
- **THEN** the runtime SHALL update only the system mode state, event, and telemetry contract
- **AND** it SHALL NOT start camera, recorder, payload power sequencing, payload data products, or payload mission execution

#### Scenario: TTC shell has no TTC automation side effects in v1
- **WHEN** an operator enters `TTC` through the v1 guarded transition matrix
- **THEN** the runtime SHALL update only the system mode state, event, and telemetry contract
- **AND** it SHALL NOT start pass scheduling, TLE/GPS pass-window evaluation, ground tracking, link authority, auth session, or CCSDS routing changes
