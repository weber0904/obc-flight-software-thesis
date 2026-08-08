## ADDED Requirements

### Requirement: ADCS Scheduled Poll Health Is Reviewable
The ADCS subsystem SHALL expose bounded scheduled-poll health state separate from local command-path outcomes.

#### Scenario: Scheduled ADCS poll health tracks transport and valid-refresh separately
- **WHEN** `AdcsBridge.schedIn` performs the active scheduled poll
- **THEN** it SHALL update bounded ADCS poll-health truth that distinguishes scheduled transport failure from scheduled no-valid-refresh behavior
- **AND** it SHALL preserve the last valid state when the scheduled poll does not produce a valid refresh

#### Scenario: Local command-path failures stay local
- **WHEN** an operator command drives ADCS transport traffic outside the scheduled poll path
- **THEN** `AdcsBridge` MAY emit local `ADCS_COMM_ERROR` or `ADCS_SENSOR_FAULT`
- **AND** it SHALL NOT treat that command-path failure as a scheduled ADCS poll-health transition

### Requirement: ADCS Shared Recovery Incidents Are Bounded
The ADCS subsystem SHALL support one bounded ADCS FDIR detector that normalizes scheduled poll-health failures into shared recovery incidents without making every local sensor fault a shared FDIR fault.

#### Scenario: Single invalid sample remains local
- **WHEN** a scheduled ADCS poll returns one invalid sensor sample with `sensor_valid = 0`
- **THEN** the active runtime SHALL preserve the last valid ADCS state
- **AND** it SHALL emit the owned local sensor-fault observability
- **AND** it SHALL NOT enter shared ADCS recovery until the scheduled no-valid-refresh threshold is met

#### Scenario: Scheduled thresholds drive shared recovery
- **WHEN** the ADCS FDIR detector evaluates scheduled poll health in this change
- **THEN** it SHALL latch `ADCS_POLL_TRANSPORT` after `3` consecutive scheduled transport failures
- **AND** it SHALL latch `ADCS_POLL_FRESHNESS` after `3` consecutive scheduled no-valid-refresh cycles
- **AND** it SHALL clear the active ADCS detector fault on the first scheduled healthy valid cycle

## MODIFIED Requirements

### Requirement: ADCS Bridge Fallback Behavior
`AdcsBridge` SHALL poll on a schedule, SHALL retain the last valid state when replies are invalid or absent, SHALL raise the appropriate comms or sensor-fault path instead of emitting random replacement values, and SHALL keep shared ADCS FDIR input limited to scheduled poll health.

#### Scenario: Invalid ADCS reply during scheduled polling preserves FDIR boundary
- **WHEN** `AdcsBridge` receives an invalid or incomplete ADCS reply during scheduled polling
- **THEN** it SHALL preserve the last valid state and signal the degraded condition through the owned fault path
- **AND** it SHALL update only the scheduled ADCS poll-health state that the bounded ADCS FDIR detector consumes
