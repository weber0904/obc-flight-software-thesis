# Test Record: multi-subsystem-fdir-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-13

## Scope

This record covers the first bounded multi-subsystem shared-recovery closure on the active hosted `TopCcsds` baseline.

- Bounded subsystem set is exactly `EPS`, `ADCS`, and `COMM`.
- `EpsFdirController`, `AdcsFdirController`, and `CommController` remain detector-local owners.
- `RecoveryExecutor` is the single runtime owner for shared incident normalization, recovery-level progression, bounded subsystem action ownership, shared `SAFE` fallback ownership, and reboot-intent persistence.
- `BootManager` / `BootMetadataStore` remain the single owner for truthful recovery-caused boot metadata.
- `WatchdogSupervisor` now supervises `ADCS_FDIR` in addition to the earlier bounded set.
- The hosted proof uses same-runtime-root relaunch to verify reboot truth after reboot-equivalent escalation.

## Out Of Scope

- generic all-subsystem FDIR or a persistent fault manager
- GPS, payload, TTC scheduler, storage-health, or other future subsystem recovery lines
- RF behavior, target-hardware reboot proof, or hardware reset proof
- truthful process restart execution; `R2` remains intent-only
- persistent recovery configuration or target deployment closure

## Commands

Fresh local build:

```bash
fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target \
  hosted_runtime_unit_test recovery_runtime_unit_test OBC_Components_RecoveryExecutor_ut_exe OBC OBC_ComFprimeLegacy
fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target OBC_Components_CommController_ut_exe
fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target OBC OBC_ComFprimeLegacy
```

Focused unit and integration tests:

```bash
fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut --output-on-failure \
  -R 'hosted_runtime_unit_test|recovery_runtime_unit_test|OBC_Components_(AdcsBridge|AdcsFdirController|CommController|RecoveryExecutor|WatchdogSupervisor|BootManager|EpsFdirController)_ut_exe|adcs_fdir_policy_unit_test|comm_fdir_policy_unit_test|eps_fdir_integration_test'
```

Fresh local verification gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-multi-subsystem-fdir-v1
```

Focused hosted probes:

```bash
bash scripts/run_recovery_executors_v1_probe.sh
bash scripts/run_multi_subsystem_fdir_v1_probe.sh
```

OpenSpec validation:

```bash
openspec validate multi-subsystem-fdir-v1
openspec validate --specs
```

## Evidence

Focused CTest:

```text
OBC_Components_WatchdogSupervisor_ut_exe: PASS
OBC_Components_EpsFdirController_ut_exe: PASS
eps_fdir_integration_test: PASS
OBC_Components_AdcsFdirController_ut_exe: PASS
adcs_fdir_policy_unit_test: PASS
OBC_Components_RecoveryExecutor_ut_exe: PASS
recovery_runtime_unit_test: PASS
OBC_Components_AdcsBridge_ut_exe: PASS
OBC_Components_CommController_ut_exe: PASS
comm_fdir_policy_unit_test: PASS
OBC_Components_BootManager_ut_exe: PASS
hosted_runtime_unit_test: PASS
100% tests passed, 0 tests failed out of 12
```

Fresh verification gate:

```text
build-artifacts/verification-ci-multi-subsystem-fdir-v1/summary.md
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

Hosted regression probe:

```text
recovery-executors-v1 probe PASS log=/tmp/recovery-executors-v1.dvw8ug/recovery-executors-v1-probe.log
  watchdog_reboot_truth_and_safe_fallback: cycles=3 rebootExit=32 finalBootCount=4 initialConsecutive=3 initialSafeFallback=yes stableAckCleared=yes
  eps_shared_executor_relatch_reboot: firstFault=R5_SAFE recoveryClear=yes secondFaultReboot=32 resetCause=RECOVERY_EPS_TIMEOUT bootCount=2 consecutive=1
```

Hosted multi-subsystem probe:

```text
multi-subsystem-fdir-v1 probe PASS log=/tmp/multi-subsystem-fdir-v1.AOo6kT/multi-subsystem-fdir-v1-probe.log
  eps_shared_executor_relatch_reboot: firstFault=R5_SAFE recoveryClear=yes secondFaultReboot=-4 resetCause=RECOVERY_EPS_TIMEOUT bootCount=2 consecutive=1
  adcs_shared_executor_relatch_reboot: firstFault=R5_SAFE recoveryClear=yes secondFaultReboot=-4 resetCause=RECOVERY_ADCS_FDIR bootCount=2 consecutive=1
  comm_failover_clear_then_reboot: firstFault=R3_FAILOVER recoveryClear=yes relatchR6Reboot=-4 resetCause=RECOVERY_COMM_FDIR bootCount=2 consecutive=1
```

OpenSpec:

```text
openspec validate multi-subsystem-fdir-v1: PASS
openspec validate --specs: PASS
```

## Notes

- `EPS` remains on the existing detector contract: retries `1-2` stay local, failure `3` enters the shared executor, first latch performs bounded `R3` EPS reset plus `R5 SAFE`, and relatch escalates to `R6`.
- `ADCS` only consumes scheduled-poll health from `AdcsBridge`; command-path transport failures remain local events and do not advance ADCS FDIR counters.
- `COMM` detector ownership remains inside `CommController`; detector-side failover was removed, and shared COMM actuation now happens only through executor-owned `performRecoveryLinkFailoverForRuntime()`.
- `COMM` proof demonstrates deterministic first-fault failover/clear and relatch-to-`R6` progression after the incident clears and reopens.
- The hosted multi-subsystem probe observed reboot-equivalent terminations as process exit `-4` after `R6` intent persistence on the interactive hosted runtime. This record treats the authoritative proof as the persisted recovery metadata observed on same-root relaunch, not as a claim about a portable hosted exit code contract.
