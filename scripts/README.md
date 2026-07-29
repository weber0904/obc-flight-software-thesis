# Scripts

The script surface is organized around five reproducible workflows. The
machine-readable file set is defined by
[`public-allowlist.txt`](public-allowlist.txt).

## Build And Verification

```bash
bash scripts/bootstrap_dev_config.sh
bash scripts/run_verification_ci.sh
```

The verification gate builds the hosted deployment and unit tests, runs all
F Prime checks, validates repository contracts, and validates OpenSpec.

## Hosted Operation

```bash
bash scripts/run_dev_stack.sh
bash scripts/run_hosted_sband_stock_ground_stack.sh
bash scripts/run_hosted_uhf_stock_ground_stack.sh
bash scripts/run_hosted_per_band_stock_ground_stacks.sh
```

Representative hosted verification:

```bash
bash scripts/run_csp_runtime_smoke.sh
bash scripts/run_eps_csp_integration.sh
bash scripts/run_adcs_csp_integration.sh
bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
bash scripts/run_mission_console_phase1_hosted_probe.sh
```

## Target And Laboratory

Deployment and shared test preparation:

```bash
bash scripts/package_rpi_bundle.sh
bash scripts/install_rpi_bundle.sh <bundle.tar.gz>
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
```

Representative target verification:

```bash
bash scripts/run_target_secure_auth_proof.sh
bash scripts/run_target_autonomous_uhf_failover_probe.sh
bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
```

## Thesis Routes

```bash
bash scripts/chapter5_routes/hosted/run_route1_hosted.sh
bash scripts/chapter5_routes/hosted/run_route2_hosted.sh
bash scripts/chapter5_routes/hosted/run_route3_hosted.sh
```

Target counterparts and the formal Route 1 rerun are under
[`chapter5_routes/`](chapter5_routes/).

## Mission Console

Manual data surfaces are under [`manual_ops/`](manual_ops/). Start a surface,
then launch:

```bash
fprime-venv/bin/python scripts/mission_console/app.py
```

See [`docs/operator/mission-console.md`](../docs/operator/mission-console.md)
for the operator workflow.
