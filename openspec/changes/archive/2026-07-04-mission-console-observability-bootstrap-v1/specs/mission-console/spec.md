## MODIFIED Requirements

### Requirement: Mission Console SHALL Distinguish Continuous, Change-Driven, Explicit Readback, And Diagnostics Surfaces

The Mission Console SHALL expose distinct surface/lifecycle truth, dashboard
continuous values, change-driven operator state, transition events, explicit
detailed readback, and diagnostics-only review surfaces.
Dashboard state SHALL retain latest known values and SHALL NOT depend solely
on future state transitions becoming visible before an explicit status request
can return fresh proof. For important operator state fields backed by shared
`update on change` telemetry, the paired explicit readback path SHALL produce
a new downlink-visible sample when the operator explicitly requests current
status.

#### Scenario: Dashboard keeps continuous, change-driven, and explicit readback distinct

- **WHEN** operators inspect the Mission Console dashboard and readback pages
- **THEN** the dashboard SHALL present continuous values, change-driven
  operator state, and transition state without expanding into a raw full
  channel/event flood
- **AND** detailed `GET_*`, `PAYLOAD_*`, `SEQ_LOG_STATUS`, boot, recovery, and
  persistent-fault observations SHALL remain explicit bounded readback actions.

#### Scenario: Explicit status request does not silently return only cached truth

- **WHEN** a dashboard-facing operator-state field is backed only by change-driven or
  shared telemetry and the operator explicitly requests current status
- **THEN** the Mission Console SHALL use its defined explicit readback path to
  obtain a new downlink-visible sample when needed
- **AND** it SHALL NOT silently treat a cache-only rewrite as successful fresh proof.
