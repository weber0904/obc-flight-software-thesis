## Context

This change is intentionally demo-first and branch-local. It sits on top of
the current `feature/route1-sequence-verification-v1` branch family because
that branch already carries the target/manual surface truth, scoped CAN FD
baseline, and Mission Console operator flow that tomorrow's demo depends on.
The goal here is to add a cleanly isolated Mission Console beacon viewer,
without forcing a premature extraction back to `main` before the parent branch
itself is fully reviewed and merged.

## Goals / Non-Goals

**Goals**
- Show hosted and target latest beacon summary directly in Mission Console.
- Keep the dashboard beacon surface intentionally small: last beacon time and
  sequence only.
- Put decoded beacon details, provenance, and bounded history on a dedicated
  `/beacon` page.
- Keep beacon state separate from readback, channels, and events.
- Preserve existing Mission Console auth/readback/Route 1 manual surfaces.

**Non-Goals**
- No RF or OTA beacon closure.
- No stock GDS beacon UI integration.
- No new command/readback surface for beacon.
- No immediate requirement to archive or merge this child branch into `main`
  before the parent Route 1 branch is reviewed.
- No change to lower-level beacon runtime semantics such as suppress policy,
  timeout, or authenticated ownership rules.

## Decisions

### Beacon viewer uses explicit manual-surface capability metadata

Mission Console does not infer beacon support from events, channels, or known
band names. Hosted and target manual-surface manifests explicitly export a
UHF-side beacon capability block that includes source kind, source band,
capture path, frame size, and decode tool ancestry.

### Dashboard summary stays intentionally narrow

The dashboard only surfaces `Last Beacon Time` and `Sequence`. Source kind,
source band, decode state, and detailed field payload stay on `/beacon` so the
dashboard does not turn into a second diagnostics page.

### Hosted and target source distinction is first-class

Hosted uses a local PTY side-channel truth:

- `sourceKind=hosted-pty-side-channel`

Target uses a remote sidecar truth:

- `sourceKind=target-remote-sidecar`

Mission Console must show that distinction explicitly on `/beacon` and in the
API payload so operators do not mistake target-side mirrored artifacts for
direct stock-GDS or RF reception.

### Target beacon viewer depends on always-on sidecar capability

The target manual ground surface owns the remote beacon sidecar lifecycle. This
is a maintained manual-surface capability, not a probe-only helper. The target
surface therefore mirrors a target-side beacon capture artifact into the local
ground surface root and advertises that path to Mission Console.

### Manual-surface owners detach from launcher sessions

Hosted and target manual-surface launch scripts start their owner through a
shared detached-launch helper. The owner becomes the leader of a new OS
session, retains its launcher log, and remains supervised by the existing
`surface_owner.py` signal and cleanup contract. This prevents a Codex exec
harness teardown from killing only the top-level owner while leaving its GDS
or gateway children behind.

The detach change does not alter target A/B/C ownership: target ground
baseline startup remains in B, and the manual owner still owns only its
operator-side processes.

## Data Model

Latest beacon payload:

- `contextId`
- `supported`
- `available`
- `reason`
- `selectedBand`
- `sourceBand`
- `sourceKind`
- `capture.path`
- `capture.sizeBytes`
- `capture.frameSize`
- `capture.frameCount`
- `decode.status`
- `decode.error`
- `summary`
- `decoded`
- `lastObservedAt`
- `sequence`

Dashboard summary payload:

- `supported`
- `available`
- `lastObservedAt`
- `sequence`
- `reason`

History persistence:

- `latest-beacon.json`
- `beacon-history.json`

with a bounded ring retained by the Mission Console runtime root.

## Risks / Trade-offs

- The target sidecar relies on current branch-local manual surface plumbing and
  remote helper availability; it is not yet a `main`-archived baseline.
- Mirrored target beacon files are a ground-side copy of target-side managed
  capture artifacts, not a direct RF/OTA receipt claim.
- The current proof is branch-local and demo-ready, but post-demo extraction
  still needs a clean parent-branch merge or cherry-pick path.

## Post-Demo Extraction Path

1. Keep beacon-specific implementation and docs in isolated commits.
2. If the parent Route 1 branch merges cleanly, rebase this child branch on
   the merged parent or `main`.
3. If the parent needs reshaping, cherry-pick only the beacon viewer commits to
   a new clean branch.
4. Do not archive this change into `main` specs until the parent dependency is
   resolved and the branch-local truth is ready to promote.
