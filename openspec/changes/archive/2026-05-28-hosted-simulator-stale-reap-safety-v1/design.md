## Context

`per-band-stock-ground-stacks-v1` intentionally kept the new maintained
launcher layer thin by reusing the existing managed-process cleanup helpers.
That reuse left one mismatch between the generic helper and the per-band
launcher's ownership model: EPS and ADCS simulator command lines are shared
across isolated hosted runs, while the real uniqueness for those runs lives in
the CSP hub environment rather than in the process command line.

As a result, the pre-launch or post-stop stale reap could match another active
simulator-backed hosted run that was still healthy and intentionally alive.

## Goals / Non-Goals

**Goals**

- Prevent the maintained hosted per-band launchers from terminating unrelated
  active EPS/ADCS simulator runs that use different CSP hub ports.
- Keep stale cleanup available for real orphan leftovers so rerun safety does
  not regress.
- Add a reviewable hosted proof and evidence record for that non-interference
  boundary.
- Sweep the current-facing documentation surfaces that still lag the merged
  per-band baseline or the current target-timing truth.

**Non-Goals**

- Do not redesign the overall per-band launcher topology.
- Do not claim system-wide orphan-process prevention for every hosted helper.
- Do not widen the maintained per-band operator-baseline claim into
  orchestration, one-GDS aggregation, one-gateway multiplexing, or
  target-bearing simultaneous proof.

## Decisions

### Extend managed-process stale reap with an orphan-only mode

`probe_process_utils.py` already has a `require_orphan` capability at the raw
reap helper layer. The missing piece is preserving that choice through the
managed-process lifecycle. The fix is to carry a `stale_require_orphan` flag on
`ManagedProcess` and thread it through:

- pre-launch stale cleanup
- post-stop cleanup
- bulk managed-process cleanup retry loops

This keeps the fix narrow and reusable without rewriting the launcher into a
separate process manager.

### Apply orphan-only cleanup only where command lines are not uniquely owned

Most maintained per-band launcher helpers already use unique ports, serial
devices, runtime roots, or other command-line fragments that make ownership
reviewable. EPS and ADCS simulators are the problematic exception because the
shared `--node-id` fragments are not unique to one hosted run.

This follow-up therefore applies orphan-only stale cleanup to the maintained
per-band EPS/ADCS simulator processes, while preserving stricter stale cleanup
for launcher-owned helpers whose command lines already encode ownership.

### Prove non-interference with a live alternate hosted simulator stack

The repository-owned hosted proof should exercise the real review comment
scenario rather than just inspect the code. The maintained per-band hosted
probe will:

- start an alternate hosted CSP hub plus EPS/ADCS simulator pair on different
  ports
- run the maintained S-band, UHF, and combined launchers
- assert that the unrelated active EPS/ADCS simulator pair is still alive
  after each launcher exits

That keeps the claim honest and bounded to the reviewed hosted cleanup
boundary.

### Sync only the current-facing docs that actually drifted

The broader per-band stock-stack truth is already present in the main current
docs, so this follow-up only updates the surfaces that still drift:

- the hosted per-band runbook and current-baseline/current-architecture wording
  for launcher cleanup ownership
- `README.md` target-timing non-claim wording, which still overstates the gap
- `docs/architecture/target-flight-design.md`, which still trails the merged
  2026-05-28 COMM baseline and queue state
- adjacent current-facing references (`comm-followup-directions`,
  `interfaces.md`, `scripts/README.md`, and the registry entry) where a short
  clarification keeps the merged baseline reviewable

## Risks / Trade-offs

- **Orphan-only cleanup could leave a still-parented stale simulator behind**
  Acceptable for this narrow fix. The reviewed bug is active-run interference,
  and the maintained proof still requires rerun safety against orphaned
  leftovers.
- **The evidence scope may look broader than it is**
  Mitigate by keeping explicit non-claims: the new proof is only about hosted
  launcher cleanup ownership and non-interference with unrelated active
  simulator runs.
