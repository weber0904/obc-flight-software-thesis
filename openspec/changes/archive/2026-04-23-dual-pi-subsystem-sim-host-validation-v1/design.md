## Overview

This change adds a governed three-host topology without redefining the existing two-host remote path:

- `macOS`: ground host running only `csp_zmqproxy` and headless `fprime-gds`
- `obc.local`: OBC target host running only the OBC deployment
- `subsystem.local`: subsystem simulator host running only `eps_simulator` node `2` and `adcs_simulator` node `3`

The implementation keeps internal CSP, ground-side GDS, and external comm as separate layers even when they coexist in one target run.

## Design Decisions

### Keep The Existing Two-Host Remote Path Intact

The current `rpi-remote-grounded-csp-validation-v1` scripts and evidence stay as-is. The new three-host topology is introduced as a separate governed path so historical evidence remains reusable and reviewers can distinguish which host ran the simulators.

### Use Explicit Role-Specific SSH Workflows

The OBC host continues to use:

- `OBC_SSH_TARGET`
- `RPI_SSH_TARGET` as a compatibility alias
- `RPI_REMOTE_DIR`

The subsystem simulator host introduces parallel workspace settings:

- `SUBSYSTEM_SIM_SSH_TARGET`
- `SUBSYSTEM_SIM_REMOTE_DIR`

This slice adds workspace sync/bootstrap plus runtime launch on `subsystem.local`, but it does not add installed-bundle or systemd lifecycle support there.

### Split Launchers By Host Responsibility

The old remote host stack launcher started everything on macOS. The new topology uses three separate launch responsibilities:

- `macOS` launcher: starts `csp_zmqproxy` and headless `fprime-gds` only
- subsystem-host launcher: starts `eps_simulator` and `adcs_simulator` only
- OBC-host launcher: starts OBC only against the remote ground/CSP host

`REMOTE_HOST` remains the single governed address for the macOS ground/CSP host to avoid expanding the configuration surface unnecessarily.

### Keep The Main Subsystem Proof And Comm Coexistence Proof Separate

The primary probe proves two related but distinct paths:

1. `obc.local -> subsystem.local` internal CSP reachability through the macOS-hosted hub
2. `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` bounded EPS and ADCS commands

External comm is intentionally excluded from that acceptance path to minimize variables after the hostname and multi-host changes.

A second probe proves coexistence:

- the same three-host subsystem path stays reachable
- `obc.local` runs external comm on `/dev/serial0`
- macOS runs `radio_mock_server`
- one governed target run can service both subsystem traffic and external comm without conflating the two paths

### Keep Claims Narrow

The three-host topology proves:

- split-host software deployment
- remote simulator reachability
- bounded GDS-driven subsystem command flow
- coexistence with external comm on the target

It does not prove:

- transparent/framed UART paths
- RF behavior
- vendor radio control-plane behavior
- future physical CAN/UART/RS485 carrier equivalence
- real EPS/ADCS hardware integration

## Risks / Mitigations

- **Risk:** SSH orchestration on the second Pi introduces new failure points unrelated to the subsystem logic.
  - **Mitigation:** add dedicated subsystem sync/bootstrap scripts and keep the launcher roles narrow so failures are attributable to one host.

- **Risk:** Reviewers may confuse the new three-host path with the existing two-host remote path.
  - **Mitigation:** keep the old scripts/evidence intact, add new script names and registry entries, and state the host responsibilities explicitly in docs and evidence.

- **Risk:** A single oversized probe would make failures hard to diagnose after hostname and role changes.
  - **Mitigation:** keep the main GDS/CSP probe and the comm coexistence probe as separate governed paths.
