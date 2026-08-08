# scenario-driven-validation Specification

## Purpose
Define the hosted offline scenario replay baseline that drives simulator-facing mission context into the EPS and ADCS layers without feeding scenario truth directly into the OBC runtime.
## Requirements
### Requirement: Offline Scenario Timeline Contract
The repository SHALL define a project-owned offline scenario timeline contract for hosted replay, and that contract SHALL represent at least replay time, sunlight state, battery state-of-charge, ground-pass-open state, link-availability state, and deployment-rate angular velocity inputs.

#### Scenario: Timeline file provides the required mission-context fields
- **WHEN** a developer prepares a first-version replay file for hosted scenario validation
- **THEN** the file SHALL encode the required replay time, EPS-facing environment values, comm-context values, and ADCS deployment-rate values using the documented project-owned contract

### Requirement: Host-Side Scenario Bridge
The project SHALL provide a host-side scenario bridge that reads the offline timeline contract, selects the current replay sample for a requested replay time, and applies that sample to the hosted simulator layer instead of feeding the OBC runtime directly.

#### Scenario: Bridge applies a replay sample to hosted simulators
- **WHEN** the replay driver advances to a requested mission time
- **THEN** the scenario bridge SHALL select the corresponding timeline sample and SHALL update the relevant hosted simulator inputs without requiring direct scenario ingestion inside the OBC runtime

### Requirement: Bridge State Preserves Future Comm Context
The first scenario bridge SHALL preserve the current ground-pass-open and link-availability values as reviewable bridge state even if no comm or autonomy consumer uses them yet.

#### Scenario: Future comm-context fields remain observable
- **WHEN** a replay sample updates ground-pass-open or link-availability
- **THEN** the bridge SHALL retain those values in its current replay state so later simulator or mission-level consumers can reuse the same scenario contract

### Requirement: Scenario Replay Keeps Autonomy Inputs Available
The scenario-driven validation capability SHALL keep the existing replay contract fields available as simulator inputs, but mode-model-v2 SHALL NOT require replay-driven low-battery or high-rate conditions to command mission autonomy.

#### Scenario: Replay inputs remain data for later policy
- **WHEN** a replay timeline includes battery state-of-charge, deployment-rate angular velocity, ground-pass-open state, or link-availability state
- **THEN** the scenario bridge SHALL preserve and apply those inputs to the simulator layer as before
- **AND** it SHALL NOT claim active MissionExecutive, FDIR, or mode-policy behavior unless a later governed change adds that consumer
