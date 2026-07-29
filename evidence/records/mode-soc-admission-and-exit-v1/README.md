# mode-soc-admission-and-exit-v1 Test Record

## Scope

- Branch: `feature/mode-soc-admission-and-exit-v1`
- Base commit: `9c4be0b636028843b911984e0f3e5e7bf5c6e334`
- Final local commit SHA: local worktree closeout state before push
- OpenSpec change: `mode-soc-admission-and-exit-v1`
- Date: 2026-05-10

This record covers the mode/power-safety vertical slice that extends the existing mode shell and cached EPS safety foundation:

- operator/manual `SAFE -> IDLE` requires cached EPS SoC `> 50%`
- operator/manual `IDLE -> PAYLOAD` requires cached EPS SoC `> 70%`
- missing or invalid cached EPS status fails closed for those guarded admissions
- automatic `PAYLOAD -> IDLE` occurs at cached EPS SoC `< 60%`
- existing `IDLE|PAYLOAD|TTC -> SAFE` at cached EPS SoC `< 40%` remains higher priority than `PAYLOAD -> IDLE`
- `HELL -> SAFE` cached-EPS guard `> 15%` and `SAFE -> HELL` `< 10%` behavior remain unchanged
- topology remains unchanged: `PAYLOAD` and `TTC` still enter only from `IDLE`; operator entry to `HELL` remains rejected

## Out Of Scope

- TTC pass scheduling, TLE/GPS pass-window logic, ADCS ground tracking, payload/camera/power control, watchdog/timeout/auth/session behavior, configurable SoC thresholds, RF behavior, target hardware, and MissionExecutive replacement.

## Commands

Original closeout commands run from `$REPO_ROOT`:

```sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-soc-admission-and-exit-v1
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeSafetyController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_integration_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeManager_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
bash scripts/run_mode_soc_admission_and_exit_hosted_probe.sh
bash scripts/run_mode_soc_admission_and_exit_ccsds_probe.sh
openspec validate mode-soc-admission-and-exit-v1
openspec validate --specs
openspec archive mode-soc-admission-and-exit-v1 --yes
python3 scripts/generate_reconciliation_matrix_md.py
python3 scripts/check_repo_consistency.py
```

Post-spec-repair no-rebuild verification rerun:

```sh
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeSafetyController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_integration_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeManager_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
bash scripts/run_mode_soc_admission_and_exit_hosted_probe.sh
bash scripts/run_mode_soc_admission_and_exit_ccsds_probe.sh
openspec validate --specs
python3 scripts/check_repo_consistency.py
```

## Results

- Fresh repository gate: PASS at `build-artifacts/verification-ci-mode-soc-admission-and-exit-v1`.
- Focused UTs: PASS for `ModeSafetyController`, `mode_safety_policy_unit_test`, `mode_safety_policy_integration_test`, `ModeManager`, `EpsBridge`, and `hosted_runtime_unit_test`.
- Post-spec-repair no-rebuild rerun: PASS for the same focused UT set, `openspec validate --specs`, and `python3 scripts/check_repo_consistency.py`.
- Hosted shell probe: original closeout PASS at `/tmp/mode-soc-admission-hosted.Nv82OP/mode-soc-admission-hosted-probe.log`; post-spec-repair no-rebuild rerun PASS at `/tmp/mode-soc-admission-hosted.De9rMj/mode-soc-admission-hosted-probe.log`.
- Default hosted CCSDS S-band probe: original closeout PASS at `/tmp/mode-soc-admission-ccsds.bGxSuM/summary.log`; post-spec-repair no-rebuild rerun PASS at `/tmp/mode-soc-admission-ccsds.90q8q5/summary.log`.
- OpenSpec archive: PASS, archived as `openspec/changes/archive/2026-05-09-mode-soc-admission-and-exit-v1/`.
- Post-archive reconciliation: `python3 scripts/generate_reconciliation_matrix_md.py` updated `docs/baseline-reconciliation-matrix.md`; `python3 scripts/check_repo_consistency.py` PASS with 92 archived changes checked.

## Focused Test Coverage

- `OBC_Components_ModeSafetyController_ut_exe` proves:
  - `SAFE -> IDLE` accepts at `50.01` and rejects at `50.0`, `49.99`, and unavailable cache
  - `IDLE -> PAYLOAD` accepts at `70.01` and rejects at `70.0`, `69.99`, and unavailable cache
  - `IDLE -> TTC` remains accepted without a new SoC gate
  - `HELL -> SAFE` still uses the shared `SOC_GUARD_UNAVAILABLE` / `SOC_GUARD_NOT_MET` codes
  - `PAYLOAD -> IDLE` at `< 60%` and `PAYLOAD -> SAFE` at `< 40%` both reach the normal mode-control surface with distinct internal apply sources
- `mode_safety_policy_unit_test` and `mode_safety_policy_integration_test` prove:
  - `PAYLOAD` at `60.0%` does not exit
  - `PAYLOAD` at `59.x%` exits to `IDLE`
  - `PAYLOAD` at `39.x%` exits to `SAFE`
  - `< 40%` takes precedence over `< 60%`
- `OBC_Components_ModeManager_ut_exe`, `OBC_Components_EpsBridge_ut_exe`, and `hosted_runtime_unit_test` remain green as affected regressions.

## Hosted Shell Probe

Command:

```sh
bash scripts/run_mode_soc_admission_and_exit_hosted_probe.sh
```

Result: PASS.

Evidence log from the post-spec-repair no-rebuild rerun: `/tmp/mode-soc-admission-hosted.De9rMj/mode-soc-admission-hosted-probe.log`

Summary:

```text
safe_to_idle_accept: initial_soc=60.00 final=IDLE shutdown=graceful
safe_to_idle_reject_boundary: initial_soc=50.00 final=SAFE shutdown=terminated
idle_to_payload_accept: initial_soc=71.00 final=PAYLOAD shutdown=graceful
idle_to_payload_reject_boundary: initial_soc=70.00 final=IDLE shutdown=terminated
idle_to_ttc_unchanged: initial_soc=60.00 final=TTC shutdown=graceful
payload_to_idle_automatic: initial_soc=71.00 final=IDLE shutdown=graceful
payload_to_safe_priority: initial_soc=71.00 final=SAFE shutdown=terminated
```

This probe uses the hosted CLI shell against the normal `ModeManager` guarded operator path and the active `ModeSafetyController` runtime path. The automatic-exit restart cases now wait for explicit post-restart `MODE_SAFETY_TRANSITION` evidence at `SOC 59.000000` and `SOC 39.000000` before asserting the final mode, so the `PAYLOAD -> IDLE` and `< 40% -> SAFE` verdicts are tied to the restarted EPS sample instead of generic pre-restart log text. The `payload_to_safe_priority` case explicitly proves that after `PAYLOAD` entry, the low-SoC transition goes directly to `SAFE` without a later `PAYLOAD -> IDLE` transition event.

## Default Hosted CCSDS S-band Probe

Command:

```sh
bash scripts/run_mode_soc_admission_and_exit_ccsds_probe.sh
```

Result: PASS.

Evidence directory from the post-spec-repair no-rebuild rerun: `/tmp/mode-soc-admission-ccsds.90q8q5/`

The probe reuses the default hosted CCSDS S-band path:

```text
fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> CSP -> OBC
```

Covered cases:

```text
safe_to_idle_accept
safe_to_idle_reject_boundary
safe_to_idle_reject_unavailable
idle_to_payload_accept
idle_to_payload_reject_boundary
idle_to_ttc_unchanged
```

Observed bounded outcomes:

- `MODE_SET IDLE` at SoC `60%` completes and reaches `SYS_MODE == IDLE`.
- `MODE_SET IDLE` at SoC `50%` emits `SYS_MODE_TRANSITION_REJECTED ... reason 4` and emits no `System mode changed to IDLE` event.
- `MODE_SET IDLE` after EPS simulator removal emits `SYS_MODE_TRANSITION_REJECTED ... reason 3` and emits no `System mode changed to IDLE` event.
- `MODE_SET PAYLOAD` after entering `IDLE` at SoC `71%` completes and reaches `SYS_MODE == PAYLOAD`.
- `MODE_SET PAYLOAD` after entering `IDLE` at SoC `70%` emits `SYS_MODE_TRANSITION_REJECTED ... reason 4`.
- `MODE_SET TTC` after entering `IDLE` at SoC `60%` completes and reaches `SYS_MODE == TTC`, confirming `IDLE -> TTC` stayed outside the new SoC admission rule.

The probe uses bounded `fprime-cli channels --search SYS_MODE` snapshots for `SYS_MODE` readback. For rejection-only unchanged-mode cases, the verdict is bounded by the latest unchanged `SYS_MODE` snapshot before the rejected command, the rejection event, and the absence of the forbidden accepted-transition event. That rule applies to the `SAFE -> IDLE` rejection cases and to the `IDLE -> PAYLOAD` rejection-at-`70%` case; it avoids over-claiming a fresh unchanged-mode telemetry emission that the current `MODE_GET` listener timing does not guarantee.

The probe also now treats `MODE_SET` command-path semantics as first-class evidence. Each routed `MODE_SET` waits for the matching `CdhCore.cmdDisp.OpCodeCompleted ... Opcode 0x10030000 completed` or `CdhCore.cmdDisp.OpCodeError ... Opcode 0x10030000 completed with error VALIDATION_ERROR` event before the next command is sent, so acceptance and guard rejection are both proven over the same default hosted CCSDS S-band uplink path instead of inferred from the local `fprime-cli` process exit code alone.

## Verification Path Use

- Reused path: registry entry 34 for the hosted shell/operator surface.
- Reused path: registry entry 35 for automatic cached-EPS mode safety behavior.
- Reused path: registry entry 42 for default hosted CCSDS S-band `MODE_SET` behavior.
- Upgraded evidence: this record extends those existing paths with SoC-gated `SAFE -> IDLE`, SoC-gated `IDLE -> PAYLOAD`, automatic `PAYLOAD -> IDLE`, `< 40%` precedence to `SAFE`, unchanged `IDLE -> TTC`, and unavailable-cache fail-closed behavior over the default CCSDS S-band command path.

## Deferred Work

- No new `IDLE -> TTC` SoC threshold was added.
- No TTC scheduler, pass-window targeting, ADCS ground tracking, payload sequencing, watchdog, auth/session, or configurable threshold command work was added.
- No new parallel mode manager or MissionExecutive replacement was introduced.
