## ADDED Requirements

### Requirement: Scenario Replay Keeps Autonomy Inputs Available
The scenario-driven validation capability SHALL keep the existing replay contract fields available as simulator inputs, but mode-model-v2 SHALL NOT require replay-driven low-battery or high-rate conditions to command mission autonomy.

#### Scenario: Replay inputs remain data for later policy
- **WHEN** a replay timeline includes battery state-of-charge, deployment-rate angular velocity, ground-pass-open state, or link-availability state
- **THEN** the scenario bridge SHALL preserve and apply those inputs to the simulator layer as before
- **AND** it SHALL NOT claim active MissionExecutive, FDIR, or mode-policy behavior unless a later governed change adds that consumer

## REMOVED Requirements

### Requirement: Low-Battery Autonomy Scenario Replay
**Reason**: The active low-battery autonomy claim depends on retired `LOW_POWER` mode behavior.
**Migration**: Add HELL/SAFE/IDLE threshold replay coverage in `mode-autonomy-policy-v2`.

### Requirement: Deployment-Style Detumbling Autonomy Scenario Replay
**Reason**: The provisional detumbling autonomy behavior is retired from the active mode-model-v2 runtime baseline.
**Migration**: Add detumble replay coverage when the redesigned MissionExecutive/FDIR policy is introduced.
