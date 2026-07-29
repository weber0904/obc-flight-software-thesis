# Hosted Operations

## Prerequisites

- create `fprime-venv` and install `requirements.txt`;
- run `bash scripts/bootstrap_dev_config.sh`;
- generate and build `OBC/TopCcsds`;
- ensure the selected ports are free.

```bash
fprime-venv/bin/fprime-util generate -f
fprime-venv/bin/fprime-util build
```

## Development Stack

Start the default OBC, simulators, COMM service, and ground data system:

```bash
bash scripts/run_dev_stack.sh
```

Use `RUNTIME_ROOT` to isolate logs, file stores, and process metadata:

```bash
RUNTIME_ROOT=/tmp/obc-hosted-demo \
  bash scripts/run_dev_stack.sh
```

The launcher prints the GDS URL, data ports, runtime directory, and log paths.
Stop it with `Ctrl-C`; the launcher owns cleanup of the processes it starts.

## S-Band And UHF Ground Stacks

Run one or both stock F Prime ground surfaces against a shared hosted OBC:

```bash
bash scripts/run_hosted_sband_stock_ground_stack.sh
bash scripts/run_hosted_uhf_stock_ground_stack.sh
bash scripts/run_hosted_per_band_stock_ground_stacks.sh
```

For an orchestrated dual-link runtime:

```bash
DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT=/tmp/dual-link-runtime \
  bash scripts/run_hosted_dual_link_orchestration.sh
```

Each launcher writes a manifest containing the assigned ports, process IDs,
southbound link, and log locations.

## Interactive Dual-GDS Surface

Start the operator surface with automatic port allocation:

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted \
MANUAL_HOSTED_RUNTIME_ROOT=/tmp/manual-dual-gds/runtime \
MANUAL_HOSTED_AUTO_PORTS=1 \
GDS_UI_MODE=ui \
  bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

Inspect it:

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted \
  bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh

cat /tmp/manual-dual-gds/hosted/manifest.json
```

The manifest exposes the S-band and UHF `gdsPort`, `gdsTtsPort`, `guiUrl`, and
file-storage directory.

Establish an S-band session and send a command:

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish

python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

After a link transition, establish the UHF-primary session with
`--band uhf-primary-after-failover`.

Stop the surface with the same root:

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted \
  bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh
```

## Mission Console

With a manual surface running:

```bash
MISSION_CONSOLE_ROOT=/tmp/mission-console \
MISSION_CONSOLE_PORT=5080 \
  fprime-venv/bin/python scripts/mission_console/app.py
```

Open `http://127.0.0.1:5080`. See [Mission Console](mission-console.md) for the
operator flow.

## Focused Verification

Representative hosted probes:

```bash
bash scripts/run_csp_runtime_smoke.sh
bash scripts/run_eps_csp_integration.sh
bash scripts/run_adcs_csp_integration.sh
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh
bash scripts/run_mission_console_phase1_hosted_probe.sh
bash scripts/chapter5_routes/hosted/run_route1_hosted.sh
bash scripts/chapter5_routes/hosted/run_route2_hosted.sh
bash scripts/chapter5_routes/hosted/run_route3_hosted.sh
```

Select the exact command and prerequisites from the
[verification path registry](../../evidence/verification-path-registry.md).

## Troubleshooting

1. Rebuild the deployment.
2. Inspect the launcher manifest rather than assuming default ports.
3. Check the OBC, gateway, COMM node, simulator, and GDS logs independently.
4. Confirm the command-auth file exists and has mode `0600`.
5. Stop the owning launcher before retrying; do not kill unrelated processes by
   name.
