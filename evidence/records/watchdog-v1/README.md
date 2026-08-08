# Test Record: watchdog-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-12

## Scope

This record covers the first active-baseline software-watchdog and runtime-liveness supervision slice on hosted `TopCcsds`.

- `WatchdogSupervisor` is the single runtime owner for watchdog-v1.
- `HealthMonitor` no longer remains as a separate active-baseline owner; CPU/RSS threshold monitoring is migrated under `WatchdogSupervisor`.
- Explicit heartbeats supervise the bounded source set:
  - `EpsBridge`
  - `EpsFdirController`
  - `ModeSafetyController`
  - `CommController`
- Freshness uses deterministic tick-based warning / fault / suppress thresholds.
- First threshold crossing records warning only.
- Fault threshold crossing latches the watchdog fault; `SAFE_REQUESTED` is only reported after a real watchdog `SAFE` request, and `SAFE` / `HELL` crossings defer that one request until a later `IDLE`, `PAYLOAD`, or `TTC` mode is observed while the fault remains latched.
- Continued stale progression suppresses watchdog feed eligibility at the supervisor boundary.
- First restored beat clears the stale source immediately; aggregate recovery waits until no enabled supervised source remains faulted or suppressed.
- Recovery does not auto-exit `SAFE`.

## Out Of Scope

- Raspberry Pi hardware watchdog stroking or watchdog-caused reset
- boot-safe-image recovery or reset-cause persistence
- process restart executors or subsystem reset executors
- broad multi-subsystem FDIR or a broader system-supervisor framework
- persistent fault/event storage
- RF behavior or target-hardware closure

## Commands

Focused tests:

```bash
fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut \
  --output-on-failure \
  -R 'OBC_Components_WatchdogSupervisor_ut_exe|watchdog_policy_unit_test|OBC_Components_ModeSafetyController_ut_exe|mode_safety_policy_integration_test|OBC_Components_EpsFdirController_ut_exe|eps_fdir_policy_unit_test|eps_fdir_integration_test|hosted_runtime_unit_test'
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-watchdog-v1
```

Focused repository-owned probe:

```bash
PROBE_TMP_DIR=$PWD/build-artifacts/watchdog-v1-probe \
PROBE_LOG=$PWD/build-artifacts/watchdog-v1-probe/watchdog-v1-probe.log \
  bash scripts/run_watchdog_v1_probe.sh
```

OpenSpec validation:

```bash
openspec validate watchdog-v1
openspec validate --specs
```

## Evidence

Focused CTest:

```text
OBC_Components_WatchdogSupervisor_ut_exe: PASS
watchdog_policy_unit_test: PASS
OBC_Components_ModeSafetyController_ut_exe: PASS
mode_safety_policy_integration_test: PASS
OBC_Components_EpsFdirController_ut_exe: PASS
eps_fdir_policy_unit_test: PASS
eps_fdir_integration_test: PASS
hosted_runtime_unit_test: PASS
100% tests passed, 0 tests failed out of 8
```

Fresh repository gate:

```text
build-artifacts/verification-ci-watchdog-v1/summary.md
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

Hosted watchdog probe:

```text
watchdog-v1-probe: PASS log=$REPO_ROOT/build-artifacts/watchdog-v1-probe/watchdog-v1-probe.log
  healthy_feed_eligible: mode=SAFE watchdog=HEALTHY feedEligible=yes
  warning_only_before_safe: aggregate=WARNING feedEligible=yes source=COMM_CONTROLLER
  stale_to_safe_and_feed_suppressed: stale->SAFE->suppressed finalMode=SAFE aggregate=FEED_SUPPRESSED feedEligible=no
  recovery_after_heartbeat_resume: stale->SAFE->suppressed->recovered finalMode=SAFE aggregate=HEALTHY feedEligible=yes
  resource_monitor_rss_threshold: SYS_LOW_MEMORY observed after hosted rss threshold update
```

OpenSpec:

```text
openspec validate watchdog-v1: PASS
openspec validate --specs: PASS
```

## Notes

- The hosted proof is intentionally bounded to supervisor-side watchdog behavior on the active `TopCcsds` baseline.
- The probe uses repository-owned beat suppression only to create stale-heartbeat conditions for hosted proof; it is not a flight public contract.
- `WatchdogSupervisor` reuses the normal mode-control path, so outward mode results remain the usual `SYS_MODE_CHANGE` surface rather than a parallel watchdog-owned mode store.
- The fresh hosted probe proves healthy, warning-only, latched, feed-suppressed, recovered, and migrated resource-monitoring behavior without claiming target hardware reset or boot recovery.
