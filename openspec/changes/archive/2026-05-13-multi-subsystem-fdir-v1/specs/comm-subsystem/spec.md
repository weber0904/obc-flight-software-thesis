## ADDED Requirements

### Requirement: COMM Fault Detection Is Reviewable And Bounded
The comm subsystem SHALL detect bounded current-primary link failure for the shared recovery path while keeping operator role policy separate from fault policy.

#### Scenario: COMM detector distinguishes policy events from fault events
- **WHEN** an operator switches the primary link or pass state starts or stops
- **THEN** COMM SHALL continue to treat those transitions as policy or observability events
- **AND** it SHALL NOT treat them by themselves as shared-recovery COMM fault incidents

#### Scenario: COMM fault thresholds are fixed
- **WHEN** the active baseline evaluates the current primary COMM link during scheduled ticks
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE` after `3` consecutive cycles where that primary link is unavailable
- **AND** it SHALL latch `COMM_PRIMARY_TRANSPORT` after `3` consecutive cycles where the primary link remains available but its cumulative `tx/rx` error total grows
- **AND** it SHALL clear the active COMM detector fault on the first scheduled cycle where the primary link is available and has no new `tx/rx` error growth

### Requirement: Fault-Driven COMM Actuation Is Executor-Owned
The comm subsystem SHALL route fault-driven primary-link switching, session revoke, and shared downlink-owner clear through the shared recovery owner instead of performing them in the detector path.

#### Scenario: Detector-side link loss does not switch primary roles directly
- **WHEN** COMM detects a current-primary fault in this change
- **THEN** `CommController` SHALL emit reviewable fault or clear truth
- **AND** it SHALL NOT directly switch the primary link set, revoke the current primary command session, or clear downlink owners from the detector-side fault transition alone

#### Scenario: Executor-owned failover returns structured truth
- **WHEN** the shared recovery owner requests bounded COMM recovery actuation
- **THEN** `CommController` SHALL return structured failover truth including whether a switch occurred, whether the runtime was already on a healthy primary, whether no healthy backup existed, how many sessions were revoked, how many owners were cleared, and the final primary command, telemetry, and file links

## MODIFIED Requirements

### Requirement: COMM Converges State On Link Loss Or Primary Switch
The comm subsystem SHALL converge authenticated-session policy and shared downlink ownership state when the current primary link becomes unavailable or the primary-link role changes, but fault-driven convergence SHALL now be performed through the shared recovery owner.

#### Scenario: Fault-driven link loss uses the shared recovery owner
- **WHEN** the current primary COMM link becomes unavailable because of a shared-recovery COMM fault
- **THEN** the active runtime SHALL converge session and shared downlink-owner state through executor-owned COMM recovery actuation
- **AND** `CommController` SHALL leave direct detector-side failover and direct detector-side session revoke out of scope for that fault transition

#### Scenario: Fault-driven failover does not auto-restore nominal S-band
- **WHEN** executor-owned COMM recovery fails over away from the nominal primary link because of a fault
- **THEN** the active runtime SHALL leave later nominal-link restoration to a separate governed operator or future recovery action
- **AND** it SHALL NOT auto-restore S-band merely because S-band later becomes healthy again
