## Why

The Mission Console target Beacon viewer currently creates temporary target
service overrides from the manual operator surface. Those overrides restart
shared OBC and UHF services, which violates the repository's A/B/C target-proof
ownership model and can interrupt unrelated target work.

## What Changes

- Move target UHF Beacon sidecar lifecycle, required service environment, and
  capture metadata into the A-layer target baseline manager.
- Make the target manual surface consume A-owned sidecar metadata and mirror
  its capture artifact locally without restarting, overriding, or stopping
  shared target services.
- Keep all remote bridge/capture cleanup owner-scoped and record sidecar
  provenance in baseline state.
- Refresh target Beacon Viewer evidence using A -> B -> C, with C limited to
  local ground helpers and evidence capture.

## Capabilities

### New Capabilities

- `target-beacon-sidecar-baseline`: A-owned target UHF Beacon sidecar lifecycle
  and reviewable metadata for consumers.

### Modified Capabilities

- `target-comm-node56-migration`: UHF node-6 service baseline gains an
  A-owned auxiliary Beacon egress path without changing serial ingress or
  authority roles.
- `mission-console`: Target Beacon Viewer consumes baseline-owned sidecar
  metadata and safely degrades when it is unavailable.
- `verification-path-registry`: Target Beacon Viewer registry wording records
  the A-owned sidecar prerequisite and C's non-interference boundary.

## Impact

- Affected code: target baseline manager, target manual-surface owner,
  sidecar helper lifecycle, Mission Console capability discovery, and focused
  target tests.
- Affected systems: governed `obc-comm-csp-stack.service` and
  `subsystem-uhf-csp.service` only through A; manual launchers no longer
  mutate those services.
- Affected evidence: target Beacon Viewer record, target baseline metadata,
  runbook, and verification-path registry.
