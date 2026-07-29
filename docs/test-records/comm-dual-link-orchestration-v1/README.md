# comm-dual-link-orchestration-v1 Evidence

Date: 2026-05-29 (Asia/Taipei local date; 2026-05-28 UTC on GitHub).

OpenSpec change: `comm-dual-link-orchestration-v1`.

## Scope

This record proves a new hosted-only layer-2 thin lifecycle owner above the
maintained hosted per-band stock ground/operator baseline.

Path under proof:

```text
hosted dual-link orchestration owner
  -> delegated layer-1 maintained per-band stock stacks
    -> stock fprime-gds(CCSDS) + ground_ttc_gateway + sband_comm_csp_node(node 5) + shared hosted OBC/TopCcsds
    -> stock fprime-gds(CCSDS) + ground_ttc_gateway + uhf_comm_csp_node(node 6) + shared hosted OBC/TopCcsds
```

Newly proven in this change:

- a distinct hosted-only thin lifecycle owner exists above `43B`
- the owner writes orchestration-owned manifest and status artifacts
- the owner records a happy-path lifecycle:
  - `preflight -> starting -> ready -> stopping -> stopped`
- the owner records a bounded startup conflict as `startup_failed`
- the owner records cleanup listener summary without using older COMM semantic
  oracles
- a same-root rerun while the first owner is still active is rejected without
  deleting the live owner's artifacts
- a concurrent fresh same-root acquisition is rejected by an orchestration-owned
  startup-claim surface before the second launcher can wipe or overwrite owner
  artifacts
- a same-root rerun against a stale non-terminal owner status is rejected until
  the ambiguous owner state is cleaned up deliberately
- a same-root rerun against any stale terminal owner status that still records
  an open listener is rejected even when the post-stop owned-process inventory
  is empty
- the owner status includes delegated per-band stock GDS and CLI processes in
  the owned-process inventory used for same-root conflict decisions

This record does **not** prove:

- command authority ownership
- gateway relay ownership or one-gateway multiplexer behavior
- one stock GDS heterogeneous multi-upstream behavior
- COMM runtime ownership inside OBC
- target-bearing simultaneous dual-link proof
- RF closure
- any reopening of UHF packet-quiet, beacon-suppress, or formal file/downlink
  semantics

## Hosted Orchestration Owner Verdict

Repository-owned proof entrypoint:

```bash
bash scripts/run_dual_link_orchestration_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `dual-link-orchestration-hosted-owner` |
| logs | `/tmp/dual-link-orchestration-hosted.LlNrxH` |
| owner manifest | `/tmp/dual-link-orchestration-hosted.LlNrxH/happy-path/owner/manifest.json` |
| owner status | `/tmp/dual-link-orchestration-hosted.LlNrxH/happy-path/owner/status.json` |
| delegated layer-1 manifest | `/private/tmp/dual-link-orchestration-hosted.LlNrxH/happy-path/owner/layer1-baseline/manifest.json` |

Observed PASS markers:

```text
owner-happy-path=PASS manifest=/tmp/dual-link-orchestration-hosted.LlNrxH/happy-path/owner/manifest.json
owner-phase-sequence=PASS phases=preflight|starting|ready|stopping|stopped
owner-layer1-reference=PASS layer1Manifest=/private/tmp/dual-link-orchestration-hosted.LlNrxH/happy-path/owner/layer1-baseline/manifest.json
owner-startup-failure=PASS collidedPort=61356
owner-startup-failure-oracle=PASS phases=preflight|startup_failed
owner-active-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/active-owner-conflict/owner
owner-owned-process-inventory=PASS ground_gds=2 events=2 channels=2
owner-startup-claim-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/startup-claim-conflict/owner
owner-stale-nonterminal-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/stale-nonterminal-owner-conflict/owner
owner-stale-cleanup-failed-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/stale-cleanup-failed-owner-conflict/owner
owner-stale-startup-failed-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/stale-startup-failed-owner-conflict/owner
owner-stale-stopped-conflict=PASS ownerRoot=/tmp/dual-link-orchestration-hosted.LlNrxH/stale-stopped-owner-conflict/owner
dual-link-orchestration-hosted-probe: PASS
formal-verdict=dual-link-orchestration-hosted-owner
oracle=orchestration-owned manifest/status/cleanup only
reused-proof=43B maintained per-band baseline remains separate
```

What this passing proof means:

- the new layer-2 surface is not just a renamed layer-1 wrapper
- the owner lifecycle can be reviewed through owner artifacts alone
- the startup-failure case is classified by the owner's preflight surface
- a conflicting same-root rerun does not wipe the active owner's manifest or
  status surface
- a concurrent fresh same-root rerun now fails on the startup-claim oracle
  before it can mutate the shared owner root
- a stale non-terminal same-root rerun is rejected rather than being treated as
  implicitly clean
- any stale terminal same-root rerun is rejected when the owner’s cleanup
  oracle still records an open listener, even if `ownedProcessSet` has already
  been cleared by delegated teardown
- the same-root conflict oracle now sees delegated ground GDS and CLI children,
  not only the layer-1 helper's direct process list
- orchestration success does not depend on `run_comm_session_and_downlink_qos_probe.sh`
  or any COMM semantic event oracle

## Independent Layer-1 Non-Regression

The owner proof does not replace `43B`. This change reran the maintained
layer-1 baseline independently.

Command:

```bash
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `per-band-stock-ground-stacks-hosted-baseline` |
| logs | `/tmp/per-band-stock-ground-stacks-hosted.tNrnfL` |

Observed PASS markers:

```text
launcher-sband=PASS manifest=/tmp/per-band-stock-ground-stacks-hosted.tNrnfL/sband/stack/manifest.json
launcher-uhf=PASS manifest=/tmp/per-band-stock-ground-stacks-hosted.tNrnfL/uhf/stack/manifest.json
launcher-combined=PASS manifest=/tmp/per-band-stock-ground-stacks-hosted.tNrnfL/combined/stack/manifest.json
combined-distinct-surfaces=PASS sbandGds=127.0.0.1:57861 uhfGds=127.0.0.1:57863 runtime=/private/tmp/per-band-stock-ground-stacks-hosted.tNrnfL/combined/runtime/combined
per-band-stock-ground-stacks-hosted-probe: PASS
```

Current non-regression note:

- the rerun kept `43B` semantics intact
- the probe now rejects only newly created repo-root alias residue rather than
  failing on unrelated pre-existing `.adm-*` / `.stg-*` leftovers already
  present in the worktree before the run

## Verification

Focused verification completed in this change:

| Step | Command | Result |
|---|---|---|
| helper syntax | `python3 -m py_compile scripts/dual_link_orchestration.py scripts/per_band_stock_ground_stacks.py` | PASS |
| launcher/probe shell syntax | `bash -n scripts/run_hosted_dual_link_orchestration.sh scripts/run_dual_link_orchestration_hosted_probe.sh scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS |
| hosted orchestration proof | `bash scripts/run_dual_link_orchestration_hosted_probe.sh` | PASS |
| layer-1 non-regression proof | `bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS |
| full local verification gate | `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| change validation | `openspec validate comm-dual-link-orchestration-v1` | PASS |
| main spec validation | `openspec validate --specs` | PASS |

## Verdict

PASS for a hosted-only thin lifecycle owner above the maintained per-band stock
ground/operator baseline.

What is new:

- a real hosted layer-2 orchestration owner
- orchestration-owned lifecycle/failure/cleanup truth

What stays delegated:

- per-band stock-stack semantics and manifests
- gateway relay behavior
- stock GDS behavior
- COMM runtime semantics inside OBC

What remains deferred:

- target-bearing simultaneous proof
- one-GDS heterogeneous multi-upstream integration
- one-gateway multiplexer behavior
- RF closure
