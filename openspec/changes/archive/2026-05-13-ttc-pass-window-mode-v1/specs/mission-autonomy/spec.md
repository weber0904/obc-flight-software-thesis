## ADDED Requirements

### Requirement: TtcPassManager Owns TTC Pass Policy
The mission-autonomy capability SHALL provide a focused `TtcPassManager` owner for TTC pass-window automation, and the active runtime SHALL keep that owner separate from `ModeManager`, `ModeSafetyController`, `CommController`, and `RecoveryExecutor`.

#### Scenario: Active topology instantiates a focused TTC policy owner
- **WHEN** the ttc-pass-window-mode-v1 runtime is built
- **THEN** the active topology SHALL instantiate, configure, and schedule `TtcPassManager`
- **AND** it SHALL NOT move TTC pass-window ownership into `ModeManager`, `ModeSafetyController`, `CommController`, or the retired `MissionExecutive`

#### Scenario: TTC policy owner consumes bounded provider truth only
- **WHEN** `TtcPassManager` evaluates TTC policy
- **THEN** it SHALL use current mode, cached GPS state, COMM availability, configured TTC policy state, and component time for bounded freshness/timeout age
- **AND** it SHALL NOT consume scenario truth directly
- **AND** it SHALL NOT command payload execution, generic scheduler actions, ADCS pointing, COMM session redesign, or broader FDIR behavior

### Requirement: TTC Pass Policy Uses Normal Mode Infrastructure
The mission-autonomy capability SHALL keep TTC pass policy on the normal runtime mode path instead of introducing a parallel TTC mode store.

#### Scenario: TTC policy requests mode changes through the normal internal path
- **WHEN** `TtcPassManager` decides that TTC should enter or exit
- **THEN** it SHALL request mode changes through the existing internal mode-control apply path
- **AND** it SHALL use a distinct TTC policy source tag

#### Scenario: Safety and recovery remain authoritative
- **WHEN** `ModeSafetyController` or `RecoveryExecutor` has already moved the runtime out of `TTC`
- **THEN** `TtcPassManager` SHALL observe that current mode change and clear its internal TTC policy bookkeeping
- **AND** it SHALL NOT restore `TTC` unless the normal TTC entry guards are later satisfied from `IDLE`

### Requirement: TTC Pass Policy Boundaries Stay Narrow
The mission-autonomy capability SHALL keep TTC pass policy bounded to one pass window and explicit exit/entry guards.

#### Scenario: TTC policy is not a generic scheduler
- **WHEN** the ttc-pass-window-mode-v1 change is implemented
- **THEN** `TtcPassManager` SHALL manage at most one configured pass window at a time
- **AND** it SHALL NOT claim queued activities, command payload execution, generalized time-tagged scheduling, or broad mission planning

#### Scenario: TTC policy defers ADCS pointing
- **WHEN** the ttc-pass-window-mode-v1 change is implemented
- **THEN** `TtcPassManager` SHALL NOT claim ADCS ground-tracking ownership or emit ADCS pointing requests in this change

## MODIFIED Requirements

### Requirement: Mode Safety Controller Owns V1 SoC Fallback
The mission-autonomy capability SHALL provide a narrow active `ModeSafetyController` component that owns the v1 cached-EPS SoC safety fallback policy for the `SAFE`, `HELL`, `IDLE`, `PAYLOAD`, and `TTC` mode model, and TTC pass-window automation SHALL remain outside that SoC-only owner.

#### Scenario: Cached EPS status remains the only safety policy input
- **WHEN** `ModeSafetyController` evaluates safety fallback or cached-EPS operator safety guard behavior
- **THEN** it SHALL use cached EPS state of charge from the runtime EPS status provider and the current mode from the runtime mode-control provider
- **AND** it SHALL NOT consume scenario truth directly, command ADCS, command EPS load shedding, command COMM behavior, schedule payload activity, schedule TTC pass windows, or perform watchdog, subsystem timeout, retry, reset, or broader FDIR actions

#### Scenario: Low-battery TTC handling stays on existing safety path
- **WHEN** current mode is `TTC`
- **AND** cached EPS SoC falls below the existing fallback threshold
- **THEN** `ModeSafetyController` SHALL continue to request the existing safety fallback through the normal internal mode-control runtime path
- **AND** TTC pass-window policy SHALL NOT override that safety action
