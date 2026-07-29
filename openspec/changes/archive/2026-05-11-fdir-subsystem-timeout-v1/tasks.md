## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `fdir-subsystem-timeout-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate fdir-subsystem-timeout-v1`.

## 2. Runtime Contracts And Topology

- [x] 2.1 Add an EPS runtime-health provider contract and health snapshot type for cache validity, last poll result, consecutive failures, and cumulative poll errors.
- [x] 2.2 Extend `EpsBridge` to maintain and publish deterministic poll-health state while preserving current cache invalidation and EPS comm-error behavior.
- [x] 2.3 Add the new passive `EpsFdirController` component with focused threshold/latch/escalation/recovery behavior.
- [x] 2.4 Add a distinct internal mode-apply source for subsystem-fault fallback.
- [x] 2.5 Wire `EpsFdirController` into both `TopCcsds` and legacy `Top` so schedule order is `epsBridge -> epsFdirController -> modeSafetyController`.

## 3. Focused Tests

- [x] 3.1 Expand `OBC_Components_EpsBridge_ut_exe` for consecutive-failure accounting, cumulative comm-error accounting, and reset-on-success behavior.
- [x] 3.2 Add `OBC_Components_EpsFdirController_ut_exe` classic component coverage for retrying, first-fault escalation, no duplicate escalation, recovery clear, and `SAFE`/`HELL` fault-only behavior.
- [x] 3.3 Add `eps_fdir_policy_unit_test` for the pure threshold/latch/recovery state machine.
- [x] 3.4 Add `eps_fdir_integration_test` proving real `EpsBridge` health output drives real `EpsFdirController` mode-fallback behavior through the normal mode-control surface.
- [x] 3.5 Rerun and keep green the affected regressions: `OBC_Components_ModeSafetyController_ut_exe`, `mode_safety_policy_integration_test`, and `eps_csp_integration_test`.

## 4. Hosted Probe And Evidence

- [x] 4.1 Add `scripts/run_eps_timeout_fdir_hosted_probe.sh` using hosted `TopCcsds`, isolated ports/runtime roots, and simulator stop/restart to create real EPS timeout windows.
- [x] 4.2 Cover a transient two-failure case with no `SAFE` fallback and healthy recovery after simulator restart.
- [x] 4.3 Cover a threshold-crossing case where the third consecutive failure latches fault and triggers exactly one `SAFE` fallback.
- [x] 4.4 Cover a recovery-after-fault case where simulator restart clears the latched fault and emits recovery evidence.
- [x] 4.5 Record evidence under `evidence/records/fdir-subsystem-timeout-v1/README.md` and add a distinct verification-path registry entry for hosted EPS-timeout FDIR.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification build/gate with `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1`.
- [x] 5.2 Run focused `ctest`, `bash scripts/run_eps_csp_integration.sh`, and `bash scripts/run_eps_timeout_fdir_hosted_probe.sh` after the fresh build.
- [x] 5.3 Run `openspec validate fdir-subsystem-timeout-v1` and `openspec validate --specs`.
- [x] 5.4 Archive the change with `openspec archive fdir-subsystem-timeout-v1 --yes`.
- [x] 5.5 Update canonical roadmap, verification, and architecture narrative documents for the new active baseline truth.
