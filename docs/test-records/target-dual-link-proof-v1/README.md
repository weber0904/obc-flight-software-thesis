# target-dual-link-proof-v1 Evidence

Current-note:

- this record remains valid only for the exact historical explicit-switch
  target dual-link branch it originally proved
- `COMM_SET_ACTIVE(UHF)` here is historical branch truth, not current
  autonomous failover truth
- post-switch `uhf-primary-after-failover` packet-quiet wording in this record
  is also historical compatibility context, not the maintained 2026-06-24
  non-quiet baseline
- do not reuse this wrapper as a template for current Chapter 5 or current
  target/lab failover closure; use the archived 2026-06-24 Chapter 5 and
  UHF non-quiet/autonomous-failover records instead

Date:
- `2026-05-29`

OpenSpec change:
- `target-dual-link-proof-v1`

## Scope

This record proves the repository's first implementation-bearing
target-bearing dual-link family on the active physical target CAN plus UHF UART
lab topology.

Path under test:

- macOS `fprime-gds`
- macOS `ground_ttc_gateway` raw relay
- `subsystem.local` S-band node `5`, UHF node `6`, EPS, and ADCS
- `obc.local` OBC over shared SocketCAN
- physical node-`6` UHF serial southbound on `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`

Exact proof family:

- phase A: default target node-`5` `sband-primary` `GET_RESET_CAUSE`
- phase B: concurrent non-quiet target node-`6` `uhf-backup`
  `GET_RESET_CAUSE`
- phase C: `sband-primary COMM_SET_ACTIVE(UHF)` with authoritative switch
  marker `COMM_PRIMARY_LINK_CHANGED command UHF telemetry UHF file UHF reason 1`
- phase C: non-quiet target node-`6` `uhf-primary-after-failover`
  `GET_RESET_CAUSE`

This record does **not** claim:

- simultaneous full-authority commands on both links
- one stock `fprime-gds` heterogeneous multi-upstream closure
- one `ground_ttc_gateway` S-band/UHF multiplexer behavior
- RF or over-the-air closure
- UHF reliable-transfer redesign
- packet-quiet redesign
- beacon-suppress redesign
- mandatory official file/downlink continuity in the main PASS boundary
- generic non-quiet node-`6` operator observability beyond this exact official
  branch and its minimum operator boundary

## Implemented Harness

Repository-owned harness changes in this change:

- [scripts/run_target_dual_link_proof.sh]($REPO_ROOT/scripts/run_target_dual_link_proof.sh)
- [scripts/comm_verification/lib/run_target_can_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_can_matrix_probe.py)

What the branch-local proof wrapper now owns:

- bounded phase ordering for the exact target dual-link family
- target-journal-first oracle precedence for phases A, B, and C
- phase-B-only quiet rescue guardrails
- dual-verdict summary artifacts for `target-claim` and
  `operator-observability`
- explicit cleanup and owned-artifact recording under one fresh runtime root

No product runtime semantics were intentionally widened in this change.

## Official Governed Run

Command:

```bash
bash scripts/run_target_dual_link_proof.sh
```

Official artifact root:

- `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof`

Primary artifacts:

| Field | Value |
|---|---|
| summary artifact | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/diagnostics/dual-link-proof-summary.json` |
| checkpoint log | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/diagnostics/checkpoints.jsonl` |
| OBC journal snapshot | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/diagnostics/journal-snapshots/dual-link-proof-obc.log` |
| UHF journal snapshot | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/diagnostics/journal-snapshots/dual-link-proof-uhf.log` |
| UHF outbound capture | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/captures/gds-to-southbound.bin` |
| UHF inbound capture | `/private/tmp/target-dual-link-proof.fOzKnc/target-dual-link-proof/captures/southbound-to-gds.bin` |

Official result branch:

- `target-claim=PASS`
- `operator-observability=PASS`
- `quiet-rescue=false`

Observed governed markers:

```text
phase-a-sband-session-source=target-journal
phase-a-command-source=target-journal
phase-b-session-source=target-journal
phase-b-command-source=target-journal
phase-c-sband-session-source=target-journal
switch-source=target-journal
phase-c-uhf-session-source=target-journal
phase-c-command-source=target-journal
target-claim=PASS
operator-observability=PASS
```

## Oracle Precedence

| Phase | Command / boundary | Authoritative PASS oracle | Ground/operator role |
|---|---|---|---|
| A | node-`5` `sband-primary GET_RESET_CAUSE` | target journal `BOOT_RECOVERY_STATUS` completion | corroborating review surface only |
| B | node-`6` non-quiet `uhf-backup GET_RESET_CAUSE` | target journal `BOOT_RECOVERY_STATUS` completion | corroborating review surface only |
| C switch | `COMM_SET_ACTIVE(UHF)` | target journal `COMM_PRIMARY_LINK_CHANGED command UHF telemetry UHF file UHF reason 1` | corroborating review surface only |
| C truth | node-`6` non-quiet `uhf-primary-after-failover GET_RESET_CAUSE` | target journal `BOOT_RECOVERY_STATUS` completion | corroborating review surface only |

## PASS / FAIL Table

| Verdict line | Official result | Meaning |
|---|---|---|
| `target-claim` | `PASS` | default node-`5` primary truth, non-quiet `uhf-backup` adjunct, explicit switched `uhf-primary-after-failover` truth all closed on the governed target oracle |
| `operator-observability` | `PASS` | reviewable S-band and UHF artifacts existed, phase B completion was corroborated on the still-primary S-band surface, and phase C post-switch UHF transport evidence stayed clean enough for the declared minimum operator boundary |

Bounded failure model preserved by this proof:

- if phase A fails, `target-claim=FAIL`
- if phase B non-quiet adjunct fails and quiet rescue also fails, `target-claim=FAIL`
- if phase C switched non-quiet truth fails, `target-claim=FAIL`
- if ground/operator artifacts are absent or unusable, `operator-observability=FAIL`

## Evidence / Non-Claim Table

| Topic | Official record |
|---|---|
| exact target path under test | default node-`5` S-band primary plus physical non-quiet node-`6` UHF backup/switched primary over shared target CAN |
| exact switch sequence under test | phase A node-`5` truth -> phase B non-quiet `uhf-backup` adjunct -> `COMM_SET_ACTIVE(UHF)` -> phase C non-quiet `uhf-primary-after-failover` truth |
| exact oracle used | target-journal-first command truth with a separate ground/operator review verdict |
| newly proven | first implementation-bearing target dual-link family for this exact primary-led, switch-closed path |
| quiet rescue used | no |
| noisy non-quiet limitation still remains | not on this exact official branch; the minimum operator boundary passed even though post-switch UHF command completion remained target-journal authoritative |
| what remains non-claim | no symmetric dual-authority closure, no one-GDS multi-upstream closure, no one-gateway multiplexer closure, no RF closure, no generic non-quiet operator claim beyond this exact branch |

## Why `operator-observability` Is PASS

The official summary artifact recorded:

- `phaseBSbandCompletionVisible = true`
- `phaseBUhfTransportVisible = true`
- `phaseCPostSwitchUhfTransportVisible = true`
- `groundBootRecoveryStatusVisible = false`
- non-empty UHF captures:
  - outbound `340` bytes
  - inbound `46080` bytes
- zero UHF checksum warnings
- zero UHF link churn

This means the target truth is still authoritative and repository-owned, while
the operator review surface now passes for this exact branch because the probe
classifies each phase against the frozen runtime semantics:

- phase B completion is corroborated on S-band because node-`5` remains the
  live telemetry primary during `uhf-backup`
- phase C post-switch UHF does not require `BOOT_RECOVERY_STATUS` on UHF
  `events.log` because `uhf-primary-after-failover` packet quiet suppresses
  live `event/tlm` packet egress on the formal UHF path
- post-switch UHF still had reviewable gateway, raw-command, and capture
  evidence with no fresh checksum or link-churn defects

## Mapping Back To Clarification Boundary

This proof lands the exact frozen family from
`target-dual-link-claim-oracle-clarification-v1` without widening it:

- default node-`5` remained the governing primary truth
- phase B used one minimal allowlisted low-authority command
- quiet rescue stayed unused and therefore unproven on this official branch
- formal UHF command truth closed only after the explicit switched
  `uhf-primary-after-failover` marker
- official file/downlink continuity stayed outside the main PASS

## Verification

Focused verification completed for the branch-local implementation:

| Step | Command | Result |
|---|---|---|
| fresh local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| Python syntax | `PATH="$PWD/fprime-venv/bin:$PATH" python -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py` | PASS |
| wrapper shell syntax | `bash -n scripts/run_target_dual_link_proof.sh` | PASS |
| governed target proof | `bash scripts/run_target_dual_link_proof.sh` | PASS for `target-claim` and `operator-observability` |
| node-`5` cleanup preflight | `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh` | PASS at `/tmp/rpi-target-recovery-restart.Jh0Ksu` |

## Verdict

The repository now has its first implementation-bearing target dual-link proof,
but only for this exact bounded family:

- default node-`5` S-band primary target truth
- non-quiet node-`6` `uhf-backup` adjunct
- explicit switched non-quiet node-`6` `uhf-primary-after-failover` target truth

The authoritative PASS boundary was target-journal-first command truth. The
official lab conditions were the active target CAN plus physical node-`6` UHF
UART topology. The official branch also closed the declared minimum
ground/operator review boundary without promoting this exact `PASS/PASS`
result into a generic non-quiet node-`6` closure claim.
