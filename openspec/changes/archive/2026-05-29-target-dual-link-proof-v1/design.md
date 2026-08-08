## Context

The repository has already frozen the exact target-bearing dual-link boundary:

- default target node-`5` S-band command truth remains the governing primary
  surface
- non-quiet physical node-`6` under `uhf-backup` is concurrent adjunct only
- formal UHF command truth is judged only after explicit switch to
  `uhf-primary-after-failover`
- quiet node-`6` may rescue only the adjunct, not the formal switched UHF
  truth
- `target-claim` and `operator-observability` are separate verdicts

The missing work is implementation-bearing proof on the active physical target
CAN + UHF UART topology. Existing quiet node-`6` evidence and target TCP
comparator evidence are useful adjacent support, but they must not be restated
as the new target-bearing proof itself.

## Goals / Non-Goals

**Goals:**

- add one repository-owned target proof for the active physical target path
- close phase A, phase B, and phase C on one governed official run
- keep quiet rescue bounded to phase B adjunct-only use
- emit dual verdicts and exact outcome-branch wording for the official run
- update registry, evidence, and current docs so they state only what the
  official run actually proved

**Non-Goals:**

- no node-`5` loss / failover-loss closure
- no symmetric dual-authority command claim
- no official file/downlink requirement in the main PASS
- no target TCP carrier proof expansion
- no hosted orchestration redesign
- no packet-quiet or beacon-suppress redesign
- no gateway or GDS ownership redesign
- no RF claim

## Exact Path And Phase Model

### Physical target/lab topology

- `macOS` runs probe-owned `fprime-gds`, `fprime-cli`, and `ground_ttc_gateway`
- `subsystem.local` runs:
  - `sband_comm_csp_node` as COMM node `5` on the S-band TCP southbound
  - `uhf_comm_csp_node` as COMM node `6` on the physical UHF UART southbound
  - EPS node `2` and ADCS node `3` on the shared target CAN topology
- `obc.local` runs `obc-comm-csp-stack.service` on the active `TopCcsds`
  baseline over `can0`

### Proof family

| Phase | Surface | Required result |
|---|---|---|
| A | default node-`5` S-band primary | required target truth |
| B | non-quiet physical node-`6` `uhf-backup` adjunct | required adjunct, or quiet rescue |
| C | explicit switched `uhf-primary-after-failover` on physical node-`6` | required target truth |
| D | quiet node-`6` rescue for phase B only | optional rescue |

### Commands used

- phase A command: `GET_RESET_CAUSE`
- phase B command: `GET_RESET_CAUSE` under `uhf-backup`
- phase C command: `GET_RESET_CAUSE` after explicit switch to
  `uhf-primary-after-failover`

The richer `EPS_GET_STATUS + ADCS_GET_ATTITUDE` same-path sequence remains
optional adjacent evidence only. It is not part of the mandatory adjunct gate
for this first proof.

## Oracle And Verdict Model

### Oracle precedence

| Verdict | Oracle | Meaning |
|---|---|---|
| `target-claim` | target-side command truth with journal-first precedence | authoritative proof result |
| `operator-observability` | ground events, channels, gateway captures, and probe-owned ground artifacts | operator-facing reviewability result |

### Exact switch-closed boundary

The switch-closed event is the target-observed marker:

- `COMM_PRIMARY_LINK_CHANGED command UHF telemetry UHF file UHF reason 1`

The proof records the source of that marker, but target truth remains the
authoritative acceptance surface.

### PASS / FAIL model

| Result | Required conditions |
|---|---|
| `target-claim=PASS` | phase A passed, phase B passed or was rescued by phase D, and phase C passed on non-quiet switched UHF |
| `target-claim=FAIL` | phase A failed, or phase B failed without quiet rescue, or phase C failed |
| `operator-observability=PASS` | reviewable ground artifacts were present and the UHF ground view stayed clean enough for the declared minimum operator boundary |
| `operator-observability=DEGRADED` | `target-claim=PASS`, but the UHF ground view remained noisy, partial, or classifier-only |
| `operator-observability=FAIL` | claimed ground surfaces were absent or unusable |

Quiet rescue never substitutes for phase C. If phase B needs quiet rescue, the
probe must restore non-quiet service before running the mandatory switched UHF
truth in phase C.

## Implementation Shape

### Probe changes

Extend `run_target_can_matrix_probe.py` with one new mode,
`dual-link-proof`, that:

1. boots the target on the current default node-`5` baseline
2. enables the probe-owned diagnostics needed to review ground/operator
   evidence
3. runs phase A over S-band
4. starts the non-quiet UHF ground path and attempts phase B under
   `uhf-backup`
5. if phase B fails, applies quiet override, reruns the adjunct on quiet
   node-`6`, records rescue use, then restores normal non-quiet service
6. issues `COMM_SET_ACTIVE(UHF)` from the still-valid S-band session
7. closes phase C on non-quiet switched UHF
8. writes a dedicated JSON summary and human-readable stdout summary for the
   exact outcome branch

### Wrapper changes

Add a dedicated wrapper, `scripts/run_target_dual_link_proof.sh`, that:

- sets the same governed target service baseline used by the non-quiet target
  CAN proof stack
- runs the new probe mode under a fresh temp root
- keeps copied artifacts under that root
- prints the exact verdict summary at the end

### Artifact shape

The proof mode writes a dedicated summary JSON including at least:

- exact path under test
- exact phase sequence
- phase-by-phase source and status
- `target-claim`
- `operator-observability`
- quiet rescue used or not
- exact ground artifacts and journal artifacts
- degradation indicators when present
- exact non-claims preserved by this proof

## Documentation And Registry Shape

- `docs/interfaces.md` stops describing this slice only as frozen future work
  and instead names the exact implementation-bearing branch proven by the
  official run.
- `evidence/verification-path-registry.md` gains one new entry for this new
  target-bearing proof path, but the entry must describe only the branch that
  the official governed run actually proved.
- `target-nonquiet-background-tm-stability-v1` remains oracle rationale and
  degraded-observability context; it is not promoted wholesale into the new
  path.
- current docs keep all adjacent non-claims explicit, especially:
  - no symmetric dual-authority command closure
  - no one-GDS multi-upstream claim
  - no one-gateway multiplexing claim
  - no generic clean non-quiet operator surface claim

## Risks / Trade-offs

- **[Risk] Quiet rescue could be misread as replacing phase C.**
  Mitigation: record quiet rescue only under phase B, restore non-quiet
  service, and require a separate successful phase C.

- **[Risk] The adjunct gate could be made too heavy.**
  Mitigation: keep the mandatory adjunct gate to one minimal allowlisted
  read/status command.

- **[Risk] The evidence could over-claim branches that were not actually run.**
  Mitigation: register only the exact official successful branch.

- **[Risk] Ground-side noise could be misclassified as target failure.**
  Mitigation: keep `target-claim` authoritative and record degraded
  observability separately.

## Migration Plan

1. Create the change artifacts and delta specs.
2. Add the new probe mode and dedicated wrapper.
3. Add the evidence record and new verification-path registry entry.
4. Update current docs and main specs to reflect only the exact branch proven.
5. Run local checks, OpenSpec validation, and the governed target proof.

## Open Questions

- None. The remaining proof branch is intentionally limited to whatever the
  official governed run actually proves.
