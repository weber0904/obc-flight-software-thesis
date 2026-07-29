# Test Record: internal-csp-foundation-v1

- Date: 2026-04-10
- Change: `internal-csp-foundation-v1`
- Scope: hosted internal libcsp foundation only
- Related specs:
  - `platform-baseline`
  - `core-system-contracts`
  - `verification-evidence`

## Path Under Test

- Hosted internal libcsp foundation path
  - repo-local `csp_zmqproxy` starts the hosted hub
  - hosted OBC `CspBridge` initializes libcsp node `1`
  - hosted CSP peer node accepts ping and raw-send diagnostics over the same hub

## Adjacent Paths Explicitly Not Proven By This Record

- direct `OBC -> GDS` TCP adapter path
- `fprime-cli -> GDS` command or uplink path
- EPS business traffic migration
- ADCS business traffic migration
- external comm or transparent UART paths

## Truth Sources Reviewed

- `openspec/specs/platform-baseline/spec.md`
- `openspec/specs/core-system-contracts/spec.md`
- `openspec/specs/verification-evidence/spec.md`
- `docs/verification-path-registry.md`
- `obc-dev-spec/00_platform_baseline.md`
- `obc-dev-spec/01_core_system_contracts.md`
- `obc-dev-spec/07_verification_evidence.md`
- `OBC/Components/CspBridge/*`
- `simulators/csp/*`
- `scripts/run_dev_stack.sh`
- `scripts/run_gds_stack.sh`
- `scripts/run_rpi_stack.sh`

## Automated Verification

| Step | Command | Expected | Observed | Verdict |
|---|---|---|---|---|
| 1 | `ctest --output-on-failure --test-dir build-fprime-automatic-native-ut -R OBC_Components_CspBridge_ut_exe` | Updated classic `CspBridge` harness passes with real-runtime facade semantics | Pass during local UT build and verification gate | Pass |
| 2 | `bash scripts/run_csp_runtime_smoke.sh` | Hosted node `1` initializes libcsp, pings hosted peer node `2`, sends bounded raw payload, and reports nonzero metrics | `csp_runtime_smoke: node=1 tx=2 rx=2 free=15` | Pass |
| 3 | `bash scripts/run_verification_ci.sh build-artifacts/internal-csp-foundation-foundation1-rerun` | Repository baseline gate passes with the new CSP foundation assets included | `01_generate` through `10_openspec_validate_specs` all passed after the smoke-script lifecycle fix and the `CspBridge` UT expectation fix | Pass |
| 4 | `openspec validate internal-csp-foundation-v1` | Change validates | `Change 'internal-csp-foundation-v1' is valid` | Pass |
| 5 | `openspec validate --specs` | Main specs validate | Passed as `10_openspec_validate_specs` within the shared verification gate | Pass |

## Hosted Smoke Configuration

- `CSP_HUB_HOST=127.0.0.1`
- `CSP_HUB_SUB_PORT=56100`
- `CSP_HUB_PUB_PORT=57100`
- `LOCAL_NODE_ID=1`
- `PEER_NODE_ID=2`

## Smoke Notes

- The hosted smoke uses libcsp's official ZMQHUB-backed interface semantics and a repo-local `csp_zmqproxy` binary.
- The smoke is intentionally foundation-only. It proves runtime bring-up and peer reachability, not subsystem service migration.
- `CspBridge` remains the runtime owner and diagnostic facade; subsystem business traffic migration belongs to later EPS and ADCS vertical slices.

## Conclusion

- The repository now has a governed hosted internal libcsp foundation path distinct from the ground path and the external comm path.
- This change does not yet claim that EPS or ADCS business traffic has moved off the legacy direct-ZMQ request or response substrate.
