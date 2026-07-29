# Maintained Scripts

This public release ships only maintained entrypoints and their transitive
support code. The exhaustive source disposition is recorded in
`verification-manifest.json`.

## Setup And CI

- `bootstrap_dev_config.sh`: create the ignored hosted-development keystore
- `run_verification_ci.sh`: full build, unit-test and governance gate
- `check_public_release.py`: public content, script, credential and manifest gate

## Hosted Operations

- `run_dev_stack.sh`: default hosted S-band development stack
- `run_gds_stack.sh`: hosted runtime against a separately started GDS
- `run_hosted_sband_stock_ground_stack.sh`
- `run_hosted_uhf_stock_ground_stack.sh`
- `run_hosted_per_band_stock_ground_stacks.sh`
- `run_per_band_stock_ground_stacks_hosted_probe.sh`
- `run_challenge_handshake_secure_command_hosted_probe.sh`
- `run_sband_observability_governance_hosted_probe.sh`
- `run_mission_console_phase1_hosted_probe.sh`
- `chapter5_routes/hosted/run_route{1,2,3}_hosted.sh`

## Target And Lab Baselines

- `ensure_target_comm_lab_baseline.sh`: A, shared target readiness owner
- `ensure_ground_dual_gds_baseline.sh`: B, shared ground readiness owner
- `sync_rpi_workspace.sh`, `bootstrap_rpi_workspace.sh`
- `package_rpi_bundle.sh`, `install_rpi_bundle.sh`
- install/status/autostart helpers for OBC and subsystem services

Current target functional entrypoints remain reviewable for future reruns, but
this public tag does not claim they were freshly executed.

## Focused Integration

- `run_eps_csp_integration.sh`
- `run_adcs_csp_integration.sh`
- current COMM node-5 transport/observability probes
- current payload official-product probes
- current boot, recovery and watchdog capability probes

## Historical Names

Historical, deprecated, fail-closed, matrix, alias-only and superseded wrappers
are not executable in this public tree. Their names, source object IDs, reasons
and successor-selection rule remain in `verification-manifest.json`; their
results remain in OpenSpec and test-record summaries.
