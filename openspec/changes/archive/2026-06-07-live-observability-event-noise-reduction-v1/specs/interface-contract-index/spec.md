## ADDED Requirements

### Requirement: Interface Index Records Current Event-Quieting Boundaries

`docs/interfaces.md` SHALL describe the current live event surface after the
event-noise reduction changes so reviewers can distinguish operator-facing
events from retained local/debug-only visibility.

#### Scenario: Current docs distinguish mask-change events from periodic reduction
- **WHEN** reviewers inspect the reduced-state observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that `STATE_MONITOR_UPDATED` is emitted on
  reduced-state mask changes rather than on every successful scheduled
  reduction.

#### Scenario: Current docs distinguish packetized operator events from local debug events
- **WHEN** reviewers inspect the COMM or data-product observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that success `CSP_PING_RESULT` is not part
  of the default packetized ground live event surface
- **AND** it SHALL state that `HK_TREND_PRODUCT_WRITTEN` remains operator-facing
  while `DpWriter.FileWritten` is local/debug-only visibility by default.
