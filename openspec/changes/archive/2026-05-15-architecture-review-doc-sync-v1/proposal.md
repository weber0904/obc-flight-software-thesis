## Why

The architecture-review follow-up changes retired the active HK fallback path,
introduced the current interface index, and split ground-link health ownership.
Several active documents and specs still described retired HK file/downlink
surfaces as current acceptance criteria, or implied RSSI/SNR placeholders should
already exist in the v1 link-health view.

This change reconciles current documentation and formal-spec wording with the
post-follow-up baseline.

## What Changes

- Mark HK fallback file/downlink paths as historical retired-fallback evidence
  instead of current mission-history proof.
- Keep official `.fdp` / `DpCatalog` as the current stored-history and file
  downlink acceptance surface.
- Clarify that RSSI/SNR integration into link-health policy is deferred to a
  future radio-metrics change.
- Refresh current architecture, roadmap, operator, script, and interface-index
  wording that still pointed at pre-follow-up state.

## Capabilities

### Modified Capabilities

- `comm-subsystem`: current lab and link-health wording.
- `core-system-contracts`: command authority wording for retired HK commands.
- `platform-baseline`: active default deployment surface.
- `resource-storage`: governed runtime-root scope after HK fallback retirement.
- `verification-evidence`: current target evidence wording.
- `verification-path-registry`: historical HK fallback entries and current FDP
  entry references.

## Impact

- Affected code: none.
- Affected public contracts: documentation/spec wording only; no command,
  telemetry, topology, or runtime behavior changes.
- Verification scope: OpenSpec/spec validation and repository consistency
  checks; no fresh F' build is required for this docs/spec-only reconciliation.
