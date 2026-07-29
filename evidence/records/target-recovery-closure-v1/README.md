# Test Record: target-recovery-closure-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-16

## Scope

This record governs the first closed `RecoveryExecutor` R2 process-restart
path on the active `TopCcsds` OBC package.

- Watchdog-source faults and ADCS scheduled-poll transport/freshness faults now
  persist recovery metadata and request `PROCESS_RESTART`.
- R2 process restart is distinct from R6 reboot-equivalent exit in action
  vocabulary, runtime status, counters, telemetry, and hosted exit code.
- `BootManager` persists R2 restart metadata before exit while preserving the
  boot metadata v1 schema.
- Hosted runtime returns exit code `31` for R2 process restart and keeps exit
  code `32` for R6 reboot-equivalent closure.
- The Raspberry Pi target proof uses the active COMM CSP systemd service model:
  OBC exits nonzero, the stack launcher returns that status, and systemd
  restarts `obc-comm-csp-stack.service` through `Restart=on-failure`.
- The target fault is induced by stopping the ADCS subsystem service; no
  probe-only public OBC command is introduced.
- Target command-path recovery is proven through the current secure-auth
  command-path family after the managed OBC service restart.

## Out Of Scope

- Raspberry Pi hardware watchdog stroking or watchdog-caused reset
- Linux reboot, bootloader or partition handoff, RF behavior, or final flight
  deployment behavior
- new legacy `Top` / `OBC_ComFprimeLegacy` proof

## Commands

Focused unit and runtime tests:

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
  cmake --build build-fprime-automatic-native-ut --target \
  OBC_Components_RecoveryExecutor_ut_exe \
  OBC_Components_BootManager_ut_exe \
  hosted_runtime_unit_test

./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RecoveryExecutor_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BootManager_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
```

Focused hosted probes:

```bash
bash scripts/run_recovery_executors_v1_probe.sh
bash scripts/run_multi_subsystem_fdir_v1_probe.sh
bash scripts/run_watchdog_v1_probe.sh
bash scripts/run_persistent_fault_ring_v1_probe.sh
```

Raspberry Pi package, install, and target proof:

```bash
bash scripts/bootstrap_rpi_workspace.sh
bash scripts/package_rpi_bundle.sh
FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh \
  build-artifacts/packages/rpi/v0.1.0-158-g20af4528-dirty/obc-rpi-v0.1.0-158-g20af4528-dirty.tar.gz

RUNTIME_ROOT=$OBC_HOME/obc-deploy/runtime/target-recovery-closure-v1-command3 \
  bash scripts/install_rpi_comm_csp_autostart.sh

RUNTIME_ROOT=$OBC_HOME/obc-deploy/runtime/target-recovery-closure-v1-command3 \
  bash scripts/run_rpi_target_recovery_restart_probe.sh
```

Repository gate and OpenSpec validation:

```bash
bash scripts/run_verification_ci.sh build-artifacts/target-recovery-closure-v1-local
openspec validate target-recovery-closure-v1
openspec validate --specs
```

## Evidence

Focused unit/runtime tests:

```text
OBC_Components_RecoveryExecutor_ut_exe: PASS
  RecoveryExecutor.BootSafeFallbackClampAlreadySafeQueuesSafeAction: PASS
  RecoveryExecutor.ProcessRestartPersistenceFailureFallsBackToSafe: PASS
OBC_Components_BootManager_ut_exe: PASS
hosted_runtime_unit_test: PASS
```

Hosted `RecoveryExecutor` probe:

```text
recovery-executors-v1 probe PASS log=/tmp/recovery-executors-v1.mSfSYS/recovery-executors-v1-probe.log
  watchdog_r2_process_restart_truth: firstFaultExit=31 resetCause=RECOVERY_WATCHDOG lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT bootCount=2 consecutive=1
  watchdog_reboot_truth_and_safe_fallback: cycles=3 rebootExit=32 finalBootCount=4 initialConsecutive=3 initialSafeFallback=yes
```

Hosted multi-subsystem FDIR probe:

```text
multi-subsystem-fdir-v1 probe PASS log=/tmp/multi-subsystem-fdir-v1.7Wztzt/multi-subsystem-fdir-v1-probe.log
  eps_shared_executor_relatch_reboot: firstFault=R5_SAFE recoveryClear=yes secondFaultReboot=-10 resetCause=RECOVERY_EPS_TIMEOUT bootCount=2 consecutive=1
  adcs_shared_executor_r2_process_restart: firstFaultExit=31 resetCause=RECOVERY_ADCS_FDIR bootCount=2 consecutive=1
  comm_failover_clear_then_reboot: firstFault=R3_FAILOVER recoveryClear=yes relatchR6Reboot=-10 resetCause=RECOVERY_COMM_FDIR bootCount=2 consecutive=1
```

Hosted watchdog probe:

```text
watchdog-v1-probe: PASS log=/tmp/watchdog-v1-hosted.i9OHYW/watchdog-v1-probe.log
  healthy_feed_eligible: mode=SAFE watchdog=HEALTHY feedEligible=yes
  warning_only_before_safe: aggregate=WARNING feedEligible=yes source=COMM_CONTROLLER
  stale_to_process_restart: stale->PROCESS_RESTART exit=31 resetCause=RECOVERY_WATCHDOG lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT
  warning_recovery_after_heartbeat_resume: warning->recovered aggregate=HEALTHY feedEligible=yes
  resource_monitor_rss_threshold: SYS_LOW_MEMORY observed after hosted rss threshold update
```

Hosted persistent fault ring probe:

```text
persistent-fault-ring-v1 probe PASS log=/tmp/persistent-fault-ring-v1.38F6RW/persistent-fault-ring-v1-probe.log
  comm_fdir_reboot_exit: code=32
  same_root_relaunch: generation=9 total=9 maxBootCountBefore=2 maxBootCountAfter=3
  dual_copy_fallback: corrupted=fault-ring-a.bin beforeActive=COPY_A afterActive=COPY_A generationStayed=9
```

Raspberry Pi package and install evidence:

```text
release_id=v0.1.0-158-g20af4528-dirty
tarball=build-artifacts/packages/rpi/v0.1.0-158-g20af4528-dirty/obc-rpi-v0.1.0-158-g20af4528-dirty.tar.gz
bin/OBC sha256=a34d87cea86eeb9acce90f73e717601448656e45edcd0c5984a773d57faa7640
dict/AppTopologyDictionary.json sha256=85f62b1ed9ed6f87f6b6caf228727f1196db26e01091308b5e0f0b1fa821c3b4
launch/run_obc_comm_csp_stack.sh sha256=5a2781d5b1afef1a9dbfe811478a2dbdee4da888200874ba1c45696cdf01878d
```

Raspberry Pi service-managed R2 restart probe:

```text
rpi target recovery restart probe PASS
obc-service=obc-comm-csp-stack.service target=operator@obc.local
adcs-service=subsystem-adcs-csp.service target=operator@subsystem.local
baseline-main-pid=221361 restarted-main-pid=221477
baseline-restarts=0 restarted-restarts=1
reset_cause=RECOVERY_ADCS_FDIR
last_recovery_source=ADCS_POLL_TRANSPORT
last_recovery_level=R2_RESTART_SOFTWARE_COMPONENT
command_path=secure-auth command path PASS summary=/tmp/rpi-target-recovery-restart.xL5wOf/post-r2-secure-auth/diagnostics/secure-auth-command-path-summary.json
boot_count=2
consecutive_reset_count=1
log=/tmp/rpi-target-recovery-restart.eY25MO/service-status.log
command-log=/tmp/rpi-target-recovery-restart.eY25MO/command-path-recovery.log
events-log=/tmp/rpi-target-recovery-restart.eY25MO/fprime-events-command-recovery.log
```

Target journal / secure-auth excerpts after restart:

```text
case-sband-apid-00fe-secure-auth=PASS
case-sband-secure-command-get-reset-cause-sequence=41
target-secure-auth-command-path=PASS
BOOT_RECOVERY_STATUS : reset RECOVERY_ADCS_FDIR (3) bootCount 2 consecutive 0 safeFallback 0 source ADCS_POLL_TRANSPORT (6) level R2_RESTART_SOFTWARE_COMPONENT (2)
```

Target service restoration after proof:

```text
RUNTIME_ROOT=$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc bash scripts/install_rpi_comm_csp_autostart.sh

reset_cause=4
boot_count=10
consecutive_reset_count=0
last_recovery_source=8
last_recovery_level=6
boot_safe_fallback_required=0
recovery_reset_pending=0
```

Fresh repository gate:

```text
build-artifacts/target-recovery-closure-v1-local/summary.md
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
openspec validate target-recovery-closure-v1: PASS
openspec validate --specs: PASS
```

## Notes

- `run_obc_comm_csp_stack.sh` now passes the default command authority profile
  and HMAC key material into the active OBC process. Without this target
  launcher fix, the restarted service could be observed, but authenticated
  command-path recovery would remain unproven on the service-managed target
  path.
- The target proof uses the active `TopCcsds` package and does not add or cite
  legacy `Top` evidence.
- The persistent fault ring probe was kept coherent by using the COMM FDIR R6
  reboot-equivalent cycle for persistence readback; watchdog and ADCS R2
  process-restart execution are covered by the dedicated target-recovery
  probes above.
