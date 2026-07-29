# libcsp-base-mainline-release-v1 Evidence

## Scope

This record captures release-readiness evidence for using `feature/libcsp-internal-network-base` as the next mainline baseline candidate.

## Environment

- Date: `2026-04-10`
- Workspace: `$REPO_ROOT`
- Branch: `feature/libcsp-internal-network-base`
- Host / target mode: macOS development host; Raspberry Pi and hardware validation are recorded separately

## Included Migration Slices

- `internal-csp-foundation-v1`
- `eps-csp-vertical-slice-v1`
- `adcs-csp-vertical-slice-v1`
- `legacy-zmq-retirement-v1`

## Path Under Test

- Release-readiness of the libcsp integration base before PR back to `main`.
- Reused registered paths:
  - hosted internal libcsp foundation path
  - hosted EPS internal libcsp service path
  - hosted ADCS internal libcsp service path
  - legacy EPS/ADCS direct-ZMQ retirement guardrail

## Not Covered

- This is not Raspberry Pi target execution evidence.
- This is not external comm UART hardware evidence.
- This is not GPS live UART evidence.
- This is not real EPS/ADCS/radio hardware evidence.

## Verification Commands

| Step | Command | Result |
|---|---|---|
| legacy direct-ZMQ checker | `python3 scripts/check_legacy_zmq_retired.py` | PASS; scanned 195 active-source files |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS; 17 main specs and 43 archived changes checked |
| verification inventory | `python3 scripts/report_verification_inventory.py --json` | PASS; inventory generated with the Pi CSP + comm probe present |
| agent entrypoint check | `python3 scripts/check_agent_entrypoint.py` | PASS |
| component test baseline | `python3 scripts/check_component_test_baseline.py` | PASS |
| OpenSpec change validation | `openspec validate libcsp-base-mainline-release-v1` | PASS |
| OpenSpec specs validation | `openspec validate --specs` | PASS; 17/17 specs valid |
| full baseline gate | `bash scripts/run_verification_ci.sh build-artifacts/libcsp-base-mainline-release-v1-final` | PASS through `11_openspec_validate_specs` |

## Verification Summary

- Verdict: PASS for local release-readiness of the libcsp integration base.
- This record supports opening a PR from `feature/libcsp-internal-network-base` back to `main`.

## Wording Guardrails

- The active EPS/ADCS internal path is libcsp over the hosted ZMQHUB-backed substrate.
- The retired path is project-local direct ZMQ REQ/REP business traffic.
- Release readiness does not imply Pi, GPS, GDS-command, RF, or real-subsystem hardware coverage.
