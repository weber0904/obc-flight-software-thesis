## ADDED Requirements

### Requirement: First-Version Mission Executive
The project SHALL provide a first-version `MissionExecutive` component that observes cached subsystem state and issues system-level autonomy actions without moving scenario ingestion into the OBC runtime.

#### Scenario: Mission executive reacts to subsystem state
- **WHEN** cached subsystem state indicates an autonomy policy condition has been met
- **THEN** the `MissionExecutive` SHALL drive the corresponding system-level action through the owned runtime control boundary

### Requirement: Low-Battery Entry To LOW_POWER
The first autonomy slice SHALL enter `SatMode::LOW_POWER` when cached EPS state-of-charge drops below the existing low-battery threshold.

#### Scenario: Low battery triggers low-power mode
- **WHEN** cached EPS state-of-charge falls below the low-battery threshold
- **THEN** the `MissionExecutive` SHALL command `ModeManager` into `LOW_POWER`

### Requirement: First-Version Sun-Safe Pointing Command
When the low-battery autonomy policy activates, the first slice SHALL command ADCS into `POINTING` and SHALL set a repository-owned fixed target quaternion representing the first-version sun-safe pointing profile.

#### Scenario: Low battery commands the ADCS pointing profile
- **WHEN** the low-battery autonomy policy activates
- **THEN** the `MissionExecutive` SHALL command ADCS to `POINTING` and SHALL issue the repository-owned sun-safe target quaternion

### Requirement: Low-Battery Policy Remains Narrow
The first low-battery autonomy slice SHALL NOT automatically restore `NOMINAL`, SHALL NOT perform comm/load shedding, and SHALL NOT require a new `SUN_TRACKING` enum.

#### Scenario: First autonomy slice keeps later policy work out of scope
- **WHEN** the low-battery autonomy slice completes
- **THEN** it SHALL validate low-power entry and sun-safe pointing only, while leaving recovery, load shedding, and richer sun-tracking semantics to later changes
