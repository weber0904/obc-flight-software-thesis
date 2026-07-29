## Why

Current operator-facing baseline truth already runs through secure-auth plus
secure command v2, but the repository still keeps legacy command-envelope v1
and public `SESSION_OPEN` surfaces alive in current runtime/spec wording and in
adjacent wrapper inventory. That leaves a real ambiguity: later work can still
accidentally treat legacy lifecycle open as a current baseline dependency even
though the maintained secure baseline no longer uses it.

This change closes that ambiguity by retiring the external legacy v1 session
path from the current baseline, while explicitly preserving the internal
runtime secure-session semantics that secure-auth still depends on.

## What Changes

- **BREAKING** Remove public legacy `SESSION_OPEN` as a current command-ingress
  surface and fail closed on legacy command-envelope v1 lifecycle traffic.
- Preserve current secure baseline runtime session semantics driven by
  `authGranted`, including `COMMAND_SESSION_OPENED`,
  `COMMAND_SESSION_REVOKED`, and `SESSION_*` / secure-sequence telemetry.
- Re-qualify verification scripts into current maintained gates, supplemental
  historical wrappers, and retired-now surfaces before deciding closeout
  gates.
- Retire direct `SESSION_OPEN` wrapper surfaces and timing wrappers from
  current maintained baseline authority without deleting archived evidence.
- Update main specs, registry wording, roadmap, and operator docs so
  secure-auth-only is the current baseline truth and legacy v1 is historical
  compatibility only.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `core-system-contracts`: current command ingress becomes secure-auth-only,
  legacy v1 command-envelope lifecycle is unsupported and fail-closed, and
  internal synthesized session evidence remains reviewable.
- `comm-subsystem`: current COMM wording stops treating legacy
  `SESSION_OPEN(seq0)` as any part of the maintained operational boundary.
- `verification-path-registry`: legacy command-ingress and timing wrapper
  families become historical or retired registry truth instead of maintained
  closeout gates.
- `verification-evidence`: current verification rules stop requiring rerunnable
  legacy `SESSION_OPEN` or timing-wrapper evidence as maintained baseline
  proof.

## Impact

- Affected runtime/config/code:
  - `OBC/Components/CommandIngressAuthority/*`
  - `OBC/Components/CommandIngressAuthority/test/*`
  - authority catalog and command policy surfaces
- Affected docs/specs:
  - `openspec/specs/core-system-contracts/spec.md`
  - `openspec/specs/comm-subsystem/spec.md`
  - `openspec/specs/verification-path-registry/spec.md`
  - `openspec/specs/verification-evidence/spec.md`
  - `docs/verification-path-registry.md`
  - `docs/roadmap/current-baseline.md`
  - `docs/roadmap/next-work.md`
  - relevant operator runbooks
- Affected verification inventory:
  - maintained secure-baseline probes stay active only after explicit
    re-qualification
  - legacy/timing wrappers are removed from current local-ready authority
