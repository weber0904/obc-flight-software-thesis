# target-tcp-southbound-parity-foundation-v1 Evidence

Current-note:

- this record remains valid for target TCP southbound carrier parity and
  file/readback transport behavior
- the retained `SESSION_OPEN` wording is historical auth-boundary language and
  must not be cited as current secure-auth truth for maintained target routes

Date:
- `2026-05-22`

OpenSpec change:
- `target-tcp-southbound-parity-foundation-v1`

## Scope

This record covers the target TCP parity foundation cells closed on the current
branch:

- `csp-reachability`
- `sband-command`
- `sband-file`
- `uhf-primary-command`
- `uhf-primary-file`

The governed development-carrier topology is:

- `macOS`: `csp_zmqproxy`, `fprime-gds`, `ground_ttc_gateway`, ground-side
  listeners, and file stores
- `obc.local`: `OBC` only
- `subsystem.local`: `sband_comm_csp_node(node 5)`,
  `uhf_comm_csp_node(node 6)`, `eps_simulator(node 2)`,
  `adcs_simulator(node 3)`

This record does not claim:

- target TCP sequence or failover closure
- target direct-control closure
- target physical UHF UART provenance

## Implemented Harness

Branch-local matrix ownership now includes:

- dedicated target TCP helper:
  [scripts/comm_verification/lib/run_target_tcp_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_matrix_probe.py)
- dedicated remote stack launchers:
  - [scripts/comm_verification/lib/run_target_tcp_obc_stack.sh]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_obc_stack.sh)
  - [scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh)
- dedicated target TCP case entrypoints:
  - [scripts/comm_verification/cases/csp-reachability.sh]($REPO_ROOT/scripts/comm_verification/cases/csp-reachability.sh)
  - [scripts/comm_verification/cases/sband-command.sh]($REPO_ROOT/scripts/comm_verification/cases/sband-command.sh)
  - [scripts/comm_verification/cases/sband-file.sh]($REPO_ROOT/scripts/comm_verification/cases/sband-file.sh)
  - [scripts/comm_verification/cases/uhf-primary-command.sh]($REPO_ROOT/scripts/comm_verification/cases/uhf-primary-command.sh)
  - [scripts/comm_verification/cases/uhf-primary-file.sh]($REPO_ROOT/scripts/comm_verification/cases/uhf-primary-file.sh)

The helper now owns:

- probe-owned runtime roots and copied artifacts
- governed macOS ground orchestration plus remote OBC/subsystem launch
- authenticated node-`5` and node-`6` command/session handling
- official `.fdp` source snapshotting and GDS byte-match validation
- local helper reap for GDS listeners, event readers, and target TCP proxy
- startup retry plus explicit proxy-readiness gating before remote launch
- distinct free-port allocation for CSP hub and southbound listeners

## Verification

Static checks:

```bash
python3 -m py_compile \
  scripts/comm_verification/lib/run_target_can_matrix_probe.py \
  scripts/comm_verification/lib/run_target_tcp_matrix_probe.py

bash -n \
  scripts/comm_verification/cases/csp-reachability.sh \
  scripts/comm_verification/cases/sband-command.sh \
  scripts/comm_verification/cases/sband-file.sh \
  scripts/comm_verification/cases/uhf-primary-command.sh \
  scripts/comm_verification/cases/uhf-primary-file.sh \
  scripts/comm_verification/matrix/run_env_matrix.sh \
  scripts/comm_verification/lib/run_target_tcp_obc_stack.sh \
  scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh
```

Immediate rerun command:

```bash
COMM_VERIFICATION_ENV=rpi_tcp \
COMM_VERIFICATION_CASES='csp-reachability sband-command sband-file uhf-primary-command uhf-primary-file' \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing rerun artifact roots:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T071825Z`
- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T072544Z`

Observed results on both reruns:

- `csp-reachability`: `PASS`
- `sband-command`: `PASS`
- `sband-file`: `PASS`
- `uhf-primary-command`: `PASS`
- `uhf-primary-file`: `PASS`

Representative passing markers:

- `csp-reachability`
  - nodes `2`, `3`, `5`, and `6` reachable on the governed three-host parity
    launcher
- `sband-command`
  - historical then-current authenticated node-`5` `SESSION_OPEN`
  - subsystem-backed `EPS_GET_STATUS` readback through the same ground path
- `sband-file`
  - official `.fdp` downlink through node `5`
  - GDS-received file byte-match against a target-side source snapshot
- `uhf-primary-command`
  - explicit `COMM_SET_ACTIVE(UHF)` switch
  - historical authenticated node-`6` `SESSION_OPEN`
  - subsystem-backed `EPS_GET_STATUS` readback through the same UHF ground path
- `uhf-primary-file`
  - explicit switch to `UHF primary`
  - official `.fdp` downlink through node `6`
  - GDS-received file byte-match against a target-side source snapshot

## Bounded Debug Findings

This change closed the foundation by fixing two matrix-owned launcher bugs
found during rerun hardening:

- target TCP startup previously raced remote subsystem startup against a local
  `csp_zmqproxy` that was not yet listening
- target TCP port allocation previously derived `CSP_HUB_PUB_PORT` from
  `CSP_HUB_SUB_PORT + 1000`, which could exceed the valid TCP port range

Current branch truth after those fixes:

- two immediate reruns of the full five-cell target TCP foundation submatrix
  passed
- each passing rerun left no surviving matrix-owned local helper processes on
  the macOS host
- node `6` remains explicitly scoped to TCP-based southbound emulation, not
  physical UART provenance
