# Test Record: mode-safety-policy-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-07
- Archived change: `openspec/changes/archive/2026-05-06-mode-safety-policy-v1/`

## Scope

This record covers the SoC-driven mode safety policy v1 slice:

- New active `ModeSafetyController` owns cached-EPS SoC fallback for `SAFE`, `HELL`, `IDLE`, `PAYLOAD`, and `TTC`.
- Strict fallback thresholds are `SAFE -> HELL` at SoC `< 10%`, `HELL -> SAFE` at SoC `> 15%`, and `IDLE|PAYLOAD|TTC -> SAFE` at SoC `< 40%`.
- `SAFE -> IDLE` remains manual; high SoC does not auto-recover from `SAFE`.
- Failed EPS status polls invalidate cached status for safety-policy use until the next successful status update, so stale SoC is not treated as authoritative.
- `ModeSafetyController` does not evaluate or publish decision telemetry when runtime mode-control or EPS-status providers are not configured.
- EPS critical-battery telemetry/alarm remains visible but no longer aborts the hosted runtime before the mode safety policy can request `SAFE -> HELL`.
- The provisional `MissionExecutive` remains retired from active topology for this policy.

## Out Of Scope

- `LOW_POWER`, sun-safe pointing, ADCS detumble, EPS load shedding, broader FDIR, watchdog, subsystem timeout/retry/reset, COMM split-link, CCSDS, reliable transfer, payload scheduling, TTC pass automation, TLE handling, RF, target hardware, and target-hardware claims.

## Commands

Formal change validation:

```bash
openspec validate mode-safety-policy-v1
openspec validate --specs
```

Fresh focused tests:

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
  ctest --test-dir build-fprime-automatic-native-ut -R "ModeSafety|mode_safety|EpsBridge" --output-on-failure
```

Fresh hosted probe:

```bash
bash scripts/run_mode_safety_policy_hosted_probe.sh
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-safety-policy-v1
```

## Evidence

Focused CTest:

```text
OBC_Components_ModeSafetyController_ut_exe: PASS
mode_safety_policy_unit_test: PASS
mode_safety_policy_integration_test: PASS
OBC_Components_EpsBridge_ut_exe: PASS
100% tests passed, 0 tests failed out of 4
```

Review follow-up coverage added after PR review:

```text
OBC_Components_ModeSafetyController_ut_exe includes UnconfiguredRuntimeDoesNotEvaluate.
OBC_Components_EpsBridge_ut_exe includes TimeoutInvalidatesCachedStatusAndPreservesLastTelemetry.
fprime-util build --ut: PASS
fprime-util build: PASS
openspec validate --specs: PASS, 23 passed, 0 failed
```

Hosted focused probe:

```text
mode_safety_policy_hosted_probe: PASS log=/tmp/mode-safety-policy-hosted.E0DN0p/mode-safety-policy-hosted-probe.log
  safe_to_hell: initial_soc=9.00 final=HELL shutdown=terminated log=/tmp/mode-safety-policy-hosted.E0DN0p/safe_to_hell.log
  hell_to_safe: initial_soc=16.00 final=SAFE shutdown=graceful log=/tmp/mode-safety-policy-hosted.E0DN0p/hell_to_safe.log
  idle_to_safe: initial_soc=39.00 final=SAFE shutdown=graceful log=/tmp/mode-safety-policy-hosted.E0DN0p/idle_to_safe.log
  payload_to_safe: initial_soc=39.00 final=SAFE shutdown=terminated log=/tmp/mode-safety-policy-hosted.E0DN0p/payload_to_safe.log
  ttc_to_safe: initial_soc=39.00 final=SAFE shutdown=terminated log=/tmp/mode-safety-policy-hosted.E0DN0p/ttc_to_safe.log
  safe_high_soc_remains_safe: initial_soc=60.00 final=SAFE shutdown=terminated log=/tmp/mode-safety-policy-hosted.E0DN0p/safe_high_soc_remains_safe.log
```

Repository verification CI:

```text
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-safety-policy-v1:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-safety-policy-v1-post-archive:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-safety-policy-v1-review-fix:
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
openspec validate mode-safety-policy-v1: PASS
openspec validate --specs: PASS, 23 passed, 0 failed
openspec archive mode-safety-policy-v1 --yes: PASS, archived as 2026-05-06-mode-safety-policy-v1
post-archive openspec validate --specs: PASS, 23 passed, 0 failed
post-archive python3 scripts/check_repo_consistency.py: PASS, 76 archived changes checked
post-archive python3 scripts/check_component_test_baseline.py: PASS, 19 real components and 7 helper/support modules checked
```

## Notes

- The hosted probe starts `csp_zmqproxy`, `eps_simulator --initial-soc <pct>`, `radio_mock_server`, and the native hosted `OBC` runtime with isolated runtime roots and alternate local ports.
- The hosted cases use simulator-owned EPS state, then the OBC observes cached EPS status through `EpsBridge`; no scenario truth is fed directly into `ModeSafetyController`.
- The hosted probe verdict is scoped to final mode outcomes. If interactive OBC shutdown does not finish promptly after a case has already produced its final `mode=` line, the probe records `shutdown=terminated` and performs cleanup instead of treating shutdown latency as mode-policy evidence.
- `ModeManager` remains the authoritative mode surface. Focused integration coverage verifies fallback requests through `ModeManager::setModeForRuntime`, and hosted logs show normal mode status outcomes.
- The EPS critical-battery event is warning-high for this slice so `ModeSafetyController` can own primary-mode fallback instead of a fatal handler terminating the hosted runtime.
- `openspec archive` emitted a task warning because closeout task checkboxes were still open before archive execution; the archive and spec sync completed successfully, and the archived task record was updated after the command.
