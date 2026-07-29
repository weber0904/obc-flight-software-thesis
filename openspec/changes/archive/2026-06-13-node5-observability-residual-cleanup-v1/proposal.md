## Why

`sband-live-observability-tier-selection-v1` solved who may see node-`5`
post-auth S-band live `event/tlm`, but it intentionally left a residual set of
runtime surfaces under the vague label `non-baseline live`. That ambiguity now
conflicts with current operator docs, current COMM evidence obligations, and
the already-owned `WatchdogSupervisor` `SYS_*` resource surface.

This change now continues as the same governed change rather than closing out
immediately. It first rebuilds the OpenSpec and canonical-doc contract around a
fresh residual inventory, then uses the same change to repair any required
runtime, probe, or proof-chain drift before the final local-ready closeout.

The immediate branch goal is therefore narrower than "rerun everything and
archive now": it is to make the current baseline answer, precisely, which
surfaces are formal pass-time truth, which must remain reviewable
proof/transport/policy observability, which belong behind bounded readback,
which remain diagnostics-only, and which adjacent detailed `GET_*` proof
surfaces require same-change requalification.

## What Changes

- Create a formal residual inventory for current node-`5` post-auth live
  surfaces covering `SystemResources`, `ComCcsds` / `OBCComCcsds` queue
  surfaces, `CspRuntimeOwner`, `GroundLinkDriver`,
  `GroundLinkHealthProvider`, `CommEgressMux`, `UartDriver`, and current
  `CommController` residual state/counter/event surfaces.
- Rebuild the change workflow into explicit phases:
  - fresh inventory / drift diagnosis
  - OpenSpec + canonical-doc alignment
  - runtime / probe / proof-chain implementation
  - final verification + closeout
- Fix the current governance vocabulary to four buckets:
  - formal pass-time keep-live truth
  - formal reviewable proof / transport / policy observability
  - bounded fresh readback / review command
  - diagnostics-only / non-baseline live
- Promote `WatchdogSupervisor` `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY` as the formal node-`5`
  resource keep-live truth.
- Demote `SystemResources.*` from node-`5` pass-time operator truth while
  keeping `SystemResources.ENABLE` governed as a runtime configuration
  control.
- Keep `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`,
  `ComCcsds.QueueOverflow`, `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`,
  and S-band `CommEgressMux` counters as formal reviewable
  proof/transport/egress observability rather than broad “non-baseline live”.
- Keep operator-facing `CommController` state-transition surfaces as formal
  node-`5` pass-time truth, classify policy counters and policy-status
  surfaces as formal reviewable observability, and keep remaining reliable
  transfer / suppress / retry internals diagnostics-only.
- Requalify the representative detailed `GET_*` packetized proof path inside
  this same change if fresh diagnosis shows proof-chain or runtime drift.
- Update current docs, operator runbooks, verification-path wording, and the
  active evidence/work record so the same boundary is stated consistently
  across the branch while implementation is still in progress.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: clarify that node-`5` residual governance now distinguishes
  pass-time truth from reviewable transport/policy observability and from
  diagnostics-only residuals.
- `core-system-contracts`: formalize `WatchdogSupervisor` `SYS_*` as the
  current node-`5` resource truth and keep `SystemResources.ENABLE` under the
  existing governed runtime-config boundary.
- `interface-contract-index`: require a per-surface residual inventory with
  explicit owner/component mapping and final bucket decisions.
- `onboard-data-products-and-live-beacon`: keep node-`5` keep-live summary
  distinct from reviewable proof surfaces and diagnostics-only residuals.
- `verification-path-registry`: record that hosted and target node-`5`
  observability proof now includes explicit residual classification rather than
  a generic `non-baseline live` catch-all, and keep the same path identity
  while detailed `GET_*` proof is requalified inside this change if needed.
- `verification-evidence`: preserve existing reviewable `GROUND_LINK_TX_BYTES`
  obligations and add explicit evidence requirements for resource / transport /
  queue / owner / egress residual classification together with any required
  detailed `GET_*` requalification evidence.

## Impact

- Affected docs:
  - `docs/interfaces.md`
  - `docs/architecture/current-development-architecture.md`
  - `docs/roadmap/current-baseline.md`
  - `docs/roadmap/next-work.md`
  - `docs/operator/hosted-per-band-stock-ground-stacks-runbook.md`
  - `docs/operator/target-obc-comm-csp-lab-runbook.md`
  - `docs/operator/hosted-official-sequencing-system-resources-runbook.md`
  - `docs/verification-path-registry.md`
  - `docs/test-records/node5-observability-residual-cleanup-v1/README.md`
- Affected formal artifacts:
  - delta specs for `comm-subsystem`, `core-system-contracts`,
    `interface-contract-index`, `onboard-data-products-and-live-beacon`,
    `verification-path-registry`, and `verification-evidence`
- Affected runtime or probe boundary:
  - focused hosted and target node-`5` observability probes now become
    implementation assets rather than immediate closeout gates; the same change
    may repair their oracle or supporting runtime path before final reruns
  - this change still does not introduce a new command plane or broad new
    runtime filtering framework
- Non-goals:
  - no secure-auth redesign
  - no second command plane
  - no generic telemetry schema rewrite
  - no repo-wide central classifier
  - no broad UHF baseline expansion
