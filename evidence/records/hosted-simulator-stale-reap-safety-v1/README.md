# hosted-simulator-stale-reap-safety-v1 Evidence

Date: 2026-05-28.

OpenSpec change: `hosted-simulator-stale-reap-safety-v1`.

## Scope

This record hardens one already-merged current path: the maintained hosted
per-band stock ground/operator baseline. It does not introduce a new operator
topology. Instead it proves a bounded cleanup-safety rule for that existing
baseline.

Path under proof:

```text
alternate hosted CSP hub + eps_simulator(node 2) + adcs_simulator(node 3) stay active
while a maintained per-band stock-stack launcher starts and stops its own hosted stack
```

Newly proven in this change:

- the maintained per-band launcher process manager can mark selected stale
  matchers as orphan-only
- the per-band launchers now apply orphan-only stale cleanup to shared EPS and
  ADCS simulator identities whose command line does not uniquely encode
  launcher ownership
- rerunning the maintained S-band, UHF, and combined launchers does not
  terminate another active hosted simulator-backed run that is using different
  CSP hub ports

This record does **not** prove:

- broader system-wide orphan-process prevention
- orchestration ownership
- simultaneous dual-link runtime arbitration
- one-GDS heterogeneous multi-upstream behavior
- one-gateway multiplexer behavior
- target-bearing simultaneous proof
- RF closure

## Current Path Relationship

This is follow-up evidence for the already-registered maintained hosted
per-band stock ground/operator baseline.

It reuses, but does not replace:

- [evidence/records/per-band-stock-ground-stacks-v1/README.md](../per-band-stock-ground-stacks-v1/README.md)

It strengthens only the hosted cleanup-ownership boundary for that same path.

## Hosted Proof Verdict

Repository-owned proof entrypoint:

```bash
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `per-band-stock-ground-stacks-hosted-baseline` |
| artifact root | `/tmp/per-band-stock-ground-stacks-hosted.0e7UHP` |
| relevant artifacts | `summary.log`, `alternate-simulator-stack/{csp-zmqproxy.log,eps.log,adcs.log}`, `<mode>/stack/manifest.json`, `<mode>/launcher.log` |

Observed new PASS markers:

```text
unrelated-active-simulator-stack-sband=PASS
unrelated-active-simulator-stack-uhf=PASS
unrelated-active-simulator-stack-combined=PASS
bounded-noninterference=unrelated active EPS/ADCS simulator stack survived all launcher reruns
```

What this passing proof means:

- a live unrelated hosted CSP hub plus EPS/ADCS simulator pair can stay active
  while the maintained S-band launcher runs and exits
- the same remains true for the maintained UHF launcher
- the same remains true for the combined maintained wrapper
- orphan-leftover cleanup still remains available for the shared simulator
  identities; this evidence only narrows ownership so active unrelated runs are
  not reaped
- the maintained launcher path no longer leaves repo-root `.adm-*`,
  `.stg-*`, `.sequence-admitted`, or `.sequence-staging` alias residue after
  the proof exits

## Current-Facing Documentation Sync

This follow-up also refreshed the current-facing docs that still lagged the
merged per-band baseline:

- `README.md`: narrowed the target-timing non-claim so it matches the current
  node-`5` empirical timing freeze
- `docs/architecture.md`: pulled the 2026-05-28 COMM
  baseline forward, including the packet-quiet/beacon-suppress split and the
  maintained per-band operator baseline
- current operator/current-baseline/current-architecture references now state
  that shared EPS/ADCS simulator stale cleanup is orphan-only and non-
  interfering with unrelated active hosted runs

## Verification

Focused local verification used for this follow-up:

| Step | Command | Result |
|---|---|---|
| helper syntax | `python3 -m py_compile scripts/per_band_stock_ground_stacks.py scripts/probe_process_utils.py` | PASS |
| launcher/probe shell syntax | `bash -n scripts/run_hosted_sband_stock_ground_stack.sh scripts/run_hosted_uhf_stock_ground_stack.sh scripts/run_hosted_per_band_stock_ground_stacks.sh scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS |
| fresh local closeout gate | `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| hosted maintained baseline probe | `bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS; artifact root `/tmp/per-band-stock-ground-stacks-hosted.0e7UHP` |
| documentation governance | `python3 scripts/check_documentation_governance.py` | PASS |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| change validation | `openspec validate hosted-simulator-stale-reap-safety-v1` | PASS |
| main spec validation | `openspec validate --specs` | PASS |

## Verdict

PASS for the bounded hosted cleanup-safety follow-up on the maintained
per-band stock ground/operator baseline.

The active repo truth is now:

- two stock GDS processes
- two gateway processes
- separate southbound paths
- one shared hosted `TopCcsds` runtime
- one combined wrapper that coordinates start/stop only
- orphan-only stale cleanup for the shared EPS/ADCS simulator identities when
  command-line ownership is not otherwise unique

What remains deferred is unchanged:

- higher-level `comm-dual-link-orchestration-v1`
- simultaneous dual-link runtime arbitration
- one-GDS multi-upstream behavior
- one-gateway multiplexer behavior
- target-bearing simultaneous closure
