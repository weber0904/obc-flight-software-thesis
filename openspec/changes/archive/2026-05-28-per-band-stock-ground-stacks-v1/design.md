## Context

The current baseline already converged on three truths that this change must
respect:

- stock `fprime-gds` remains a single communication-adapter operator surface
- `ground_ttc_gateway` remains a one-northbound or one-southbound raw relay
- near-term simultaneous operator access before any higher-level orchestration
  exists is expected to use separate per-band stock stacks

The missing piece is maintainability. Today that dual-surface hosted topology
exists mainly as probe-owned logic inside
`scripts/run_comm_session_and_downlink_qos_probe.sh` and adjacent hosted proofs.
That is enough to prove policy or transport behavior, but it is not enough to
serve as the repository's maintained operator baseline or as a clean handoff
layer for future orchestration work.

This change therefore needs to extract the shared hosted dual-surface topology
into a maintained launcher and helper surface, add a dedicated hosted operator
runbook, and register one hosted-first proof boundary that stays honest about
what is new versus what is only reused.

## Goals / Non-Goals

**Goals:**

- make the frozen hosted near-term simultaneous operator model runnable through
  maintained repo-owned entrypoints
- preserve one shared hosted `TopCcsds` runtime while exposing two distinct
  stock operator surfaces:
  - S-band stock GDS plus gateway plus node `5`
  - UHF stock GDS plus gateway plus node `6`
- expose one launcher per band and one combined composition-only wrapper
- print reviewable operator manifests that capture ports, endpoints, runtime
  roots, logs, startup order, and cleanup order
- add hosted-first evidence and a registry entry that later work can cite as
  the maintained baseline
- sync current-facing docs so they match the post-2026-05-28 baseline and stop
  treating `comm-dual-link-orchestration-v1` as if it were the current truth

**Non-Goals:**

- no new custom multi-band GDS plugin
- no one-GDS heterogeneous upstream integration claim
- no one-gateway simultaneous S-band or UHF multiplexer claim
- no target-bearing simultaneous dual-link runtime proof
- no reopening of UHF packet-quiet, beacon-suppress, reliable-transfer, RF, or
  security boundaries
- no policy/orchestration ownership in the new combined wrapper

## Decisions

### 1. Keep one shared hosted runtime and compose distinct stock stacks around it

The maintained baseline will keep one shared hosted `TopCcsds` runtime and
surround it with two distinct ground/operator surfaces. This matches the
existing probe-owned dual-surface topology, preserves current node `5` and
node `6` truths, and avoids inventing a second runtime-owner model.

Alternative considered:

- start one runtime per band

Rejected because it would introduce a new concurrency or ownership model that
does not match the current baseline and would distract from the operator-surface
problem this change is actually solving.

### 2. Extract a dedicated Python process manager from the probe-owned topology

The dual-surface topology logic will move out of the QoS probe into a dedicated
shared Python helper that can:

- allocate or accept explicit ports and roots
- start stock GDS surfaces through the maintained existing launcher
- start S-band and UHF gateway or COMM node processes
- print one reviewable manifest
- own coordinated cleanup for the maintained launchers and for the new proof

The maintained Bash entrypoints will stay thin wrappers over this helper.

Alternative considered:

- duplicate the topology logic across new Bash scripts

Rejected because the probe already proved the topology shape, and duplication
would make the maintained operator surface and its proof drift apart quickly.

### 3. Keep per-band launchers public and the combined wrapper composition-only

The public maintained surface will include:

- one S-band launcher
- one UHF launcher
- one combined wrapper

The combined wrapper will only start or stop owned processes and print the
combined manifest. It will not present itself as a policy owner, runtime
arbiter, or operator multiplexer.

Alternative considered:

- only expose the combined wrapper

Rejected because it would hide the per-band truth that this change is supposed
to make explicit and would make later orchestration layering less reviewable.

### 4. Make external runtime roots namespace roots, not destructively reused exact paths

When a launcher receives `PER_BAND_RUNTIME_ROOT` or `--runtime-root` outside its
owned `stack_root`, the helper will treat that path as a namespace root and own
a mode-scoped subdirectory beneath it. This keeps sequential or parallel
per-band entrypoints from deleting each other's live hosted runtime state while
preserving a reviewable, launcher-owned runtime root.

Alternative considered:

- keep deleting the exact external runtime root path on each launcher start

Rejected because that lets one launcher wipe another launcher's runtime state
and makes caller-supplied runtime roots destructively unsafe by default.

### 5. Treat existing per-band evidence as prerequisites, not as the new proof

The new hosted operator-baseline evidence will prove:

- both maintained stock stacks start successfully
- both surfaces remain distinct and reviewable
- the maintained launchers and combined wrapper own clean startup and cleanup

It will reuse adjacent governed evidence for:

- hosted S-band CCSDS path truth
- hosted UHF CCSDS path truth
- COMM role and session behavior
- hosted UHF beacon suppress
- hosted UHF primary packet quiet

Alternative considered:

- rerun one broad proof and present it as complete simultaneous-runtime closure

Rejected because that would over-claim orchestration and target-bearing
simultaneous behavior that this change explicitly defers.

### 6. Add a dedicated hosted operator runbook and one new registry path

Because this is becoming a maintained operator baseline rather than a probe
detail, the workflow needs:

- a dedicated hosted operator runbook under `docs/operator/`
- a dedicated verification-path registry entry that later changes can cite

Alternative considered:

- keep instructions only in `scripts/README.md` and evidence docs

Rejected because the user explicitly asked for maintained workflow and for a
clear handoff baseline for future orchestration work.

## Risks / Trade-offs

- **[Risk] Extracting topology code from the QoS probe could regress existing hosted proofs**
  - Mitigation: keep the process manager behavior aligned with the current
    probe topology, rerun the adjacent hosted proofs, and keep the proof-owned
    surfaces as explicit dependencies rather than rewriting their semantics.

- **[Risk] Operators may misread the combined wrapper as a new orchestration layer**
  - Mitigation: print explicit non-claims in launcher manifests and the runbook,
    and register the wrapper as composition-only in specs and evidence.

- **[Risk] Current docs may still imply that future orchestration is already the active queue truth**
  - Mitigation: update roadmap and architecture wording in the same change and
    explicitly reposition `comm-dual-link-orchestration-v1` as future work on
    top of this maintained baseline.

- **[Risk] The hosted-only maintained baseline could be mistaken for target/lab simultaneous closure**
  - Mitigation: keep the new evidence and registry entry hosted-only, cite
    target node-`6` and nonquiet records only as adjacent non-claims, and state
    the target-bearing simultaneous boundary explicitly in every current-facing
    doc that this change touches.

## Migration Plan

1. Create the formal OpenSpec artifacts and spec deltas.
2. Extract the shared hosted per-band stock-stack process manager from the QoS
   probe into a maintained helper.
3. Add the maintained S-band, UHF, and combined launcher wrappers.
4. Add a dedicated hosted operator proof wrapper that exercises the maintained
   launcher surface and records the combined manifest.
5. Update the hosted QoS probe to use the shared helper without changing its
   proof boundary.
6. Record hosted-first evidence and update the verification-path registry.
7. Sync current-facing docs and scripts inventory in the same branch.

Rollback is straightforward because the new surface is additive. If the helper
extraction regresses existing probes, the repo can revert the helper and
launcher changes without changing current COMM policy or target behavior.

## Open Questions

- None. This design intentionally keeps orchestration, one-GDS aggregation, and
  target-bearing simultaneous closure out of scope for a later governed change.
