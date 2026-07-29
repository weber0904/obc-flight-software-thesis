## 1. Phase 1: Fresh Inventory And Drift Diagnosis

- [x] 1.1 Capture the current node-`5` residual inventory from the current dictionary, fresh hosted logs, and current owner/component code paths.
- [x] 1.2 Record for each residual surface: current emitted exposure, governance bucket, and implementation action (`docs-only reclassification`, `proof/oracle repair`, `runtime behavior repair`, or explicit same-change deferral).
- [x] 1.3 Diagnose the representative detailed `GET_*` packetized proof path and decide whether the current drift is in the proof oracle, the packetized delivery path, or the component-owned summary/detail split.
- [x] 1.4 Keep the current fresh local gate and the failing hosted observability rerun as diagnostic inputs only until final closeout reruns are complete.

## 2. Phase 2: OpenSpec And Canonical-Doc Alignment

- [x] 2.1 Rewrite `proposal.md`, `design.md`, and `tasks.md` so this change no longer assumes immediate closeout and instead reflects the four same-change phases.
- [x] 2.2 Update the delta specs for `comm-subsystem`, `core-system-contracts`, `interface-contract-index`, `onboard-data-products-and-live-beacon`, `verification-path-registry`, and `verification-evidence` to match the rebuilt same-change plan.
- [x] 2.3 Update `docs/interfaces.md` so the node-`5` residual inventory includes current observed exposure and implementation action in addition to owner/component, final bucket, replacement surface, and rationale.
- [x] 2.4 Update `docs/architecture/current-development-architecture.md`, `docs/roadmap/current-baseline.md`, `docs/roadmap/next-work.md`, and the affected operator runbooks so current branch truth distinguishes keep-live truth, reviewable observability, bounded fresh readback, diagnostics-only residuals, and active detailed `GET_*` requalification work.
- [x] 2.5 Update `evidence/verification-path-registry.md` and `evidence/records/node5-observability-residual-cleanup-v1/README.md` so they describe an active same-change investigation plus later proof completion rather than a pending immediate closeout rerun.
- [x] 2.6 Keep `openspec/specs/comm-subsystem/spec.md`, `openspec/specs/core-system-contracts/spec.md`, `openspec/specs/interface-contract-index/spec.md`, `openspec/specs/onboard-data-products-and-live-beacon/spec.md`, `openspec/specs/verification-path-registry/spec.md`, and `openspec/specs/verification-evidence/spec.md` aligned with the rebuilt branch contract.

## 3. Phase 3: Runtime / Probe / Proof-Chain Implementation

- [x] 3.1 Repair the hosted node-`5` observability proof first if the current summary/detail split still exists in component code and the drift is in the passive-oracle/capture chain.
- [x] 3.2 Repair the target node-`5` observability proof with the same wrapper identity and registry path, adding deterministic bounded detailed `GET_*` observation rather than creating a parallel proof family.
- [x] 3.3 Only change `EpsBridge`, `GpsBridge`, `AdcsBridge`, `RadioController`, or `StorageHealthBridge` if diagnosis proves that the component-owned detailed publication itself drifted from the intended summary/detail split.
- [x] 3.4 Only change `CommController` or `CommEgressMux` if diagnosis proves that the explicit authenticated detailed readback is being suppressed or misrouted by current live-observability policy.
- [x] 3.5 Keep `SystemResources.*`, queue-depth counters, `UART_*`, and other diagnostics-only residuals as docs-only reclassification unless the implementation diagnosis shows that a runtime removal is explicitly required by the updated design.

## 4. Phase 4: Final Verification And Closeout

- [x] 4.1 Rerun the fresh local verification gate after phase 3 is complete.
- [x] 4.2 Rerun the aligned hosted and target node-`5` observability proofs plus the secure-auth control regression.
- [x] 4.3 Rerun touched UTs, including at least `CommController`, `CommEgressMux`, and `WatchdogSupervisor` when those surfaces are changed, plus any touched family bridge or hosted proof support tests.
- [x] 4.4 Run `openspec validate node5-observability-residual-cleanup-v1` and `openspec validate --specs`.
- [x] 4.5 Archive `node5-observability-residual-cleanup-v1` only after phases 1-4 agree.
- [x] 4.6 Update `openspec/reconciliation/baseline-reconciliation-matrix.json`, regenerate `openspec/reconciliation/baseline-reconciliation-matrix.md`, and run repo consistency checks only after archive.
- [x] 4.7 Record final commands and verdicts here and leave the branch at local-ready with no push or PR.

### Phase 4 Status

- `4.1` PASS:
  `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- `4.2` PASS:
  - hosted observability PASS:
    `bash scripts/run_sband_observability_governance_hosted_probe.sh`
  - target secure-auth command-path preflight PASS:
    `bash scripts/run_target_secure_auth_command_path_probe.sh`
  - target observability PASS:
    `bash scripts/run_target_sband_observability_governance_probe.sh`
  - target secure-auth regression PASS:
    `bash scripts/run_target_secure_auth_proof.sh`
    fresh proof root:
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.uiUAYj/target-secure-auth-proof-v1`
    closure:
    - force the full proof to restart OBC ground-link diagnostics by default so S-band auth does not inherit stale APID sequence state
    - let the target secure-auth probe consume handshake messages from native packet logs when wire captures miss the first challenge/auth-status window, especially on the UHF path
    - restart the UHF primary-reauth helper on a fresh runtime root so the secondary GDS does not fail on the reused `custom-data-handlers-app` path
- `4.3` PASS:
  `PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut --output-on-failure -R "OBC_Components_CommController_ut_exe|OBC_Components_CommEgressMux_ut_exe|OBC_Components_WatchdogSupervisor_ut_exe|OBC_Components_GroundLinkHealthProvider_ut_exe|hosted_runtime_unit_test"`
- `4.4` PASS:
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate node5-observability-residual-cleanup-v1`
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`
- `4.5` PASS:
  - attempted archive without `--skip-specs` correctly refused to reapply already-synced spec deltas
  - final archive command:
    `PATH="$PWD/fprime-venv/bin:$PATH" openspec archive node5-observability-residual-cleanup-v1 --yes --skip-specs`
  - archived change:
    `2026-06-13-node5-observability-residual-cleanup-v1`
- `4.6` PASS:
  - matrix source updated:
    `openspec/reconciliation/baseline-reconciliation-matrix.json`
  - markdown regenerated:
    `python3 scripts/generate_reconciliation_matrix_md.py`
  - consistency checks:
    `python3 scripts/check_repo_consistency.py`
- `4.7` PASS:
  - final post-archive spec validation:
    `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`
  - branch status:
    archived and local-ready, with no push and no PR opened
