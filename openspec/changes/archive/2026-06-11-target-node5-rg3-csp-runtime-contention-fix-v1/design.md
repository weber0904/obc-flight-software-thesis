## Context

The source debug branch proved a specific target-side RG3 blocker on the
service-managed node-`5` secure-auth path. The root cause was not intrinsic
RG3 compute load. The blocker was observability work waiting behind the shared
internal CSP runtime mutex while other callers were doing blocking `ping` or
request/reply traffic.

That investigation used additional diagnostics and dirty vendored F' timing
instrumentation. Those surfaces were useful to isolate the bug, but they are
not required to keep the bounded product fix in mainline. This salvage change
therefore keeps only the product-facing code fixes and a narrowed evidence
record, then leaves the broader CSP client architecture rework to the follow-up
owner-component change.

## Goals / Non-Goals

**Goals:**

- Preserve the smallest product fix that removes the proven RG3 blocker.
- Keep runtime observer reads bounded without changing business-traffic
  serialization.
- Record a clean-branch evidence trail that is reviewable without depending on
  dirty submodule patches.

**Non-Goals:**

- No continued mainline support for the investigation-only diagnostics.
- No cadence tuning, queue resizing, or thread-priority retuning.
- No attempt to solve the remaining RG1 contention in this change.
- No full CSP client architecture rewrite; that belongs to the follow-up
  `CspRuntimeOwner` change.

## Decisions

### 1. Keep `CommController` runtime-state reads on a cached snapshot

`CommController::getStateForRuntime()` now returns a cached
`OBC::CommRuntimeState` that is refreshed when the component publishes or
changes the underlying runtime-visible state.

This removes unnecessary recomputation pressure from runtime observers while
preserving the external runtime contract.

Alternative considered:

- keep rebuilding the runtime state on every observer call
  - rejected because the source investigation showed runtime observers were
    part of the avoidable auth-open pressure path.

### 2. Keep CSP metrics reads bounded with a cache, not a lock redesign

`LibCspRuntime::metrics()` now uses `try_to_lock`. If blocking CSP traffic
already owns the runtime mutex, metrics readers reuse the latest cached
snapshot. The cache is refreshed after `init`, `ping`, `sendRaw`,
`requestReply`, and `shutdown`.

This is the smallest fix that removes the proven RG3 blocker without changing
the existing shared-runtime business-traffic serialization model.

Alternatives considered:

- lower subsystem probe cadence first
  - rejected as the mainline fix because the salvage target is the proven RG3
    blocker, not the broader RG1 follow-up.
- redesign the runtime lock model in this change
  - rejected because that is larger than required for bounded RG3 closure and
    will be handled in the owner-component follow-up.

### 3. Keep evidence narrow and submodule-clean

The formal evidence record for this salvage change may cite the source-branch
investigation roots as diagnosis input, but the clean-branch proof must stand
without carrying the dirty `lib/fprime` member-timing patch into mainline.

That means the checked-in record focuses on:

- the salvaged code delta
- the pre-fix diagnosis class established on the source branch
- the clean-branch local verification
- a clean-branch target rerun that confirms RG3 no longer slips on the
  reproduced node-`5` auth-open path

### 4. Stop the claim at RG3 closure

This change explicitly does not over-claim full target timing closure. The
evidence record must keep any remaining RG1 contention separate so the
follow-up runtime-owner refactor starts from a clean and truthful baseline.

## Risks / Trade-offs

- [Risk] Cached runtime state and cached runtime metrics can be slightly stale.
  - Mitigation: both caches are refreshed on the existing state-change and
    runtime-operation boundaries that matter for observability.
- [Risk] Reviewers could mistake the source-branch investigation inputs for
  current mainline dependencies.
  - Mitigation: the narrowed evidence record explicitly labels those inputs as
    diagnosis provenance, not as required mainline diagnostics.
- [Risk] Closing the RG3 blocker makes RG1 contention more visible.
  - Mitigation: that is an intended outcome and is recorded as separate
    follow-up work.
