# target-node5-rg3-csp-runtime-contention-fix-v1 Evidence

Status: salvage record for the bounded RG3 blocker closure on the maintained
target node-`5` secure-auth path.
Last updated: 2026-06-11.

## Scope

This record is the clean-branch salvage evidence for the bounded RG3 blocker
fix. It does not preserve the earlier investigation diagnostics as mainline
product surfaces. Instead it keeps:

- the root-cause class established on the source debug branch
- the mainline-worthy code fixes that were salvaged onto a clean branch
- the clean-branch local verification
- the clean-branch target rerun used to confirm RG3 no longer slips on the
  reproduced node-`5` auth-open path

This is not the follow-up runtime-owner refactor and not a claim of full RG1
closure.

## Root-Cause Class

The source debug branch established the reproduced RG3 blocker as:

- class:
  `cross-group-or-thread-contention`
- exact blocker:
  `rateGroup3` observability work waited behind shared internal CSP runtime
  traffic
- mechanism:
  `LibCspRuntime::metrics()` shared the same mutex already owned by blocking
  `ping` and request/reply traffic, so observer reads could stall behind that
  critical section

The mainline salvage change keeps that classification, but does not keep the
debug-only member timing patch or state-monitor phase markers as part of the
product baseline.

## Salvaged Product Changes

### 1. Bounded CSP runtime metrics reads

`LibCspRuntime::metrics()` now attempts a non-blocking lock acquisition. When a
blocking CSP operation already owns the runtime mutex, the metrics reader
returns the latest cached snapshot instead of waiting behind the critical
section. The cache is refreshed after `init`, `ping`, `sendRaw`,
`requestReply`, and `shutdown`.

Touched files:

- `simulators/csp/CspRuntime.hpp`
- `simulators/csp/CspRuntime.cpp`

### 2. Cached `CommRuntimeState` publication

`CommController::getStateForRuntime()` now serves a cached
`OBC::CommRuntimeState` snapshot that is refreshed when runtime-visible state
changes are published.

Touched files:

- `OBC/Components/CommController/CommController.hpp`
- `OBC/Components/CommController/CommController.cpp`

## What Stayed Out Of Mainline

The salvage branch intentionally does **not** carry forward:

- dirty `lib/fprime` `ActiveRateGroup` timing diagnostics
- `OnboardStateMonitor` phase markers and other investigation-only
  diagnostics
- temporary poll-throttling knobs or cadence retuning from the later RG1
  checkpoint work
- the old dedicated slip-investigation probe wrappers

Those artifacts remain diagnosis provenance only.

## Clean-Branch Verification

### Local verification

Clean-branch local verification is recorded with the standard repo build path:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
./fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target OBC csp_runtime_smoke csp_zmqproxy csp_service_peer
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target OBC_Components_CommController_ut_exe
./build-fprime-automatic-native-ut/bin/OBC_Components_CommController_ut_exe --gtest_brief=1
./build-fprime-automatic-native/bin/csp_zmqproxy &
./build-fprime-automatic-native/bin/csp_service_peer &
./build-fprime-automatic-native/bin/csp_runtime_smoke
```

Observed clean-branch verdicts:

- `fprime-util generate -f`: PASS
- `fprime-util generate --ut -f`: PASS
- native focused build (`OBC`, `csp_runtime_smoke`, `csp_zmqproxy`,
  `csp_service_peer`): PASS
- `OBC_Components_CommController_ut_exe --gtest_brief=1`: PASS (`68` tests)
- `csp_runtime_smoke` with proxy/peer: PASS
  - final line:
    `csp_runtime_smoke: node=1 tx=2 rx=2 free=15`

### Target rerun

The clean-branch target confirmation uses the maintained A/B/C target proof
path and target journal truth:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
PROBE_ROOT=/tmp/target-node5-rg3-salvage-clean-rerun \
bash scripts/run_target_secure_auth_command_path_probe.sh
```

Review the captured target OBC journal around `SECURE_AUTH_ESTABLISHED` and the
same auth-open observation window used in the earlier diagnosis. The clean
rerun acceptance for this salvage record is:

- `OBCApp.rateGroup3Comp.RateGroupCycleSlip` does not appear in the reproduced
  auth-open window
- any remaining `rateGroup1` slip, if present, is recorded separately and not
  promoted into an RG3 claim

Observed clean-branch rerun artifact root:

- `/tmp/target-node5-rg3-salvage-clean-rerun`
- secure-auth proof summary:
  `/tmp/target-node5-rg3-salvage-clean-rerun/diagnostics/secure-auth-command-path-summary.json`
- target OBC journal:
  `/tmp/target-node5-rg3-salvage-clean-rerun/diagnostics/journal-snapshots/secure-auth-command-path-obc.log`

Observed clean-branch rerun verdicts:

- maintained `run_target_secure_auth_command_path_probe.sh`: PASS
- `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED`:
  `2026-06-11T21:21:57.642221`
- `SECURE_AUTH_ESTABLISHED`:
  `2026-06-11T21:21:57.642384`
- `rateGroup3Comp.RateGroupCycleSlip`: `0` matches in the captured target OBC
  journal and ground event log for this rerun
- residual `rateGroup1Comp.RateGroupCycleSlip`: present
  - first pre-auth-adjacent slip in the captured journal:
    `2026-06-11T21:21:57.420883` (`cycle 30`)
  - first post-auth slip:
    `2026-06-11T21:21:58.264229` (`cycle 32`)
  - later residual slips remain visible at cycles `34`, `35`, `36`, `37`,
    `38`, `40`, `43`, `46`, and `48`

Clean rerun interpretation:

- the bounded salvage fix preserves the maintained secure-auth command path
- the reproduced RG3 auth-open blocker is absent on the clean branch
- residual RG1 contention remains separate follow-up work and is not folded
  into this salvage claim

## Diagnosis Provenance

The pre-fix diagnosis provenance came from the source debug branch and is
retained only as review context for how the root-cause class was established.
It is not a required mainline diagnostic dependency.

Source-branch diagnosis provenance roots:

- pre-fix investigation root:
  `/tmp/rg-slip-investigation-state-monitor-refresh2`
- pre-fix investigation summary:
  `/tmp/rg-slip-investigation-state-monitor-refresh2/diagnostics/target-sband-rate-group-slip-investigation-summary.json`
- post-fix source-branch confirmation root:
  `/tmp/rg-slip-investigation-cspruntime-metrics-cache`
- post-fix source-branch confirmation summary:
  `/tmp/rg-slip-investigation-cspruntime-metrics-cache/diagnostics/target-sband-rate-group-slip-investigation-summary.json`
- clean-branch salvage rerun root:
  `/tmp/target-node5-rg3-salvage-clean-rerun`

## Bounded Claim

This record supports exactly this claim:

1. the reproduced node-`5` auth-open RG3 blocker was caused by shared CSP
   runtime observer contention, not by intrinsic RG3 membership size
2. the salvaged cached-metrics and cached-runtime-state fixes are sufficient to
   remove that reproduced RG3 blocker on a clean branch
3. any remaining RG1 contention is separate follow-up work
