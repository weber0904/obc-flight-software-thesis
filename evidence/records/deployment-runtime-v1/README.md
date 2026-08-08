# deployment-runtime-v1 Evidence

## Summary

- Goal: close the gap between isolated capability slices and a runnable hosted OBC project
- Scope: hosted OBC deployment, integrated dev stack launcher, standalone radio comm mock, basic operator control loop
- Date: 2026-03-21

## Automated build evidence

### Build

- Command:
  - `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
- Result:
  - PASS
- Notes:
  - Produced `build-fprime-automatic-native/bin/Darwin/OBC`

### Unit and integration regression gate

- Commands:
  - `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
  - `/bin/zsh -lc 'PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all'`
- Result:
  - PASS
- Observed:
  - `12/12` tests passed

## Integrated runtime evidence

### Full dev stack launch

- Command:
  - `bash scripts/run_dev_stack.sh`
- Result:
  - PASS
- Expected:
  - EPS simulator, ADCS simulator, radio mock server, and OBC runtime all launch
  - OBC runtime shows boot / health / CSP init events
- Observed:
  - `radio_mock_server` listened on `127.0.0.1:7000`
  - OBC printed `SYS_BOOT`, `HEALTH_CHECKING_SET`, `CSP_INIT_COMPLETE`, `EPS_STATUS_RECEIVED`, and `UART_OPEN`
  - runtime entered interactive mode with `OBC runtime started. Type 'help' for commands.`

### Interactive operations

- Commands exercised:
  - `status`
  - `radio status`
  - `eps pdu 1 on`
  - `adcs mode detumble`
  - `comm pass start 5`
- Result:
  - PASS
- Observed:
  - `status` printed mode / CSP / comm / boot / EPS / ADCS / radio / UART summary
  - `eps pdu 1 on` returned response `0`
  - `adcs mode detumble` returned response `0` and emitted `ADCS_MODE_CHANGE` + `ADCS_DETUMBLE_COMPLETE`
  - `comm pass start 5` returned response `0` and `status` showed `passActive=yes`

## Remaining constraints

| Area | Status | Notes |
|------|--------|-------|
| Raspberry Pi target integration | `Deferred-RPi` | This change validates only the hosted `dev-macos` profile |
| Physical UART and real radio hardware | `Blocked-HW` | PTY/TCP mock remain the available replacement paths |
| Full GDS-integrated deployment | `Deferred-RPi` | Hosted runtime currently uses a local REPL instead of a complete uplink/downlink stack |
