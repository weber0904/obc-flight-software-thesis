## ADDED Requirements

### Requirement: Hosted Probe Cleanup Migration Follows Touch-On-Use Policy
The delivery workflow SHALL modernize repository-owned hosted probe cleanup on a touch-on-use basis instead of requiring a batch rewrite of every legacy probe before unrelated feature work can continue.

#### Scenario: Change reuses a legacy-cleanup probe
- **WHEN** a current change needs a repository-owned probe whose cleanup still uses the legacy ad hoc model
- **THEN** that change SHALL migrate the probe to the repository's current managed cleanup pattern before treating the probe result as governed evidence for the change
- **AND** the change SHALL verify at least one immediate rerun plus absence of owned-helper orphan or high-CPU residual processes for that migrated probe path

#### Scenario: Migrated probe still fails for unrelated path reasons
- **WHEN** a probe has already been migrated to the current cleanup model but still fails because of known architecture drift, stale assumptions, or unrelated instability outside the current change scope
- **THEN** the change MAY record bounded blocker evidence instead of forcing unrelated deep debug
- **AND** it SHALL NOT claim that failing path as passing proof for the current change
