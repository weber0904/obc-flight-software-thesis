## 1. Change Artifacts

- [x] 1.1 Add proposal, design, delta specs, and tasks for the three-host `macOS + obc.local + subsystem.local` topology.
- [x] 1.2 Keep the existing two-host remote path and the existing single-Pi comm evidence in scope as preserved baselines rather than redefining them.

## 2. Subsystem-Host Workflow

- [x] 2.1 Add subsystem-host workspace sync and bootstrap scripts that target `SUBSYSTEM_SIM_SSH_TARGET` and `SUBSYSTEM_SIM_REMOTE_DIR`.
- [x] 2.2 Add a subsystem-host launcher plus an SSH wrapper that starts only `eps_simulator` and `adcs_simulator` against the macOS CSP hub.
- [x] 2.3 Add a ground-host launcher that starts only `csp_zmqproxy` and headless `fprime-gds`.

## 3. Dual-Pi Probes

- [x] 3.1 Add the main dual-Pi split-host probe for CSP reachability plus GDS-driven `EPS_SET_PDU` and `ADCS_SET_MODE`.
- [x] 3.2 Add the separate dual-Pi comm coexistence probe for split-host CSP plus `/dev/serial0` external comm.
- [x] 3.3 Keep the old two-host probe intact and avoid silently repurposing its evidence.

## 4. Docs, Specs, And Evidence

- [x] 4.1 Update platform, comm, verification-evidence, and verification-path-registry deltas for the three-host topology.
- [x] 4.2 Update operator docs and verification docs so they distinguish the two-host and three-host remote paths and identify the new script entrypoints.
- [x] 4.3 Add a new evidence record for this change and update reconciliation surfaces.

## 5. Validation And Closeout

- [x] 5.1 Run `bash -n` on every modified shell script and rerun repo consistency checks.
- [x] 5.2 Validate the subsystem-host sync/bootstrap flow against `subsystem.local`.
- [x] 5.3 Run the main dual-Pi split-host probe and the separate dual-Pi comm coexistence probe.
- [x] 5.4 Run `openspec validate dual-pi-subsystem-sim-host-validation-v1` and `openspec validate --specs`.
- [x] 5.5 Archive the change, update main specs/reconciliation surfaces, and stop at local-ready without pushing.
