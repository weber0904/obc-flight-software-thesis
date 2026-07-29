## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and any minimal formal
  delta required for a valid reviewable change.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-timing-empirical-ceiling-freeze-v1`.

## 2. Timing Closure Probe Workflow

- [x] 2.1 Add the top-level
  `scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh` workflow
  with mandatory `control-partition preflight` and `timing measurement`
  stages.
- [x] 2.2 Add the bounded per-run helper
  `scripts/target_timing_empirical_ceiling_freeze_v1_probe.py`.
- [x] 2.3 Snapshot installed service truth before measurement and record it in
  the per-run result set.
- [x] 2.4 Align the timing probe readiness, session-open, and retry contract
  with the proven node-`5` control probe; keep `COMMAND_ENVELOPE_OBSERVED` as
  diagnostics-only.
- [x] 2.5 Replace the old fixed-duration window with the declared count-gated
  measurement contract and blocker classification.
- [x] 2.6 Aggregate three fresh timing runs, including one explicit
  service-restart rerun, into a single empirical ceiling result.
- [x] 2.7 Keep the slow-group oracle tied to repo-owned rateGroup2 truth; if an
  existing target channel cannot prove `5s` cadence inside the declared
  contract, add only the minimum periodic telemetry support needed for the
  timing probe.

## 3. Documentation And Evidence

- [x] 3.1 Add `docs/test-records/target-timing-empirical-ceiling-freeze-v1/README.md`
  with control preflight verdict, installed service snapshot, per-run roots,
  per-window timing summaries, and aggregated ceilings or blocker class.
- [x] 3.2 Update `docs/verification-path-registry.md` entry `68` so it reflects
  either numeric empirical timing closure or a narrower reproducible blocker.
- [x] 3.3 Update `docs/interfaces.md` target-flightlike timing section so
  empirical service-managed ceilings move to `verified empirical` or remain as
  an explicitly narrowed residual blocker.
- [x] 3.4 Update current-baseline / next-work / architecture docs only where
  the active numeric timing truth or the remaining blocker wording actually
  changes.

## 4. Verification

- [x] 4.1 Run fresh local verification/build work for the maintained native
  targets touched by this change, including `OBC`, `ground_ttc_gateway`, and
  probe sanity checks.
- [x] 4.2 Repackage and reinstall the Raspberry Pi target bundle for the active
  service-managed node-`5` baseline.
- [x] 4.3 Run the mandatory target control preflight:
  `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`.
- [x] 4.4 Run the new timing closure workflow and record either frozen empirical
  ceilings or a narrowed blocker classification honestly.
- [x] 4.5 Run `openspec validate target-timing-empirical-ceiling-freeze-v1` and
  `openspec validate --specs`.
