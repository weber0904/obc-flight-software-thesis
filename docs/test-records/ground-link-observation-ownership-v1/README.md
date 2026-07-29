# ground-link-observation-ownership-v1 Evidence

## Scope

This record governs the `ground-link-observation-ownership-v1` follow-up cleanup.

The change does not introduce a new validation path. It reuses existing COMM evidence to prove that the ground-link observation ownership cleanup did not change the active hosted paths:

- default hosted CCSDS S-band node-5 path: `docs/verification-path-registry.md` entry 43
- active hosted UHF CCSDS node-6 path: `docs/verification-path-registry.md` entry 43A
- hosted dual-link COMM simulator identity and generic node-4 compatibility path: `docs/verification-path-registry.md` entry 37

## Covered

- provider-facing ground-link observation types are owned by `OBC/Components/GroundLinkDriver/GroundLinkObservationRuntime.hpp`
- `simulators/comm` implements and populates the OBC-owned observation contract instead of owning it
- `GroundLinkHealthProvider` evaluates `GroundLinkHealthSemantics` rather than raw CSP node IDs
- active COMM CSP health policy remains scoped to node `5` S-band and node `6` UHF
- generic COMM node `4` remains executable compatibility evidence with connected-only fallback semantics
- `CommController` no longer stores direct `GroundLinkDriver*` policy dependencies

## Not Covered

- new MTU ceilings
- ARQ, NACK, CFDP, reliable-transfer, or retransmission behavior
- RSSI/SNR or radio-metrics ownership
- target timing closure
- full transport/probe interface split or `GroundLinkDriver` rename
- new RF, UART hardware, Raspberry Pi, or SocketCAN target evidence

## Governing Commands

- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all`
- `python3 scripts/check_component_test_baseline.py`
- `openspec validate ground-link-observation-ownership-v1`
- `openspec validate --specs`
- `bash scripts/run_sband_ccsds_primary_probe.sh`
- `bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh`
- `bash scripts/run_comm_csp_ground_gateway_probe.sh`

## Results

Date:

- `2026-05-16`

Final local-ready verification:

| Step | Command | Result |
|---|---|---|
| generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f` | PASS |
| build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build` | PASS |
| UT generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f` | PASS |
| UT build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut` | PASS |
| local gate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all` | PASS |
| component baseline | `python3 scripts/check_component_test_baseline.py` | PASS |
| OpenSpec change validate | `openspec validate ground-link-observation-ownership-v1` | PASS |
| OpenSpec spec validate | `openspec validate --specs` | PASS |
| focused hosted probe | `bash scripts/run_sband_ccsds_primary_probe.sh` | PASS |
| focused hosted probe | `bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh` | PASS |
| focused hosted probe | `bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |

Focused probe cleanup note:

- `scripts/run_comm_session_and_downlink_qos_probe.sh` was touched only for scoped cleanup hardening.
- The UHF focused probe was rerun after the fresh build and an immediate rerun verified cleanup safety.
- Post-run process scans found no owned-helper orphan or high-CPU residual `fprime_gds.executables.comm` process from the touched probe path.

Review follow-up verification:

| Step | Command | Result |
|---|---|---|
| affected build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build` | PASS |
| OpenSpec change validate | `openspec validate ground-link-observation-ownership-v1` | PASS |
| OpenSpec spec validate | `openspec validate --specs` | PASS |

## Verdict

PASS for the observation ownership cleanup.

- The observation contract is now OBC-owned.
- Runtime/topology configuration supplies health semantics before backend publication.
- Generic node `4` remains compatibility-only connected fallback and does not participate in active stale/activity or transport-growth policy.
- Active node `5` and node `6` COMM CSP semantics remain governed by the existing active hosted COMM paths.
