## ADDED Requirements

### Requirement: Explicit GET paths SHALL produce a new downlink-visible sample

For operator-state fields that still rely on shared `update on change` telemetry, an operator-triggered `GET_*` path SHALL produce a new downlink-visible sample during explicit refresh instead of only rewriting flight-side cache truth.

#### Scenario: 值相同時，明確刷新仍能取得新的下傳證據

- **WHEN** the operator invokes explicit refresh for a context/band whose
  current operator-state value has not changed since the last observed sample
- **AND** the paired field is backed by shared `update on change` telemetry
- **THEN** the corresponding `GET_*` path SHALL still produce a new
  downlink-visible sample for the required operator-state field
- **AND** the Mission Console SHALL be able to use that sample as fresh
  explicit-readback evidence without waiting for a later unrelated state transition.

### Requirement: Shared posture channels SHALL prefer forced resend over immediate surface duplication when feasible

When an operator-state value already has an existing primary telemetry channel, the design SHALL prefer forcing a resend on that same channel before introducing a second semantically duplicate readback surface.

#### Scenario: 既有 channel 可同時支援變更通知與主動查詢

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

#### Scenario: 明確刷新完成後，系統回到平常的變化驅動更新

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

#### Scenario: recovery 或 payload status 維持有界結果面

- **WHEN** a `GET_*` or status command already emits a report-style event with
  the bounded payload required by the operator
- **THEN** the refresh/readback design MAY keep that command event-based
- **AND** the Mission Console SHALL treat it as a query-result surface rather
  than as ambient continuous telemetry.

### Requirement: Event-Based Readback SHALL Close On Fresh Result Evidence Rather Than Stale History

When a readback command remains event-based, Mission Console SHALL require fresh
result evidence from the current query window instead of accepting a stale event
ring hit, cached snapshot, or unrelated historical evidence as success.

#### Scenario: 單筆結果型 event readback 只能靠這次查詢的新事件成立

- **WHEN** the operator invokes a query whose intended response is a single
  report-style event
- **THEN** Mission Console SHALL require a matching fresh event emitted after
  the current readback marker as the primary success evidence
- **AND** it SHALL NOT accept a stale event from an earlier query as success
- **AND** it SHALL NOT treat cached snapshot state as equivalent to that fresh
  report result.

#### Scenario: payload 單筆結果型 event readback 需要完整可結構化 payload

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

#### Scenario: persistent-fault history 不可只收到 status 就關閉成功

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

#### Scenario: boot 類查詢允許 bounded completion fallback

- **WHEN** an operator invokes a boot/history-confirmation readback and the
  command completes but the preferred fresh result event is not observed within
  the bounded window
- **THEN** Mission Console MAY surface bounded completion as fallback evidence
- **BUT** it SHALL label that result as fallback-only rather than as the same
  truth level as a fresh boot/recovery result event.
