# ttc-pass-window-mode-v1 Test Record

## Scope

- Branch: `feature/ttc-pass-window-mode-v1`
- Base commit: `bce5504`
- Final local commit SHA: branch worktree closeout state before push
- OpenSpec change: `ttc-pass-window-mode-v1`
- Date: 2026-05-13

This record covers the first bounded `TTC` pass-window automation closure on the active hosted `TopCcsds` baseline.

The active runtime claims proven here are:

- `TtcPassManager` is the single runtime owner of bounded `TTC` pass-window policy automation
- runtime accepts bounded TT&C config:
  - `enabled`
  - `loss_of_lock_timeout_sec`
- runtime accepts a single bounded pass-window input contract:
  - `start_unix_sec`
  - `end_unix_sec`
  - explicit clear command
- `IDLE -> TTC` auto-entry occurs only when:
  - `TT&C enabled`
  - pass window is active
  - cached GPS time basis is valid/fresh
  - current mode is `IDLE`
- `TTC -> IDLE` auto-exit occurs when:
  - pass window ends
  - pass window/config becomes invalid or cleared
  - GPS time basis becomes invalid
  - bounded COMM loss timeout is exceeded
- existing `ModeSafetyController` / `RecoveryExecutor` paths still retain precedence over TTC policy when safety/recovery conditions fire
- existing manual `MODE_SET TTC` / `MODE_SET IDLE` coexist with the automatic path without a parallel mode store

## Out Of Scope

- generic time-tagged scheduler behavior
- payload automation or payload scheduling
- TLE upload/parsing or onboard orbital propagation
- ADCS ground-target tracking or broad pointing framework
- target / Raspberry Pi deployment closure
- RF behavior or hardware radio proof

## Commands

Commands run from `$REPO_ROOT`:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-ttc-pass-window-mode-v1
PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut --output-on-failure -R '^(OBC_Components_TtcPassManager_ut_exe|ttc_pass_policy_unit_test|OBC_Components_CommController_ut_exe|hosted_runtime_unit_test)$'
bash scripts/run_ttc_pass_window_mode_v1_probe.sh
openspec validate ttc-pass-window-mode-v1
openspec validate --specs
```

## Results

- Fresh repository verification gate: PASS at `build-artifacts/verification-ci-ttc-pass-window-mode-v1/summary.md`
- Focused post-build CTest rerun: PASS for:
  - `OBC_Components_TtcPassManager_ut_exe`
  - `ttc_pass_policy_unit_test`
  - `OBC_Components_CommController_ut_exe`
  - `hosted_runtime_unit_test`
- Hosted TTC pass-window probe: PASS at `/tmp/ttc-pass-window-mode-v1.b5KVfs/ttc-pass-window-mode-v1-probe.log`
- `openspec validate ttc-pass-window-mode-v1`: PASS
- `openspec validate --specs`: PASS

## Focused Coverage

### 1. L1 TTC policy helpers

`ttc_pass_policy_unit_test` proves:

- epoch pass-window validation rejects `0` values and `end <= start`
- active-window comparison uses inclusive start and exclusive end
- bounded UTC `ymd + sec_of_day -> epoch` conversion works for valid GPS inputs and fails closed for impossible inputs
- GPS freshness logic requires advancing accepted-sentence count within the stale threshold
- COMM loss timer tracks elapsed no-link seconds only while `TTC` is active and no link is available

### 2. Real component L2 harness

`OBC_Components_TtcPassManager_ut_exe` proves:

- command handlers expose bounded TTC config, pass-window set/clear, and status surfaces
- invalid config/window inputs fail closed
- `IDLE -> TTC` entry requests use `ModeApplySource::TtcPassPolicy`
- `TTC -> IDLE` exit requests fire for:
  - disabled config
  - cleared window
  - inactive/ended window
  - invalid GPS time basis
  - bounded COMM loss timeout
- manual TTC entry and manual `MODE_SET IDLE` coexistence suppress same-window re-entry until the pass window changes or ends
- safety-precedence interactions keep `TtcPassManager` subordinate to the existing mode safety/recovery owner path

### 3. Hosted runtime surface

`hosted_runtime_unit_test` proves:

- hosted shell parses and validates:
  - `ttc status`
  - `ttc config <on|off> <loss-timeout-sec>`
  - `ttc window set <start-unix-sec> <end-unix-sec>`
  - `ttc window clear`
- hosted status output surfaces TTC policy state in a reviewable single-line form

### 4. COMM regression surface

`OBC_Components_CommController_ut_exe` remains PASS after the TTC policy change and keeps the COMM-owned runtime state surface stable for the v1 loss-of-lock proxy.

## Hosted Probe

Repository-owned proof entry point:

```bash
bash scripts/run_ttc_pass_window_mode_v1_probe.sh
```

Final passing run:

```text
ttc-pass-window-mode-v1-probe: PASS log=/tmp/ttc-pass-window-mode-v1.b5KVfs/ttc-pass-window-mode-v1-probe.log
formal-verdict=ttc-pass-window-mode-v1
case-disabled-no-entry=PASS mode=IDLE shutdown=terminated
case-enabled-auto-entry=PASS mode=TTC shutdown=terminated
case-window-end-auto-exit=PASS mode=IDLE shutdown=terminated
case-comm-loss-recovery-precedence=PASS mode=SAFE shutdown=terminated
case-safety-precedence=PASS mode=SAFE shutdown=terminated
case-manual-ttc-policy-coexistence=PASS mode=IDLE shutdown=terminated
case-manual-idle-coexistence=PASS mode=IDLE shutdown=terminated
```

This probe uses:

- hosted default `OBC` / `TopCcsds`
- hosted `csp_zmqproxy`, `eps_simulator`, `adcs_simulator`, and `radio_mock_server`
- hosted GPS replay input
- direct-TCP ground-link peer only as a bounded COMM availability source

The probe proves:

- pass-window input alone does not enter `TTC` when `enabled=no`
- enabled policy with valid GPS time and active epoch window auto-enters `TTC` from `IDLE`
- window end auto-exits `TTC -> IDLE`
- hosted direct-TCP COMM-loss on this baseline is preempted by the existing `COMM_PRIMARY_UNAVAILABLE` shared recovery path before a TTC policy `COMM_LOSS_TIMEOUT` exit is observed
- existing safety precedence still wins and drives `SAFE` when the low-SoC safety path triggers during `TTC`
- manual operator `MODE_SET TTC` still coexists with policy-owned automatic exit behavior
- manual operator `MODE_SET IDLE` during an active pass window stays in `IDLE` until the window changes or ends

The probe does **not** directly prove TTC timeout exit on the hosted direct-TCP COMM-loss path. That timeout contract is instead covered by:

- `ttc_pass_policy_unit_test` for zero-based elapsed-second timeout math
- `OBC_Components_TtcPassManager_ut_exe` for `COMM_LOSS_TIMEOUT`-driven `TTC -> IDLE` policy exits

The probe does **not** prove:

- default hosted CCSDS S-band GDS uplink behavior
- generic scheduler semantics
- ADCS tracking closure
- target/Pi deployment behavior

## Verification Path Use

- Reused path: hosted GPS fake/replay path for cached GPS time-basis behavior
- Reused path: hosted mock-radio TCP / direct-TCP ground-link wiring only as a bounded COMM availability dependency
- Reused path: hosted SoC-driven mode safety and hosted shared recovery behavior for precedence, not as the newly proven surface
- Newly proven path: hosted TTC pass-window policy automation on the active hosted `TopCcsds` runtime

## Non-Claims

- No generic time-tagged scheduler was added.
- No payload-operation scheduler or payload manager was added.
- No onboard TLE engine or orbital propagation was added.
- No ADCS hook beyond the existing bounded non-claim boundary was added.
- No target / Raspberry Pi proof was added.
