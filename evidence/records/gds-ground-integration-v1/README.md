# gds-ground-integration-v1 Evidence

## Summary

- Goal: connect the hosted OBC deployment to `fprime-gds` through the documented F' command / event / telemetry stack
- Scope: GDS-connected topology, repo-local launch path, evidence of the hosted TCP ground link
- Path identity: this record proves the hosted direct `OBC -> GDS` TCP adapter path only; it does not by itself prove the separate `fprime-cli -> GDS` command/uplink path
- Date: 2026-03-21

## Automated build evidence

### Build

- Command:
  - `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
  - `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
- Result:
  - PASS
- Notes:
  - Rebuilt `OBC/Top/` after adding `CdhCore`, `ComFprime`, `Drv::TcpClient`, and rate-group wiring.

## Hosted ground-link evidence

### Headless GDS launch

- Command:
  - `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-gds -n -g none --framing-selection fprime --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json --ip-address 127.0.0.1 --ip-port 50000`
- Result:
  - PASS
- Observed:
  - `F prime is now running. CTRL-C to shutdown all components.`

### Hosted stack launch against GDS

- Command:
  - `bash scripts/run_gds_stack.sh`
- Result:
  - PASS
- Expected:
  - EPS simulator, ADCS simulator, radio mock server, and OBC runtime launch
  - OBC connects to the headless `fprime-gds` IP adapter on `127.0.0.1:50000`
- Observed:
  - `radio_mock_server listening on 127.0.0.1:7000`
  - OBC emitted command-registration, boot, health, CSP init, EPS status, and UART-open events
  - OBC printed `Connected to 127.0.0.1:50000 as a tcp client`
  - `lsof -nP -iTCP:50000` showed `ESTABLISHED` between `OBC` and the GDS Python process

## Remaining constraints

| Area | Status | Notes |
|------|--------|-------|
| Raspberry Pi target integration | `Deferred-RPi` | Validation remains on the hosted `dev-macos` profile |
| Physical UART and real radio hardware | `Blocked-HW` | Ground path is verified over hosted TCP only |
| Manual commanding through the full GUI flow | `Deferred-RPi` | This change validates the headless adapter path and hosted connectivity baseline |
