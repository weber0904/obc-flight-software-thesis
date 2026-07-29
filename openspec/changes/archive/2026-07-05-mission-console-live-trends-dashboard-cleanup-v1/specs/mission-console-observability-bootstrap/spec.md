## ADDED Requirements

### Requirement: Trend Defaults SHALL Use Curated Continuous Operator Telemetry

Mission Console live trends SHALL default to a curated continuous-telemetry set
that reflects operator-facing continuous observation value rather than the full
runtime channel inventory.

#### Scenario: Default trend selectors expose only curated continuous channels
- **WHEN** the Mission Console builds the default trend selector catalog
- **THEN** it SHALL include only curated continuous operator telemetry from the
  approved subsystem groups
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
- **WHEN** Mission Console chooses default dashboard fields for mission posture
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
