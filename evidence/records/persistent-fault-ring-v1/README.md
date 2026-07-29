# Test Record: persistent-fault-ring-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-16

## Scope

This record governs the first hosted persistent fault ring closure on the
active `TopCcsds` baseline.

- `PersistentFaultManager` is the single public owner for persistent recovery
  breadcrumb readback.
- `PersistentFaultStore` owns the dual-copy whole-file snapshot format under
  `persistent-data/recovery/fault-ring-{a,b}.bin`.
- `BootManager` writes boot-observed and recovery-boot-ack breadcrumbs without
  changing existing boot metadata truth ownership.
- `RecoveryExecutor` writes shared recovery lifecycle breadcrumbs for incident
  open, action request, action execution, reboot pending, reboot issued, and
  incident clear without adding detector-local duplicate writers.
- Hosted readback is exposed through the dictionary-visible
  `GET_PERSISTENT_FAULT_HISTORY(limit)` command and the hosted shell
  `fault history [count]`.
- The governed hosted probe uses same-runtime-root relaunch after a COMM FDIR
  reboot-equivalent cycle plus deliberate corruption of the newer copy to prove
  dual-copy fallback and post-reboot readability.

## Out Of Scope

- target power-loss robustness or target-hardware reboot persistence
- `.fdp`, `OnboardState`, `HkTrendRecord`, or beacon summary export
- detector-local duplicate writers in `WatchdogSupervisor`, `EpsFdirController`,
  `AdcsFdirController`, or `CommController`
- persistent anti-replay state, boot trust hardening, RF behavior, or Raspberry
  Pi deployment closure

## Commands

Focused tests:

```bash
fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut --output-on-failure \
  -R 'OBC_Components_(RecoveryExecutor|CommandIngressAuthority|BootManager|PersistentFaultManager)_ut_exe|persistent_fault_store_unit_test|hosted_runtime_unit_test'
```

Focused repository-owned probe:

```bash
bash scripts/run_persistent_fault_ring_v1_probe.sh
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-persistent-fault-ring-v1
```

OpenSpec validation:

```bash
openspec validate persistent-fault-ring-v1
openspec validate --specs
```

## Evidence

Focused CTest:

```text
OBC_Components_RecoveryExecutor_ut_exe: PASS
OBC_Components_CommandIngressAuthority_ut_exe: PASS
OBC_Components_BootManager_ut_exe: PASS
OBC_Components_PersistentFaultManager_ut_exe: PASS
persistent_fault_store_unit_test: PASS
hosted_runtime_unit_test: PASS
100% tests passed, 0 tests failed out of 6
```

Hosted persistent fault ring probe:

```text
persistent-fault-ring-v1 probe PASS log=/tmp/persistent-fault-ring-v1.38F6RW/persistent-fault-ring-v1-probe.log
  comm_fdir_reboot_exit: code=32
  same_root_relaunch: generation=9 total=9 maxBootCountBefore=2 maxBootCountAfter=3
  dual_copy_fallback: corrupted=fault-ring-a.bin beforeActive=COPY_A afterActive=COPY_A generationStayed=9
```

Fresh repository gate:

```text
build-artifacts/verification-ci-persistent-fault-ring-v1/summary.md
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
openspec validate persistent-fault-ring-v1: PASS
openspec validate --specs: PASS
```

## Notes

- The hosted probe intentionally uses the shared COMM FDIR reboot-equivalent
  path for the persistent-ring cycle so watchdog R2 process-restart behavior is
  covered by `target-recovery-closure-v1` instead of reopening this evidence
  around old intent-only assumptions.
- The reboot-equivalent hosted proof observes the R6 runtime exit code `32`;
  R2 process-restart exit code `31` is covered separately by
  `target-recovery-closure-v1`.
- Same-root relaunch is required evidence because persistent fault history is a
  post-reboot claim; one-shot pre-reboot logs alone do not prove recovery
  breadcrumbs survive relaunch.
- Dual-copy fallback is demonstrated by holding record count and generation flat
  across the extra relaunch while `maxBootCount` still increases from `2` to
  `3`, showing the corrupted newer copy was discarded and the older valid copy
  remained usable.
