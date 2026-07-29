## Why

The current Mission Console Phase 1 surface already reuses the maintained
manual dual-GDS operator baseline for auth, commands, readback, and sequence
actions, but it still lacks a first-class operator-facing beacon viewer.
Hosted and target branches already have bounded UHF beacon ancestry through the
maintained node-`6` side channel, yet operators still need to leave Mission
Console and inspect ad hoc files or helper outputs to see the latest beacon.

For the immediate demo branch, the highest-priority gap is not RF closure or
new beacon runtime semantics; it is a reviewable operator surface that can show
the latest beacon summary on the dashboard and expose full decoded/provenance
detail on a dedicated page without pretending that beacon is just another
readback or stock-GDS event/channel stream.

## What Changes

- Add a Mission Console beacon viewer surface for both hosted and target manual
  operator contexts.
- Extend hosted and target manual-surface manifests with explicit UHF beacon
  capability metadata.
- Add a gateway-owned beacon manager and bounded latest/history persistence
  separate from readback, event, and channel caches.
- Add a minimal dashboard beacon summary card and a dedicated `/beacon` detail
  page.
- Record branch-local demo-first evidence and document the post-demo extraction
  path back toward a clean mainline merge.

## Capabilities

### New Capabilities
- `mission-console-beacon-viewer`: Mission Console hosted/target beacon viewer
  surface with dashboard summary plus detailed `/beacon` operator page.

### Modified Capabilities
- `mission-console`: manual-surface-backed Mission Console now consumes
  explicit beacon capability metadata and exposes beacon latest/history APIs.
- `verification-path-registry`: Mission Console beacon viewer proof becomes a
  separate registered operator path, distinct from lower-level UHF beacon
  runtime and suppress semantics.

## Impact

- Affected code: manual surface owners, Mission Console gateway registry,
  snapshot store, new beacon manager, dashboard/beacon UI, and focused Mission
  Console tests.
- Affected docs: Mission Console runbooks, interface contract index, current
  architecture note, verification-path registry, and a dedicated beacon viewer
  test record.
- Public behavior: hosted and target manual Mission Console contexts can now
  surface a bounded UHF beacon view without claiming stock GDS ownership,
  readback semantics, or RF/OTA closure.
