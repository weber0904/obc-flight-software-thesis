## ADDED Requirements

### Requirement: EPS Runtime Poll Health Contract
The EPS subsystem SHALL provide a deterministic runtime poll-health contract alongside the existing cached EPS status behavior so downstream runtime owners can distinguish healthy polling, transient poll failure, and repeated poll failure without inventing transport-side policy.

#### Scenario: Successful poll resets EPS poll-health state
- **WHEN** `EpsBridge` completes a scheduled EPS status poll successfully
- **THEN** it SHALL mark the last poll result as successful
- **AND** it SHALL reset the consecutive poll-failure count to `0`
- **AND** it SHALL keep the cumulative poll comm-error count unchanged for that cycle

#### Scenario: Failed poll advances EPS poll-health state
- **WHEN** `EpsBridge` fails to complete a scheduled EPS status poll because of timeout or equivalent transport failure
- **THEN** it SHALL mark the last poll result as unsuccessful
- **AND** it SHALL increment the consecutive poll-failure count
- **AND** it SHALL increment the cumulative poll comm-error count

#### Scenario: Failed poll still invalidates cached EPS status
- **WHEN** `EpsBridge` fails to complete a scheduled EPS status poll
- **THEN** it SHALL treat the cached EPS status as unavailable until a later successful status update
- **AND** it SHALL NOT leave the cache marked valid for runtime consumers

#### Scenario: Runtime health snapshot is available after any poll attempt
- **WHEN** a runtime consumer requests EPS poll-health state from `EpsBridge`
- **THEN** it SHALL receive a project-owned health snapshot that includes cache-validity, last-poll-success, consecutive poll failures, and cumulative poll comm errors

## MODIFIED Requirements

### Requirement: EpsBridge Public Behavior
`EpsBridge` SHALL own the `EPS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, SHALL preserve the last valid state on comms failure while raising the EPS comms error path, and SHALL maintain deterministic runtime poll-health state for repeated poll failure handling.

#### Scenario: EPS timeout preserves last valid telemetry but invalidates cache
- **WHEN** `EpsBridge` fails to receive a valid simulator response within the configured timeout
- **THEN** it SHALL emit the comms error behavior
- **AND** it SHALL NOT overwrite EPS telemetry with arbitrary replacement values
- **AND** it SHALL invalidate cached EPS status for runtime cache consumers
- **AND** it SHALL advance the owned poll-health counters for that failure
