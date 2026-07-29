# Test Record: fdir-subsystem-timeout-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-11

## Scope

This record covers the narrow EPS-only subsystem-timeout FDIR vertical slice on the active hosted `TopCcsds` baseline:

- `EpsBridge` owns scheduled EPS status polling, cache validity, comm-error visibility, and deterministic poll-health state.
- `EpsFdirController` owns bounded retry, fault latch/clear, and one-shot SAFE escalation for repeated EPS poll failures.
- Failures `1-2` remain retry-only; failure `3` latches fault and escalates once.
- `IDLE`, `PAYLOAD`, and `TTC` request `SAFE` through the existing `ModeManager` internal mode path with the dedicated source `FdirSubsystemFault`.
- `SAFE` and `HELL` record the EPS fault without issuing duplicate mode requests.
- The first successful recovery poll clears the latched EPS fault and emits explicit recovery evidence.
- `ModeSafetyController` remains SoC-only, `HealthMonitor` remains CPU/RSS-only, and `MissionExecutive` remains inactive.

## Out Of Scope

- Broad all-subsystem FDIR, shared policy engines, watchdog/process heartbeat supervision, persistent event storage, EPS reset/power-cycle/load-shedding, command auth/session/QoS, TTC pass scheduling, payload control, RF, and target-hardware claims.

## Commands

Focused tests:

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
  ctest --test-dir build-fprime-automatic-native-ut \
  -R 'OBC_Components_EpsBridge_ut_exe|OBC_Components_EpsFdirController_ut_exe|eps_fdir_policy_unit_test|eps_fdir_integration_test|OBC_Components_ModeSafetyController_ut_exe|mode_safety_policy_integration_test' \
  --output-on-failure
```

Focused repository-owned probes:

```bash
bash scripts/run_eps_csp_integration.sh
bash scripts/run_eps_timeout_fdir_hosted_probe.sh
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1-final
```

OpenSpec validation and archive:

```bash
openspec validate fdir-subsystem-timeout-v1
openspec validate --specs
openspec archive fdir-subsystem-timeout-v1 --yes
```

## Evidence

Focused CTest:

```text
OBC_Components_ModeSafetyController_ut_exe: PASS
mode_safety_policy_integration_test: PASS
OBC_Components_EpsFdirController_ut_exe: PASS
eps_fdir_policy_unit_test: PASS
eps_fdir_integration_test: PASS
OBC_Components_EpsBridge_ut_exe: PASS
100% tests passed, 0 tests failed out of 6
```

Focused EPS CSP regression:

```text
eps_csp_integration_test: node=2 soc=76 pdu=0x3 tx=5 rx=10
```

Hosted EPS timeout FDIR probe:

```text
eps_timeout_fdir_hosted_probe: PASS log=$REPO_ROOT/build-artifacts/eps-timeout-fdir-debug/eps-timeout-fdir-hosted-probe.log
  transient_two_failures_no_safe: final=IDLE fault=False recovery=False log=$REPO_ROOT/build-artifacts/eps-timeout-fdir-debug/transient_two_failures_no_safe.log
  threshold_crossing_to_safe: final=SAFE fault=True recovery=False log=$REPO_ROOT/build-artifacts/eps-timeout-fdir-debug/threshold_crossing_to_safe.log
  recovery_after_fault_clear: final=SAFE fault=True recovery=True log=$REPO_ROOT/build-artifacts/eps-timeout-fdir-debug/recovery_after_fault_clear.log
```

Fresh repository gate:

```text
Canonical closeout command:
  bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1

Authoritative final rerun summary:
  scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1-final
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
openspec validate fdir-subsystem-timeout-v1: PASS
openspec validate --specs: PASS
openspec archive fdir-subsystem-timeout-v1 --yes: PASS
```

## Notes

- The hosted proof is scoped to the active `TopCcsds` schedule order `epsBridge -> epsFdirController -> modeSafetyController`; the legacy `Top` wiring is compile-safe regression support only.
- `EpsBridge.getStatusForRuntime()` now leaves FDIR consecutive-failure accounting unchanged so shell/runtime status reads do not perturb the timeout policy; scheduled poll success/failure remains the only FDIR input boundary in this slice.
- The hosted probe proves transient two-failure tolerance, threshold-crossing SAFE fallback, and explicit recovery clear on the first successful poll after simulator restart.
- The final hosted proof rerun uses event-driven recovery with isolated dynamic ports and a slower hosted tick so the transient two-failure case remains below the third-failure escalation boundary while still proving the real runtime path.
- `SAFE`/`HELL` no-duplicate-mode-request behavior is intentionally proven at the classic component level rather than by extending the hosted probe into a broader mode-matrix script.
- The fresh gate initially exposed an unrelated legacy `comm_csp_ground_gateway_probe_test` telemetry-capture fragility. The repository-owned probe was aligned to the existing bounded `fprime-cli channels --logs --log-directly --timeout` snapshot pattern before the authoritative rerun gate was recorded here.
