## Context

The repository already froze the maintained hosted near-term simultaneous
operator baseline in entry `43B`:

- two distinct stock `fprime-gds` plus `ground_ttc_gateway` surfaces
- one shared hosted `TopCcsds` runtime
- one combined wrapper that stays composition-only

That layer-1 baseline is intentionally not an orchestration owner. It proves
start/stop, manifests, owned runtime roots, and bounded non-interference, but
it does not own an authoritative orchestration lifecycle contract. The review
feedback on the initial plan was correct: if this change introduces reviewable
orchestration lifecycle/failure/cleanup state, the repository must honestly
call that a thin lifecycle owner rather than continuing to describe it as a
mere wrapper.

This change therefore adds a distinct hosted-only layer-2 owner above `43B`
while keeping COMM runtime semantics, gateway relay meaning, and target-bearing
simultaneous claims outside scope.

## Goals / Non-Goals

**Goals:**

- add one hosted-only thin lifecycle owner above the maintained per-band
  stock-stack baseline
- define a hard criterion between:
  - layer-1 composition-only wrapper
  - layer-2 thin lifecycle owner
- make the orchestration proof depend only on orchestration-owned status,
  manifest, failure, and cleanup surfaces
- keep `43B` as the unchanged layer-1 maintained baseline and register a new
  reusable path only for the distinct layer-2 boundary
- sync current-facing docs so they consistently describe three layers:
  - maintained per-band stock-stack baseline
  - hosted orchestration owner
  - deferred target-bearing simultaneous work

**Non-Goals:**

- no command dispatch or command authority ownership
- no session/profile composition policy owner
- no gateway relay ownership or one-gateway multiplexer behavior
- no stock-GDS plugin behavior or one-GDS heterogeneous upstream aggregation
- no COMM runtime ownership inside OBC
- no RF, link-health, reliable-transfer, or target-bearing simultaneous claim
- no reopening of UHF packet-quiet, beacon-suppress, or file/downlink semantics

## Decisions

### 1. Hard-separate layer-1 wrapper from layer-2 owner

This change adopts the following repository-local criterion:

- `composition-only wrapper` may coordinate launch and passive aggregation of
  existing layer-1 manifests or logs, but it must not introduce authoritative
  lifecycle states, failure classes, or cleanup verdicts that later probes or
  operators depend on.
- `thin lifecycle owner` may own authoritative hosted-only lifecycle state,
  owned transitions, startup-failure classification, and cleanup summary for a
  higher-level operator surface, while still delegating per-band TT&C behavior
  and runtime semantics to adjacent layers.

Because the new proof must validate lifecycle/failure/cleanup through
orchestration-owned artifacts, this change intentionally takes the second path
and labels the result a thin lifecycle owner.

### 2. Keep the layer-2 owner additive and above `43B`

The existing `scripts/per_band_stock_ground_stacks.py` and maintained S-band,
UHF, and combined layer-1 launchers remain baseline truth. The new owner will
not silently repurpose the combined wrapper or change `43B` wording.

Instead, the layer-2 entrypoint will:

- create its own owner-root, manifest, status, and logs
- run the existing combined layer-1 stack beneath it
- record that layer-1 manifest as a delegated prerequisite rather than the new
  owner contract

This keeps the proof and registry split honest.

### 3. Limit owner responsibility to hosted lifecycle/state only

The layer-2 owner may own:

- preflight validation for its declared hosted surface
- lifecycle phase transitions
- owned process-set reporting
- shared hosted runtime coordination summary
- startup failure and cleanup failure classification
- explicit delegated-boundary statements

It may not own:

- command-path decisions
- `SESSION_OPEN(seq0)` or COMM role semantics
- gateway routing or byte relay behavior
- stock GDS adaptation logic
- reliable-transfer or file policy
- target/lab simultaneous claims

### 4. Use orchestration-owned oracle only

The new proof must not depend on old COMM semantic oracles. In particular it
must not use `run_comm_session_and_downlink_qos_probe.sh`, and it must not
require any event or channel observation whose meaning belongs to COMM runtime
policy rather than the new owner.

The allowed proof oracle is limited to:

- orchestration manifest and status JSON
- transition history written by the owner
- process/listener cleanup inspection
- delegated layer-1 manifest existence and non-regression rerun through `43B`

### 5. Register a new path only if the boundary is genuinely distinct

The new hosted orchestration path will be registered separately from `43B`
because it proves a different claim:

- `43B`: maintained layer-1 per-band stock surfaces on one shared runtime
- new path: hosted layer-2 orchestration owner lifecycle/failure/cleanup
  contract above that baseline

If the implementation drifted back into merely aggregating `43B` outputs, the
correct action would be to stop and narrow the change rather than registering a
duplicate path. This design assumes the owner remains distinct.

## Data / Artifact Shape

The new owner surface will write at least:

- `manifest.json`
  - `ownerType=thin-lifecycle-owner`
  - `layer1Baseline`
  - `ownedTransitions`
  - `nonOwnedAdjacentState`
  - `failureBehavior`
  - `cleanupBehavior`
  - `sharedRuntimeInteraction`
  - `nonClaims`
- `status.json`
  - `requestedMode`
  - `lifecyclePhase`
  - `phaseHistory`
  - `ownedProcessSet`
  - `sharedRuntimeState`
  - `failure` or `cleanup`

The layer-2 owner remains hosted-only and combined-surface-only.

## Risks / Trade-offs

- **[Risk] The new owner could be mistaken for COMM runtime authority**
  - Mitigation: keep delegated state explicit in the manifest, runbook, spec
    deltas, and evidence non-claims.

- **[Risk] The new path could collapse back into `43B` wording**
  - Mitigation: require a distinct owner-root, distinct artifacts, and a new
    proof oracle that `43B` never owned.

- **[Risk] Failure-path proof could accidentally depend on child-process
  behavior instead of the owner contract**
  - Mitigation: give the owner an explicit hosted preflight port-availability
    check so the bounded startup-failure case is classified by the owner
    itself.

## Migration Plan

1. Create proposal, design, tasks, and four minimal delta specs.
2. Add a distinct hosted orchestration helper and launcher above the existing
   combined layer-1 helper.
3. Add a hosted orchestration proof that validates only orchestration-owned
   lifecycle/failure/cleanup surfaces.
4. Add a new evidence record and a distinct registry entry for the layer-2
   owner path.
5. Sync current-facing docs and inventory wording so the three-layer split is
   consistent everywhere.

## Open Questions

- None. This design intentionally locks the result to a hosted-only thin
  lifecycle owner and keeps broader runtime or target-bearing work deferred.
