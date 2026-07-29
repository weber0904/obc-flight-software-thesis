# Test Record: recovery-executors-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-13

## Scope

This record covers the first shared recovery-executor closure on the active hosted `TopCcsds` baseline.

- `RecoveryExecutor` is the single runtime owner for shared recovery incident normalization, recovery-level progression, and bounded recovery action ownership.
- `WatchdogSupervisor` remains the bounded software-watchdog detector and now emits shared watchdog recovery requests and clears instead of owning `SAFE` escalation directly.
- `EpsFdirController` remains the EPS-timeout detector and now emits shared EPS recovery requests and clears instead of owning `SAFE` escalation directly.
- `BootManager` and `BootMetadataStore` now truthfully persist recovery-caused `reset_cause`, `boot_count`, `consecutive_reset_count`, `last_recovery_source`, `last_recovery_level`, and the boot-after-recovery `SAFE` fallback clamp.
- Bounded v1 action set:
  - watchdog first-fault `R2` process-restart intent as a reviewable intent only
  - EPS timeout `R3` interface reset through `EpsBridge.resetForRuntime()`
  - shared `R5` mode fallback through the normal `ModeManager` path
  - shared hosted `R6` reboot-equivalent exit after metadata persistence
- The hosted proof uses same-runtime-root relaunch to verify reboot truth rather than claiming target hardware reboot.

## Out Of Scope

- broad all-subsystem FDIR or a generic fault manager
- target hardware watchdog stroking, Raspberry Pi reboot proof, or power-loss recovery
- generic operator `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET`
- persistent event-log infrastructure
- TTC pass scheduling, payload behavior, RF behavior, or target-hardware closure

## Commands

Focused tests:

```bash
fprime-venv/bin/ctest --test-dir build-fprime-automatic-native --output-on-failure \
  -R 'OBC_Components_(WatchdogSupervisor|EpsFdirController|BootManager|RecoveryExecutor)_ut_exe|hosted_runtime_unit_test|eps_fdir_integration_test|boot_metadata_store_unit_test|command_authority_catalog_check'
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-recovery-executors-v1
```

Focused repository-owned probe:

```bash
bash scripts/run_recovery_executors_v1_probe.sh
```

OpenSpec validation and archive:

```bash
openspec validate recovery-executors-v1
openspec validate --specs
openspec archive recovery-executors-v1 --yes
```

## Evidence

Focused CTest:

```text
OBC_Components_WatchdogSupervisor_ut_exe: PASS
OBC_Components_EpsFdirController_ut_exe: PASS
OBC_Components_RecoveryExecutor_ut_exe: PASS
command_authority_catalog_check: PASS
OBC_Components_BootManager_ut_exe: PASS
eps_fdir_integration_test: PASS
boot_metadata_store_unit_test: PASS
hosted_runtime_unit_test: PASS
100% tests passed, 0 tests failed out of 8
```

Hosted recovery-executor probe:

```text
recovery-executors-v1 probe PASS log=/tmp/recovery-executors-v1.qOH25l/recovery-executors-v1-probe.log
  watchdog_reboot_truth_and_safe_fallback: cycles=3 rebootExit=32 finalBootCount=4 initialConsecutive=3 initialSafeFallback=yes stableAckCleared=yes
  eps_shared_executor_relatch_reboot: firstFault=R5_SAFE recoveryClear=yes secondFaultReboot=32 resetCause=RECOVERY_EPS_TIMEOUT bootCount=2 consecutive=1
```

Fresh repository gate:

```text
build-artifacts/verification-ci-recovery-executors-v1/summary.md
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
```

OpenSpec:

```text
openspec validate recovery-executors-v1: PASS
openspec validate --specs: PASS
openspec archive recovery-executors-v1 --yes: PASS, archived as 2026-05-12-recovery-executors-v1
```

Adjacent regression check:

```text
boot-trust-chain probe PASS
runtime-root=/tmp/boot-trust-chain-v1-probe
```

## Notes

- The new hosted proof is intentionally about the shared recovery-executor closure, not about proving every pre-existing watchdog-v1 or EPS-timeout-v1 detector detail again from scratch.
- The archived `watchdog-v1` hosted probe still reflects the older detector-owned `SAFE` escalation contract and is no longer the authoritative active-path proof after this change; `run_recovery_executors_v1_probe.sh` is the new shared-path evidence boundary.
- The watchdog path proves visible progression from `R2` restart intent to `R5` `SAFE` fallback to `R6` reboot-equivalent closure when stale suppression persists.
- The EPS path proves that the first timeout latch enters the same executor path, executes the bounded `R3` EPS reset plus `R5` `SAFE` fallback, and then escalates to `R6` reboot when the incident relatches.
- Same-root relaunch is required evidence because boot truth is part of this contract; isolated one-shot runtime logs alone are not sufficient to claim `reset_cause`, `boot_count`, `consecutive_reset_count`, or boot-safe-fallback closure.
- `RecoveryExecutor` keeps mode fallback on the normal internal mode path and keeps boot truth in `BootManager`; this change does not create a parallel mode store or a parallel boot metadata owner.
