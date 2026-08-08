## 1. A-Owned Sidecar Baseline

- [x] 1.1 Add A-owned remote bridge/capture lifecycle with unique label, PID
  metadata, health checks, and targeted repair.
- [x] 1.2 Move Beacon OBC/UHF service environment into dedicated A-managed
  baseline drop-ins and record effective metadata in the readiness JSON.
- [x] 1.3 Ensure A's stale-override and repair logic recognizes the new
  baseline-owned configuration without treating it as probe residue.

## 2. Manual Surface Consumption

- [x] 2.1 Retire target manual owner normal-path service overrides, remote bridge startup,
  and remote cleanup ownership.
- [x] 2.2 Consume A-published sidecar metadata, mirror the capture locally,
  and omit capability safely when baseline metadata is absent/unhealthy.
- [x] 2.3 Add focused tests covering owner-scoped sidecar metadata, no manual
  service mutation, and Mission Console fallback.

## 3. Governed Evidence And Documentation

- [x] 3.1 Run targeted static/unit checks and A -> B -> C target proof with
  before/after shared-service invocation identity.
- [x] 3.2 Refresh target Beacon Viewer record, manual runbook, registry, and
  change artifacts with A-owned sidecar provenance and non-claims.
