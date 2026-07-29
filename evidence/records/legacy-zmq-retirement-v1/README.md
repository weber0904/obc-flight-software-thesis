# legacy-zmq-retirement-v1 Evidence

## Summary

- Date: 2026-04-10
- Branch: `feature/legacy-zmq-retirement-v1`
- Change: `legacy-zmq-retirement-v1`
- Scope: retire the active EPS/ADCS project-local direct-ZMQ request/reply business path while preserving libcsp's hosted ZMQHUB-backed internal CSP substrate.

## Path Under Test

- Newly recorded guardrail path:
  - active EPS and ADCS traffic uses subsystem-owned CSP request/reply envelopes over libcsp
  - hosted development continues to use the libcsp ZMQHUB-backed internal substrate
  - `scripts/check_legacy_zmq_retired.py` rejects restoration of retired EPS/ADCS direct-ZMQ active-source patterns
- Reused registered paths:
  - hosted EPS internal libcsp service path
  - hosted ADCS internal libcsp service path
  - hosted internal libcsp foundation path

## Not Covered

- This is not a new ground-path proof.
- This is not an external comm, GPS, boot/update, CAN, scheduler, or hardware bring-up proof.
- Historical archived records remain historical and are not rewritten by this cleanup.

## Source Cleanup Summary

- Removed active EPS/ADCS direct-ZMQ transport classes and factory endpoint configuration.
- Removed the shared hosted protocol header from active source.
- Moved EPS state/result semantics to `simulators/eps/EpsTypes.hpp`.
- Moved ADCS state/result semantics to `simulators/adcs/AdcsTypes.hpp`.
- Kept EPS and ADCS CSP wire authority in `EpsCspProtocol.hpp` and `AdcsCspProtocol.hpp`.
- Removed the stale ADCS direct-ZMQ integration test source.
- Updated scenario and mission-policy integration tests to exercise active CSP request/reply helpers.

## Verification Commands

| Step | Command | Result |
|---|---|---|
| legacy direct-ZMQ checker | `python3 scripts/check_legacy_zmq_retired.py` | Pass; scanned 194 active files |
| source scan | `rg -n "<retired-patterns>" ...` | Pass; only checker pattern definitions matched |
| F' generate | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f` | Pass |
| F' build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build` | Pass |
| UT generate | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f` | Pass |
| UT build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut` | Pass |
| EPS CSP smoke | `bash scripts/run_eps_csp_integration.sh` | Pass; node `2`, SoC `76`, PDU `0x3`, tx `5`, rx `10` |
| ADCS CSP smoke | `bash scripts/run_adcs_csp_integration.sh` | Pass; node `3`, mode `2`, pointing error `3.72852 deg`, tx `16`, rx `32` |
| CSP runtime smoke | `bash scripts/run_csp_runtime_smoke.sh` | Pass; node `1`, tx `2`, rx `2`, free buffers `15` |
| scenario bridge regression | `./build-fprime-automatic-native-ut/bin/Darwin/scenario_bridge_integration_test` | Pass |
| repo consistency | `python3 scripts/check_repo_consistency.py` | Pass |
| verification inventory | `python3 scripts/report_verification_inventory.py --json` | Pass; inventory still reports EPS/ADCS CSP integration tests and no missing classic component L2 coverage |
| OpenSpec change validation | `openspec validate legacy-zmq-retirement-v1` | Pass; CLI emitted non-fatal PostHog DNS telemetry errors after success |
| OpenSpec specs validation | `openspec validate --specs` | Pass; 17 specs passed, 0 failed; CLI emitted non-fatal PostHog DNS telemetry errors after success |
| baseline gate | `bash scripts/run_verification_ci.sh build-artifacts/legacy-zmq-retirement-v1-final` | Pass through `11_openspec_validate_specs` |

## Wording Guardrails

- `csp_zmqproxy` and libcsp ZMQHUB remain part of the valid hosted internal CSP substrate.
- The retired path is the project-local EPS/ADCS direct-ZMQ request/reply business transport.
- Do not describe the GDS ground path, external comm path, or GPS path as part of this cleanup.
