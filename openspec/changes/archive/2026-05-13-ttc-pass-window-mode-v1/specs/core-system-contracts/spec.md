## ADDED Requirements

### Requirement: TTC Pass Policy Runtime Surface
The core system contracts capability SHALL provide a bounded runtime TTC pass-policy surface instead of leaving `TTC` as a pure manual shell.

#### Scenario: TTC config is bounded
- **WHEN** TTC pass policy is configured at runtime
- **THEN** the active contract SHALL expose `enabled` and `loss_of_lock_timeout_sec`
- **AND** it SHALL NOT require generic scheduler payloads, queued activities, TLE uploads, or payload-operation plans

#### Scenario: Pass-window contract uses one epoch interval
- **WHEN** TTC pass policy accepts pass-window truth at runtime
- **THEN** it SHALL accept one active window using `start_unix_sec` and `end_unix_sec`
- **AND** it SHALL provide an explicit clear-window action
- **AND** it SHALL NOT claim a generic multi-window scheduler or onboard pass-prediction engine

#### Scenario: Invalid pass windows are rejected fail-closed
- **WHEN** a TTC pass window is configured with `start_unix_sec == 0`, `end_unix_sec == 0`, or `end_unix_sec <= start_unix_sec`
- **THEN** the runtime SHALL reject that window input
- **AND** the runtime SHALL NOT auto-enter `TTC` from that invalid input

### Requirement: TTC Pass Policy Entry And Exit
The core system contracts capability SHALL define deterministic TTC pass-policy entry and exit behavior for the active baseline.

#### Scenario: TTC auto-entry requires bounded guards
- **WHEN** current mode is `IDLE`
- **AND** TTC config `enabled == true`
- **AND** a pass window is configured
- **AND** cached GPS time basis is valid
- **AND** current GPS-derived Unix epoch time is inside the configured window using `start <= now < end`
- **THEN** the TTC pass-policy owner SHALL request `TTC`

#### Scenario: TTC auto-entry does not bypass non-IDLE modes
- **WHEN** the pass window is active and GPS time basis is valid
- **AND** current mode is not `IDLE`
- **THEN** the TTC pass-policy owner SHALL NOT force a cross-mode takeover in this change

#### Scenario: TTC auto-exit occurs when window ends
- **WHEN** current mode is `TTC`
- **AND** the configured pass window is no longer active
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs when TTC is disabled
- **WHEN** current mode is `TTC`
- **AND** TTC config `enabled == false`
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs when GPS validity is lost
- **WHEN** current mode is `TTC`
- **AND** the cached GPS time basis becomes invalid
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs on bounded COMM loss timeout
- **WHEN** current mode is `TTC`
- **AND** both `sbandAvailable == false` and `uhfAvailable == false`
- **AND** that condition persists for longer than `loss_of_lock_timeout_sec`
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

### Requirement: GPS Time Basis For TTC Policy
The core system contracts capability SHALL use bounded cached GPS truth for TTC pass-window evaluation.

#### Scenario: GPS time basis requires cached fix truth
- **WHEN** TTC pass policy evaluates GPS validity
- **THEN** it SHALL require `hasSample == true`
- **AND** it SHALL require `fixValid == true`
- **AND** it SHALL require nonzero UTC date and second-of-day fields

#### Scenario: GPS time basis requires freshness
- **WHEN** TTC pass policy evaluates cached GPS validity
- **THEN** it SHALL require `acceptedSentenceCount` to have advanced within a fixed bounded freshness threshold
- **AND** it SHALL fail closed if freshness cannot be proven

#### Scenario: TTC policy converts cached UTC to epoch internally
- **WHEN** TTC pass policy compares the current time to the configured epoch window
- **THEN** it SHALL convert cached GPS `utcDateYmd` and `utcSecondsOfDay` to Unix epoch time inside the TTC pass-policy logic
- **AND** it SHALL fail closed on impossible UTC fields or conversion failure
- **AND** it SHALL NOT treat component wall-clock time as the authoritative source for pass-window comparison

### Requirement: TTC Policy Coexists With Manual TTC Path
The core system contracts capability SHALL keep manual TTC entry available while making TTC retention policy-owned.

#### Scenario: Manual entry remains available
- **WHEN** an operator requests `IDLE -> TTC` through the existing guarded operator mode path
- **THEN** the operator path SHALL remain available in this change

#### Scenario: Active TTC remains governed by policy
- **WHEN** current mode is `TTC`
- **THEN** the TTC pass-policy owner SHALL evaluate the same exit guards whether `TTC` was entered manually or automatically

#### Scenario: Invalid manual TTC does not persist
- **WHEN** an operator manually enters `TTC`
- **AND** TTC policy conditions are not satisfied on the next policy cycle
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: Manual exit remains available
- **WHEN** an operator requests `TTC -> IDLE` or `TTC -> SAFE`
- **THEN** the manual request SHALL remain available through the existing guarded operator path

## MODIFIED Requirements

### Requirement: Mode Shell Boundary
The first mode-model-v2 implementation SHALL make `HELL`, `PAYLOAD`, and `TTC` primary modes without adding unrelated deferred behaviors, and the ttc-pass-window-mode-v1 implementation SHALL upgrade `TTC` from a pure manual shell to a bounded pass-window-driven mode policy.

#### Scenario: Mode shells do not imply unrelated deferred subsystems
- **WHEN** an operator command or internal safety path changes the current mode to `HELL`, `PAYLOAD`, or `TTC`
- **THEN** the system SHALL update the public mode state
- **AND** it SHALL NOT claim generic scheduler behavior, payload execution, ADCS tracking, COMM architecture redesign, storage-policy behavior, or broader FDIR from that transition alone

#### Scenario: PAYLOAD shell remains free of payload mission side effects
- **WHEN** an operator enters `PAYLOAD`
- **THEN** the runtime SHALL update only the approved system mode state, event, and telemetry contract
- **AND** it SHALL NOT start camera, recorder, payload power sequencing, payload data products, or payload mission execution

#### Scenario: TTC mode is policy-driven but remains bounded
- **WHEN** TTC pass policy enters or retains `TTC`
- **THEN** the runtime SHALL update the approved system mode state, event, telemetry, and TTC policy status surfaces
- **AND** it SHALL NOT claim generic scheduling, TLE parsing, orbital propagation, ADCS ground tracking, link authority, auth session, or CCSDS routing changes from this change alone
