## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, and `tasks.md` for
  `target-node5-telemetry-backpressure-and-restart-stability-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-node5-telemetry-backpressure-and-restart-stability-v1`.

## 2. Telemetry Backpressure Closure

- [x] 2.1 Inspect the active node-`5` downlink path and existing target truth
  surfaces to classify `ComCcsds.comQueue.QueueOverflow` on telemetry queue
  index `1`.
- [x] 2.2 Add only the minimum repository-owned instrumentation or bounded
  observability required to explain telemetry queue saturation and drain
  behavior under the declared representative workload.
- [x] 2.3 Implement the minimum product-side fix needed to remove sustained
  telemetry backpressure or overflow under that workload, or record that no
  additional queue/backpressure fix is required once a fresh rebuilt/install
  baseline is revalidated.
- [x] 2.4 Add or refresh a focused blocker-diagnostic proof for the
  representative node-`5` workload and capture reviewable evidence.

## 3. Restart Stability Closure

- [x] 3.1 Reproduce and classify fresh restart-path transient node-`5`
  availability loss and early `RateGroupCycleSlip`.
- [x] 3.2 Add only the minimum repository-owned diagnostics or instrumentation
  needed to correlate restart timing, node-`5` availability, and rate-group
  slip.
- [x] 3.3 Implement the minimum product-side fix needed to keep the fresh
  restart path within the active baseline contract, or record that the active
  baseline already satisfies the contract after the bounded branch-local fix.
- [x] 3.4 Add or refresh a focused restart-stability proof and capture
  reviewable evidence.

## 4. Timing-Probe Confirmation

- [x] 4.1 Re-run the existing
  `scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh` workflow
  after the blocker fixes and confirm the new product blockers no longer
  dominate the timing path.
- [x] 4.2 If the timing rerun still fails, classify only the remaining narrow
  residual blocker honestly rather than broadening scope; otherwise record the
  clean blocker-removal verdict.

## 5. Documentation And Evidence

- [x] 5.1 Add
  `evidence/records/target-node5-telemetry-backpressure-and-restart-stability-v1/README.md`
  with blocker baseline, commands, observations, product fixes, and final
  verdicts.
- [x] 5.2 Update `evidence/verification-path-registry.md` for any new or narrowed
  blocker-diagnostic path created by this change.
- [x] 5.3 Update `docs/roadmap/next-work.md` so target timing follow-up work
  reflects these prerequisite product blockers explicitly.
- [x] 5.4 Update current architecture or interface docs only if active baseline
  truth or residual-blocker wording truly changes.

## 6. Verification

- [x] 6.1 Run fresh local build and focused verification for all touched code.
- [x] 6.2 Repackage and reinstall the Raspberry Pi target bundle for the active
  service-managed node-`5` baseline.
- [x] 6.3 Run the mandatory target control preflight:
  `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`.
- [x] 6.4 Run the blocker-focused target proofs and capture fresh evidence.
- [x] 6.5 Run `openspec validate target-node5-telemetry-backpressure-and-restart-stability-v1`
  and `openspec validate --specs`.
