# mission-console-observability-bootstrap Specification

## Purpose
Define the Mission Console observability bootstrap rules that separate
continuous telemetry, change-driven posture, explicit refresh, and
event/report-style readback closure so operators can request current truth
without turning the console into ambient polling.

## Requirements

### Requirement: Explicit GET paths SHALL produce a new downlink-visible sample

For operator-state fields that still rely on shared `update on change` telemetry, an operator-triggered `GET_*` path SHALL produce a new downlink-visible sample during explicit refresh instead of only rewriting flight-side cache truth.

#### Scenario: Explicit refresh produces a new downlink sample when values are identical

- **WHEN** the operator invokes explicit refresh for a context/band whose
  current operator-state value has not changed since the last observed sample
- **AND** the paired field is backed by shared `update on change` telemetry
- **THEN** the corresponding `GET_*` path SHALL still produce a new
  downlink-visible sample for the required operator-state field
- **AND** the Mission Console SHALL be able to use that sample as fresh
  explicit-readback evidence without waiting for a later unrelated state transition.

### Requirement: Shared posture channels SHALL prefer forced resend over immediate surface duplication when feasible

When an operator-state value already has an existing primary telemetry channel, the design SHALL prefer forcing a resend on that same channel before introducing a second semantically duplicate readback surface.

#### Scenario: Existing channel supports both change notification and active query

- **WHEN** an operator-state field already exists as a shared telemetry channel
- **AND** the operator explicitly invokes its paired `GET_*` path
- **THEN** the implementation SHOULD prefer forcing a new downlink-visible
  sample on that existing channel
- **AND** it SHOULD NOT require a second semantically duplicate surface unless
  technical constraints make that impossible or the existing response is
  already naturally report-style.

### Requirement: Mission Console SHALL Keep Explicit Refresh Operator-Driven Rather Than Converting It Into Ambient Polling

Mission Console SHALL keep operator-state refresh as an operator-driven
explicit step and SHALL NOT convert dashboard state into unbounded
continuous polling merely to compensate for `update on change` blind spots.

#### Scenario: System returns to change-driven updates after explicit refresh completes

- **WHEN** the Mission Console completes an explicit refresh for the active
  context/band
- **THEN** the dashboard SHALL continue from cached latest-known values plus
  future change-driven updates
- **AND** the console SHALL NOT repeatedly rerun the same full refresh on a
  short timer unless the operator explicitly asks again.

### Requirement: Existing Report-Style Readback Surfaces MAY Remain Event-Based

Mission Console SHALL NOT require every explicit refresh or readback surface to
be converted into telemetry. Existing query-result, status-report, or
history-stream surfaces MAY remain event-based when that event already carries
the bounded result payload needed by operators.

#### Scenario: Recovery or payload status maintains a bounded result surface

- **WHEN** a `GET_*` or status command already emits a report-style event with
  the bounded payload required by the operator
- **THEN** the refresh/readback design MAY keep that command event-based
- **AND** the Mission Console SHALL treat it as a query-result surface rather
  than as ambient continuous telemetry.

### Requirement: Event-Based Readback SHALL Close On Fresh Result Evidence Rather Than Stale History

When a readback command remains event-based, Mission Console SHALL require fresh
result evidence from the current query window instead of accepting a stale event
ring hit, cached snapshot, or unrelated historical evidence as success.

#### Scenario: Single-event readback requires a fresh event from the current query

- **WHEN** the operator invokes a query whose intended response is a single
  report-style event
- **THEN** Mission Console SHALL require a matching fresh event emitted after
  the current readback marker as the primary success evidence
- **AND** it SHALL NOT accept a stale event from an earlier query as success
- **AND** it SHALL NOT treat cached snapshot state as equivalent to that fresh
  report result.

#### Scenario: Payload single-event readback requires a fully structurable payload

- **WHEN** the operator invokes a payload query whose intended response is a
  single report-style event
- **AND** a matching fresh payload event is observed after the current readback
  marker
- **BUT** the event payload cannot be fully structured into the required fields
- **THEN** Mission Console SHALL surface that result as incomplete rather than
  full success
- **AND** it SHALL NOT let the readback succeed based only on the fresh event
  name or a separate command-completion event.

### Requirement: Event-Group Readback SHALL Require Complete Fresh Group Evidence

Mission Console SHALL treat a bounded status-plus-record readback as complete
only when the fresh event group for the current query is complete enough to
represent the requested history.

#### Scenario: Persistent-fault history cannot close successfully with status only

- **WHEN** the operator invokes a history-style readback that returns a fresh
  status event followed by zero or more record events
- **THEN** Mission Console SHALL require the fresh status event plus the fresh
  record event group for the current query before closing full success
- **AND** if only the status event arrives, the console SHALL treat the result
  as incomplete unless that status explicitly declares an empty result set.

### Requirement: Completion Fallback SHALL Be Marked As Fallback Rather Than Main Evidence

Mission Console SHALL distinguish bounded command-completion fallback from
primary result evidence for readbacks whose native event/report observability is
known to vary across paths, and SHALL NOT present fallback-only closure as full
query truth.

#### Scenario: Boot queries allow bounded completion fallback

- **WHEN** an operator invokes a boot/history-confirmation readback and the
  command completes but the preferred fresh result event is not observed within
  the bounded window
- **THEN** Mission Console MAY surface bounded completion as fallback evidence
- **BUT** it SHALL label that result as fallback-only rather than as the same
  truth level as a fresh boot/recovery result event.

### Requirement: Trend Defaults SHALL Use Curated Continuous Operator Telemetry

Mission Console live trends SHALL default to a curated continuous-telemetry
set that reflects operator-facing continuous observation value rather than the
full runtime channel inventory.

#### Scenario: Default trend selectors expose only curated continuous channels
- **WHEN** the Mission Console builds the default trend selector catalog
- **THEN** it SHALL include only curated continuous operator telemetry from
  the approved subsystem groups
- **AND** it SHALL exclude channels that are classified as explicit-readback,
  change-driven-only, or diagnostics-only by the current observability
  contract.
- **AND** the first active selector tranche SHALL exclude `GPS` until a later
  governed change explicitly promotes it into continuous default trends.

### Requirement: Dashboard Defaults SHALL Prefer Operator Truth Over Autonomy Breadcrumbs

The current observability bootstrap SHALL treat primary dashboard posture as
operator truth and SHALL keep autonomy breadcrumb fields out of the primary
summary layer unless a future tranche explicitly promotes them.

#### Scenario: Mode-safety breadcrumb fields do not define primary posture
- **WHEN** Mission Console chooses default dashboard fields for mission
  posture
- **THEN** `SYS_MODE` SHALL remain the primary mission-mode truth
- **AND** `MODE_SAFETY_LAST_CURRENT_MODE` and
  `MODE_SAFETY_LAST_TARGET_MODE` SHALL be treated as secondary breadcrumb
  detail rather than primary operator posture.

### Requirement: Diagnostics-Only Residual Live Surfaces SHALL Stay Out Of Default Operator Views

The Mission Console SHALL keep residual live-chatter channels that remain
useful for engineering review, but are not promoted operator-facing telemetry,
out of the default dashboard and trend views until a later tranche explicitly
promotes them.

#### Scenario: Residual diagnostics chatter is inventoried without promotion
- **WHEN** the repository records residual continuously emitted channels that
  are still present at runtime
- **THEN** the Mission Console default dashboard and trend views SHALL keep
  those diagnostics-only channels out of operator-first summaries
- **AND** the change SHALL record their residual runtime presence as inventory
  rather than silently treating them as promoted operator truth.

### Requirement: Operator Readback Views SHALL Distinguish Saved Truth From Proof Provenance

Mission Console readback UX SHALL distinguish the saved subsystem truth that an
operator wants to inspect from the proof provenance used to verify that a
readback action closed correctly.

#### Scenario: Live continuous telemetry does not require explicit-refresh provenance in the primary viewer
- **WHEN** an operator is viewing continuously updated telemetry that already
  belongs to the curated live operator set
- **THEN** the primary viewer SHALL treat the latest saved value as the main
  operator truth
- **AND** it SHALL NOT require the operator to first interpret whether the
  sample arrived via background live update or an explicit refresh path.

#### Scenario: Explicit-readback proof provenance remains secondary but available
- **WHEN** the operator needs to inspect how a readback result was proven
- **THEN** the Mission Console SHALL still preserve proof provenance such as
  fresh-channel evidence, event evidence, bounded search usage, or fallback
  classification
- **AND** that provenance SHALL remain secondary to the saved readback values
  in the operator-facing readback view.

#### Scenario: Proof provenance terminology matches viewer-first UX
- **WHEN** Mission Console exposes proof provenance alongside saved readback
  truth
- **THEN** the operator-facing viewer SHALL describe the primary saved fields
  as latest saved values
- **AND** proof provenance SHALL be labeled as a secondary proof/debug layer
  rather than presented as the main readback payload.
