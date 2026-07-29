# Target Proof A/B/C Governance

Status: canonical ownership contract for target/lab probe and benchmark work.
Last reconciled during `uhf-primary-nonquiet-autofailover-v1` closeout on 2026-06-25.

Use this document when creating or modifying any Raspberry Pi or subsystem-backed
proof, benchmark, or probe in this repository.

## Canonical Order

For every independent testcase or repetition:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/<probe-or-benchmark>.sh
```

Think of these as:

- `A`: target baseline manager
- `B`: ground baseline manager
- `C`: probe-owned functional test

Within one continuous semantic scenario, do not rerun `A/B` mid-flow. Example:
S-band secure-auth -> governed node-`5` unavailable window -> UHF re-auth is
one continuous `C` scenario.

## A: Target Baseline Manager

Repository entrypoint:

- [scripts/ensure_target_comm_lab_baseline.sh](../../scripts/ensure_target_comm_lab_baseline.sh)

`A` owns readiness and bounded repair for the shared target baseline on
`obc.local` and `subsystem.local`.

`A` may:

- verify required shared services are active exactly once
- start or restart governed shared services when readiness is broken
- perform a bounded transport reset when ordinary restart repair still leaves
  the maintained OBC S-band availability marker broken
- remove stale proof-owned systemd drop-ins
- reap duplicate runtime processes that conflict with governed services
- verify governed CAN settings and target command-path markers
- install and verify the maintained scoped COMM CAN FD profile on the OBC,
  S-band node `5`, and UHF node `6` services

`A` is the canonical ready-state contract for shared target services such as:

- `obc-lab-can.service`
- `obc-comm-csp-stack.service`
- `subsystem-eps-adcs-lab-can.service`
- `subsystem-comm-lab-can.service`
- `subsystem-eps-csp.service`
- `subsystem-adcs-csp.service`
- `subsystem-sband-csp.service`
- `subsystem-uhf-csp.service`

That bounded transport reset remains part of `A`, not `C`. A probe must not
recreate this stop/restart sequence as probe-owned cleanup or functional flow.

Current policy is that `A` always requires UHF readiness. Functional probes may
stay on the S-band command/data path, but they do not opt out of the shared
node-`6` readiness baseline.

The maintained node-`5` V3 profile is also an A-layer responsibility. Its
effective service environment is:

- `COMM_CSP_SOCKETCAN_USE_CANFD=1`
- `COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=5,6`
- `COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=40`

This profile applies only to OBC/S-band/UHF COMM services. EPS/ADCS remain
outside it. A functional `C` may verify these values, but must not install,
restart, or remove the shared `57-csp-socketcan-canfd.conf` profile.

## B: Ground Baseline Manager

Repository entrypoint:

- [scripts/ensure_ground_dual_gds_baseline.sh](../../scripts/ensure_ground_dual_gds_baseline.sh)

`B` owns readiness and cleanup for the local macOS dual-GDS ground baseline.

`B` may:

- detect and reap proof-owned local residue such as:
  - `fprime-gds`
  - `ground_ttc_gateway`
  - `fprime-cli events`
  - `fprime-cli channels`
  - secure-auth helper residue
- verify no stale repo-owned listeners remain
- verify the machine is clean before a new probe-owned ground runtime starts

`B` does **not** mean “keep a standing GDS stack running for later probes”.
Its job is to return the machine to a clean, ready-to-start state.

## C: Probe-Owned Functional Test

`C` owns only the functional scenario under proof plus probe-owned evidence
capture.

`C` may:

- start probe-owned local GDS, gateway, and passive observer helpers
- create probe-owned temp runtime roots
- apply temporary testcase overlays or diagnostics
- collect target journals, service snapshots, native CLI logs, and captures

`C` must not:

- restart `obc-comm-csp-stack.service` as ordinary testcase flow
- restart subsystem S-band/UHF/EPS/ADCS services as ordinary testcase flow
- restart `*-lab-can.service`
- shut down shared target baseline services during cleanup
- mix environment repair into capability proof

Cleanup for `C` must remove only:

- probe-owned local helpers/listeners
- probe-owned temp runtime roots
- probe-owned temporary overlays and diagnostics

## Preferred References

When building a new target/lab probe, prefer these current references before
looking at older wrappers:

- [scripts/run_target_secure_auth_command_path_probe.sh](../../scripts/run_target_secure_auth_command_path_probe.sh)
- [scripts/run_target_secure_auth_proof.sh](../../scripts/run_target_secure_auth_proof.sh)
- [scripts/run_target_uhf_primary_nonquiet_runtime_probe.sh](../../scripts/run_target_uhf_primary_nonquiet_runtime_probe.sh)
- [scripts/run_target_autonomous_uhf_failover_probe.sh](../../scripts/run_target_autonomous_uhf_failover_probe.sh)
- [docs/test-records/uhf-primary-nonquiet-runtime-v1/README.md](../test-records/uhf-primary-nonquiet-runtime-v1/README.md)
- [docs/test-records/target-autonomous-uhf-failover-v1/README.md](../test-records/target-autonomous-uhf-failover-v1/README.md)

Choose references by explicit authority, not by proximity:

- start from the exact registry entry or canonical evidence record for the
  path under proof
- then follow only the wrapper explicitly named by this document, the
  target/lab runbook, or that governing evidence record
- do not browse `docs/test-records/` by “nearest” topic match
- do not browse `scripts/` by filename similarity and assume the closest probe
  is current

Use older target probes only when they are the exact registered path being
maintained. Do not copy their embedded bootstrap or cleanup patterns forward
without rechecking ownership against this document.

## Matrix Library Boundary

`scripts/comm_verification/lib/run_target_can_matrix_probe.py` remains a
shared implementation library and helper surface for several current proofs.
That shared code reuse does **not** make the older matrix proof family the
current authority for:

- secure-auth readiness
- A/B/C ownership
- current non-quiet target UHF truth
- current autonomous failover semantics

Current secure-auth and governed target/lab proofs must take their authority
from the current target/lab runbook, the verification registry, and the exact
governing evidence record for the path under proof.

## Anti-Patterns

Do not:

- treat a legacy probe with embedded shared-service restart logic as a template
- let a testcase own both baseline repair and functional proof
- reuse “environment cleanup” code that stops shared target services
- infer that one successful proof run makes a dirty environment acceptable
- use the old `SESSION_OPEN` comparator as the current secure-auth readiness gate
