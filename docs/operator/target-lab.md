# Target And Lab Operations

## Topology

The laboratory setup separates three roles:

```text
ground host
  F Prime GDS · Mission Console · gateway
             |
      S-band TCP / UHF service
             |
Raspberry Pi OBC target
  TopCcsds · CSP router · systemd · watchdog
             |
        SocketCAN / UART / TCP
             |
subsystem host
  EPS node 2 · ADCS node 3 · COMM nodes 5/6
```

Set the SSH targets used by the repository scripts:

```bash
export RPI_SSH_TARGET=operator@obc.local
export SUBSYSTEM_SSH_TARGET=operator@subsystem.local
```

## Package And Install

Build the target bundle with a private keystore:

```bash
OBC_PACKAGE_KEYSTORE_PATH=/absolute/path/private.ini \
  bash scripts/package_rpi_bundle.sh
```

Install the resulting archive:

```bash
bash scripts/install_rpi_bundle.sh \
  build-artifacts/packages/rpi/<release-id>/obc-rpi-<release-id>.tar.gz
```

The package manifest covers the executable, configuration, service units,
version metadata, and command-auth keystore. The installer verifies the
manifest before activation.

## Provision Laboratory Services

Install and inspect the CAN services:

```bash
bash scripts/install_lab_can_services.sh
bash scripts/lab_can_status.sh
```

Prepare the subsystem host:

```bash
bash scripts/sync_subsystem_sim_workspace.sh
bash scripts/bootstrap_subsystem_sim_workspace.sh
bash scripts/install_subsystem_comm_csp_services.sh
bash scripts/subsystem_comm_csp_status.sh
```

Install and inspect OBC COMM startup:

```bash
bash scripts/install_rpi_comm_csp_autostart.sh
bash scripts/rpi_comm_csp_status.sh
```

The service set includes EPS node `2`, ADCS node `3`, S-band node `5`, UHF node
`6`, the OBC CSP stack, and the active `TopCcsds` bundle.

## Prepare A Test Session

Target and ground preparation have separate owners:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/<registered-functional-probe>.sh
```

- the target baseline owns shared OBC, CAN, and subsystem services;
- the ground baseline owns GDS, CLI listeners, gateways, and local ports;
- the functional probe owns only its stimulus, observations, and output.

This order keeps service recovery separate from test assertions and makes a
failed layer visible.

## Interactive Ground Surface

Prepare the target:

```bash
bash scripts/manual_ops/target/start_target_manual_baseline.sh
bash scripts/manual_ops/target/status_target_manual_baseline.sh
```

Start local S-band and UHF ground surfaces:

```bash
MANUAL_TARGET_GROUND_AUTO_PORTS=1 \
GDS_UI_MODE=ui \
  bash scripts/manual_ops/target/start_target_manual_ground_surface.sh

bash scripts/manual_ops/target/status_target_manual_ground_surface.sh
cat /tmp/manual-dual-gds/target-ground/manifest.json
```

Fixed ports can be selected with:

```bash
export MANUAL_TARGET_SBAND_GDS_PORT=51900
export MANUAL_TARGET_SBAND_TTS_PORT=51901
export MANUAL_TARGET_UHF_GDS_PORT=51910
export MANUAL_TARGET_UHF_GDS_TTS_PORT=51911
export MANUAL_TARGET_SBAND_GUI_PORT=5100
export MANUAL_TARGET_UHF_GUI_PORT=5101
```

Use `scripts/manual_ops/manual_secure_ops.py` with `--env target` and the
manifest path to establish sessions and send commands.

## Representative Probes

```bash
bash scripts/run_target_secure_auth_proof.sh
bash scripts/run_target_autonomous_uhf_failover_probe.sh
bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
```

Consult the
[verification path registry](../../evidence/verification-path-registry.md)
before running a probe; its entry records the required topology, service
profile, authority path, and expected outputs.

## Integrated Evidence Routes

The Chapter 5 target runners compose the same baseline owners with the
route-specific functional stages:

```bash
bash scripts/chapter5_routes/target/run_route1_target.sh
bash scripts/chapter5_routes/target/run_route2_target.sh
bash scripts/chapter5_routes/target/run_route3_target.sh
```

Route 1 also provides a campaign runner that binds the hosted and target
results to the synchronized source, remote build, installed bundle, service
unit, and executable hashes:

```bash
EVIDENCE_DATE=YYYY-MM-DD \
  bash scripts/chapter5_routes/run_route1_sequence_formal_rerun.sh
```

Use the corresponding verification-registry entry to select environment
variables, target roles, and evidence destination before starting a campaign.

## Status And Diagnostics

```bash
bash scripts/lab_can_status.sh
bash scripts/subsystem_comm_csp_status.sh
bash scripts/rpi_comm_csp_status.sh
```

Inspect each service independently with `systemctl status` and
`journalctl -u <service>`. Confirm:

- the expected package version is active;
- CAN FD and destination allowlists match the probe;
- nodes `2`, `3`, `5`, and `6` are reachable;
- ground manifests point at the current dictionary and assigned ports;
- the authentication service ID matches the active band.

## Shutdown

```bash
bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```

The ground stop command cleans local operator processes. The target baseline
stop command retires its local session metadata and scoped overrides; shared
systemd services remain managed on their respective hosts.
