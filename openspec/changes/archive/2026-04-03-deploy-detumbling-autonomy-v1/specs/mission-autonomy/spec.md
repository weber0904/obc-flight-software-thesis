## MODIFIED Requirements

### Requirement: First-Version Mission Executive
The project SHALL provide a first-version `MissionExecutive` component that observes cached subsystem state and issues system-level autonomy actions without moving scenario ingestion into the OBC runtime.

#### Scenario: Mission executive reacts to high-rate ADCS state
- **WHEN** cached ADCS state indicates the angular-rate norm is above the mission detumble threshold
- **THEN** the `MissionExecutive` SHALL drive the corresponding detumble response through the owned runtime control boundary

### Requirement: Low-Battery Policy Remains Narrow
The first low-battery autonomy slice SHALL NOT automatically restore `NOMINAL`, SHALL NOT perform comm/load shedding, and SHALL NOT require a new `SUN_TRACKING` enum.

#### Scenario: High-rate detumbling takes priority over sun-safe pointing
- **WHEN** the mission executive simultaneously sees a low-battery condition and a high-rate detumbling condition
- **THEN** it SHALL prioritize the detumble ADCS response until the angular-rate norm falls below the mission detumble threshold

## ADDED Requirements

### Requirement: High-Rate Entry To DETUMBLE
The mission-autonomy capability SHALL command ADCS into `DETUMBLE` when the cached ADCS angular-rate norm exceeds the existing mission detumble threshold.

#### Scenario: High angular rate triggers detumble mode
- **WHEN** cached ADCS state reports an angular-rate norm above the mission detumble threshold
- **THEN** the `MissionExecutive` SHALL command ADCS to `DETUMBLE`
