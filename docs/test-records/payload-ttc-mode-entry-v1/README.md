# payload-ttc-mode-entry-v1 Test Record

## Scope

- Branch: `feature/payload-ttc-mode-entry-v1`
- Base commit: `189203d381d8320fbc15e2d337f3386445be17bb`
- Final local commit SHA: committed branch head; use `git rev-parse HEAD` on `feature/payload-ttc-mode-entry-v1`
- OpenSpec change: `payload-ttc-mode-entry-v1`
- Date: 2026-05-09

This record covers first-version manual PAYLOAD/TTC entry and exit guard behavior only. It does not cover scheduler behavior, ADCS ground tracking, payload/camera control, link authority, FDIR subsystem timeout/watchdog behavior, HK data-product field changes, UHF command authority, RF behavior, or CCSDS default-path changes.

## Build And Unit Verification

Commands run from `$REPO_ROOT`:

```sh
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j 8
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut -j 8
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-payload-ttc-mode-entry-v1
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeManager_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeSafetyController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/mode_safety_policy_integration_test
```

Result summary:

- Fresh verification gate: PASS at `build-artifacts/verification-ci-payload-ttc-mode-entry-v1`.
- `ModeManager` classic component UT: PASS, including all 25 operator from/to pairs, same-mode no-op behavior, rejection reason/event behavior, guard-unconfigured response, and guard path coverage.
- `ModeSafetyController` UT: PASS, including operator matrix coverage and `HELL -> SAFE` cached EPS cases `> 15`, `== 15`, `< 15`, and unavailable.
- Hosted runtime parser/unit test: PASS, including canonical lowercase mode spellings and mixed-case/invalid rejection behavior.
- Existing mode-safety helper/integration tests: PASS; autonomous SoC fallback/recovery behavior remains covered.

## Hosted Shell Regression

Command:

```sh
bash scripts/run_mode_model_v2_hosted_probe.sh
```

Result: PASS.

Evidence log: `/tmp/mode-model-v2-hosted.1BzpvJ/obc-mode-smoke.log`

Observed sequence excerpts:

```text
mode=SAFE
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from SAFE (0) to PAYLOAD (3) reason 1
mode response=2
mode=SAFE
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from SAFE (0) to TTC (4) reason 1
mode response=2
mode=SAFE
SYS_MODE_CHANGE : System mode changed to IDLE (1)
mode response=0
mode=IDLE
SYS_MODE_CHANGE : System mode changed to PAYLOAD (3)
mode response=0
mode=PAYLOAD
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from PAYLOAD (3) to TTC (4) reason 1
mode response=2
mode=PAYLOAD
SYS_MODE_CHANGE : System mode changed to SAFE (0)
mode response=0
mode=SAFE
SYS_MODE_CHANGE : System mode changed to TTC (4)
mode response=0
mode=TTC
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from TTC (4) to PAYLOAD (3) reason 1
mode response=2
mode=TTC
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from IDLE (1) to HELL (2) reason 2
mode response=2
mode=IDLE
unknown mode
```

The shell probe starts from `SYS_MODE == SAFE`, does not use illegal operator commands to reset state, and verifies parser errors for retired/unknown/mixed-case spellings remain parser-only.

## Hosted Safety Regression

Command:

```sh
bash scripts/run_mode_safety_policy_hosted_probe.sh
```

Result: PASS.

Evidence log: `/tmp/mode-safety-policy-hosted.WVzhsm/mode-safety-policy-hosted-probe.log`

Summary:

```text
safe_to_hell: initial_soc=9.00 final=HELL shutdown=terminated
idle_to_safe: initial_soc=39.00 final=SAFE shutdown=graceful
payload_to_safe: initial_soc=39.00 final=SAFE shutdown=terminated
ttc_to_safe: initial_soc=39.00 final=SAFE shutdown=terminated
safe_high_soc_remains_safe: initial_soc=60.00 final=SAFE shutdown=terminated
```

This regression keeps `ModeSafetyController` as SoC fallback owner. It no longer relies on operator `mode hell` to set up recovery state.

## CCSDS/F Prime Command Path Probe

Command:

```sh
bash scripts/run_payload_ttc_mode_entry_ccsds_probe.sh
```

Result: PASS.

Evidence directory: `/tmp/payload-ttc-mode-entry-ccsds.PiLQNE/`

The probe reuses the default hosted CCSDS S-band OBC path:

```text
fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> CSP -> OBC
```

Command sequence:

```text
MODE_SET PAYLOAD
MODE_SET TTC
MODE_SET IDLE
MODE_SET PAYLOAD
MODE_SET TTC
MODE_SET SAFE
```

`fprime-cli command-send` returned `returncode=0` for each send, meaning each command was accepted by the GDS CLI and dispatched far enough for command-status events. Command completion semantics were observed in events:

```text
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from SAFE to PAYLOAD reason 1
OpCodeError : Opcode 0x10030000 completed with error VALIDATION_ERROR
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from SAFE to TTC reason 1
OpCodeError : Opcode 0x10030000 completed with error VALIDATION_ERROR
SYS_MODE_CHANGE : System mode changed to IDLE
OpCodeCompleted : Opcode 0x10030000 completed
SYS_MODE_CHANGE : System mode changed to PAYLOAD
OpCodeCompleted : Opcode 0x10030000 completed
SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from PAYLOAD to TTC reason 1
OpCodeError : Opcode 0x10030000 completed with error VALIDATION_ERROR
SYS_MODE_CHANGE : System mode changed to SAFE
OpCodeCompleted : Opcode 0x10030000 completed
```

Initial telemetry snapshot:

```text
OBCApp.modeManager.SYS_MODE (268632064) SAFE
```

Final telemetry snapshots after the command sequence were captured through bounded `fprime-cli channels --search SYS_MODE` reads. The first two reads drained older queued `SYS_MODE` samples, and the third read proved the final accepted mode state:

```text
OBCApp.modeManager.SYS_MODE (268632064) IDLE
OBCApp.modeManager.SYS_MODE (268632064) PAYLOAD
OBCApp.modeManager.SYS_MODE (268632064) SAFE
```

The probe asserts command completion events, transition/rejection event presence, and final `SYS_MODE == SAFE` telemetry without depending on event/telemetry transport arrival ordering. It does not keep a channel listener open while sending commands because the current `fprime-cli` TTS listener behavior can delay command delivery when interleaved with command sends; instead, it captures initial and final bounded telemetry snapshots through the same GDS/CCSDS path.

## Verification Path Use

- Reused path: registry entry 42, default hosted CCSDS S-band node-5 adoption path.
- Upgraded evidence: this record adds bounded `OBCApp.modeManager.MODE_SET` command/rejection behavior and final `SYS_MODE` telemetry readback over that path.
- Updated shell/safety probes refine registry entries 34 and 35 so they no longer describe arbitrary operator mode setting or operator `mode hell` setup.

## Deferred Work

- SoC admission thresholds for `SAFE -> IDLE`, `IDLE -> PAYLOAD`, and `IDLE -> TTC`.
- Automatic TTC pass scheduling, TLE/GPS pass-window logic, and ground tracking.
- Payload/camera/recorder control, payload power sequencing, and payload data products.
- Link authority, auth/session policy, command sequence windows, and UHF failover authority.
- Subsystem timeout/retry/reset FDIR and watchdog behavior.
- HK data-product field changes and CCSDS path changes.
