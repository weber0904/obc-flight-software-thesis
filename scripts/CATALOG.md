# Script Catalog

This catalog defines the maintained automation surface. Operator-facing
commands are listed first; implementation files are listed afterward so every
file under `scripts/` has an explicit owner and purpose.

## Operator Entry Points

### Build And Repository Verification

| Command | Purpose |
|---|---|
| `bash scripts/bootstrap_dev_config.sh` | Create the local command-auth configuration from the public example. |
| `bash scripts/run_verification_ci.sh` | Generate and build the hosted deployment and unit tests, run F Prime checks, and validate repository contracts. |

### Hosted Runtime

| Command | Purpose |
|---|---|
| `bash scripts/run_dev_stack.sh` | Start the default hosted OBC, subsystem simulators, COMM service, and GDS. |
| `bash scripts/run_hosted_stock_ground_stack.sh sband` | Start the maintained S-band stock ground surface. |
| `bash scripts/run_hosted_stock_ground_stack.sh uhf` | Start the maintained UHF stock ground surface. |
| `bash scripts/run_hosted_stock_ground_stack.sh combined` | Start both maintained stock ground surfaces against one hosted runtime. |
| `bash scripts/run_hosted_dual_link_orchestration.sh` | Start the hosted dual-link orchestration runtime. |
| `bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh` | Start the interactive hosted dual-GDS surface. |
| `bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh` | Inspect the hosted manual surface and its manifest. |
| `bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh` | Stop processes owned by the hosted manual surface. |
| `fprime-venv/bin/python scripts/mission_console/app.py` | Start Mission Console against an active manual surface. |

### Target And Laboratory Runtime

| Command | Purpose |
|---|---|
| `bash scripts/package_rpi_bundle.sh` | Build the versioned Raspberry Pi deployment bundle. |
| `bash scripts/install_rpi_bundle.sh <bundle>` | Verify and activate a versioned bundle on the OBC target. |
| `bash scripts/install_lab_can_services.sh` | Install the laboratory SocketCAN services. |
| `bash scripts/lab_can_status.sh` | Inspect laboratory SocketCAN service state. |
| `bash scripts/sync_subsystem_sim_workspace.sh` | Transfer the tracked workspace needed by the subsystem host. |
| `bash scripts/bootstrap_subsystem_sim_workspace.sh` | Create the subsystem-host environment and build its binaries. |
| `bash scripts/install_subsystem_comm_csp_services.sh` | Install EPS, ADCS, S-band, and UHF subsystem services. |
| `bash scripts/subsystem_comm_csp_status.sh` | Inspect the subsystem CSP services. |
| `bash scripts/install_rpi_comm_csp_autostart.sh` | Install the OBC COMM/CSP systemd service for the active bundle. |
| `bash scripts/rpi_comm_csp_status.sh` | Inspect the OBC COMM/CSP service and recent journal. |
| `bash scripts/ensure_target_comm_lab_baseline.sh` | Establish the shared target and subsystem baseline for a target probe. |
| `bash scripts/ensure_ground_dual_gds_baseline.sh` | Establish the shared ground-side GDS and gateway baseline. |
| `bash scripts/manual_ops/target/start_target_manual_baseline.sh` | Prepare target services for an interactive session. |
| `bash scripts/manual_ops/target/status_target_manual_baseline.sh` | Inspect the interactive target baseline. |
| `bash scripts/manual_ops/target/stop_target_manual_baseline.sh` | Close target-baseline session ownership. |
| `bash scripts/manual_ops/target/start_target_manual_ground_surface.sh` | Start the target-facing dual-GDS ground surface. |
| `bash scripts/manual_ops/target/status_target_manual_ground_surface.sh` | Inspect the target-facing ground surface. |
| `bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh` | Stop processes owned by the target-facing ground surface. |
| `python scripts/manual_ops/manual_secure_ops.py ...` | Establish authenticated sessions, send commands, and operate sequences and files. |
| `python3 scripts/chapter5_routes/eps_sim_control.py ...` | Query or change EPS simulator state during a hosted route. |
| `python3 scripts/chapter5_routes/adcs_sim_control.py ...` | Query or change ADCS simulator state during a hosted route. |

### Maintained Verification

| Command | Environment | Verified path |
|---|---|---|
| `bash scripts/run_csp_runtime_smoke.sh` | Hosted | CSP router and subsystem node reachability. |
| `bash scripts/run_eps_csp_integration.sh` | Hosted | EPS command and telemetry integration. |
| `bash scripts/run_adcs_csp_integration.sh` | Hosted | ADCS command and telemetry integration. |
| `bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | Hosted | S-band, UHF, and combined stock-ground launchers. |
| `bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh` | Hosted | Challenge handshake and authenticated command ingress. |
| `bash scripts/run_mission_console_phase1_hosted_probe.sh` | Hosted | Mission Console with the hosted manual surface. |
| `bash scripts/chapter5_routes/hosted/run_route1_hosted.sh` | Hosted | Sequence-driven payload capture, data product, and SoC fallback. |
| `bash scripts/chapter5_routes/hosted/run_route2_hosted.sh` | Hosted | Mission mode, TTC admission, failover, and housekeeping continuity. |
| `bash scripts/chapter5_routes/hosted/run_route3_hosted.sh` | Hosted | ADCS and EPS recovery-executor behavior. |
| `bash scripts/run_target_secure_auth_proof.sh` | Target/lab | Target S-band authenticated command and readback closure. |
| `bash scripts/run_target_autonomous_uhf_failover_probe.sh` | Target/lab | Autonomous S-band-to-UHF failover closure. |
| `bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh` | Target/lab | Hardware watchdog reset and post-restart state. |
| `bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh` | Target/lab | Payload preview, raw artifact, downlink, and extraction closure. |
| `bash scripts/chapter5_routes/target/run_route1_target.sh` | Target/lab | Sequence-driven payload capture and SoC fallback evidence route. |
| `bash scripts/chapter5_routes/target/run_route2_target.sh` | Target/lab | Mission mode, TTC admission, failover, and housekeeping evidence route. |
| `bash scripts/chapter5_routes/target/run_route3_target.sh` | Target/lab | ADCS recovery, EPS safe fallback, and watchdog evidence route. |
| `bash scripts/chapter5_routes/run_route1_sequence_formal_rerun.sh` | Hosted + target/lab | Build, deploy, run, import, and provenance-check the formal Route 1 campaign. |
| `bash scripts/run_boot_trust_chain_probe.sh` | Hosted | Signed-manifest boot selection and rejection behavior. |

## Complete File Inventory

### Shared Launch And Data Utilities

| File | Role | Purpose |
|---|---|---|
| `scripts/README.md` | Guide | Short workflow index for the script surface. |
| `scripts/CATALOG.md` | Catalog | Complete ownership and purpose inventory. |
| `scripts/_common.sh` | Library | Shared path, target-role, hashing, process, SSH, and build-output helpers. |
| `scripts/decode_beacon_v1.py` | Utility | Decode the versioned compact housekeeping beacon wire format. |
| `scripts/decode_ccsds_capture.py` | Utility | Inspect bounded CCSDS space-packet, TC-frame, and TM-frame captures. |
| `scripts/payload_fdp_extract.py` | Utility | Validate and extract official payload `.fdp` data products. |
| `scripts/probe_port_hygiene.sh` | Library | Refuse to reuse ports owned by unrelated processes. |
| `scripts/probe_process_utils.py` | Library | Start, identify, wait for, and reap probe-owned process trees. |
| `scripts/secure_link_auth_lib.py` | Library | Encode, decode, and verify secure-link handshake and command packets. |
| `scripts/hosted_secure_command_helpers.py` | Library | Shared hosted secure-session and command-observation helpers. |
| `scripts/security_server_sim.py` | Simulator | Local security-service endpoint used by hosted secure-link workflows. |

### Hosted Runtime Implementation

| File | Role | Purpose |
|---|---|---|
| `scripts/run_dev_stack.sh` | Entry point | Default integrated hosted runtime launcher. |
| `scripts/run_hosted_stock_ground_stack.sh` | Entry point | Parameterized S-band, UHF, or combined stock-GDS launcher. |
| `scripts/per_band_stock_ground_stacks.py` | Runtime owner | Own per-band OBC, COMM, gateway, GDS, manifest, and cleanup lifecycle. |
| `scripts/run_hosted_dual_link_orchestration.sh` | Entry point | Shell entry for dual-link orchestration. |
| `scripts/dual_link_orchestration.py` | Runtime owner | Own the dual-link runtime state and process lifecycle. |
| `scripts/run_ground_gds_only_stack.sh` | Internal launcher | Start a ground-only GDS/gateway stack used by target and per-band workflows. |
| `scripts/challenge_handshake_secure_command_hosted_probe.py` | Probe engine | Execute the hosted challenge-handshake scenario. |
| `scripts/run_challenge_handshake_secure_command_hosted_probe.sh` | Probe entry | Validate prerequisites and launch the secure-command probe engine. |
| `scripts/run_csp_runtime_smoke.sh` | Probe entry | Exercise the hosted CSP runtime at subsystem level. |
| `scripts/run_eps_csp_integration.sh` | Probe entry | Exercise the hosted EPS CSP path. |
| `scripts/run_adcs_csp_integration.sh` | Probe entry | Exercise the hosted ADCS CSP path. |
| `scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | Probe entry | Verify all modes of the parameterized stock-ground launcher. |
| `scripts/run_mission_console_phase1_hosted_probe.sh` | Probe entry | Exercise Mission Console through a live hosted surface. |
| `scripts/run_boot_trust_chain_probe.sh` | Probe entry | Exercise signed-manifest boot selection and rejection behavior. |
| `scripts/run_onboard_state_data_hosted_probe.sh` | Route stage | Validate onboard-state beacon and housekeeping-file continuity for Route 2. |
| `scripts/run_uhf_node6_backup_probe.sh` | Route stage | Validate the hosted UHF node-6 backup path for Route 2. |
| `scripts/run_recovery_executors_v1_probe.sh` | Route stage | Run scoped ADCS or EPS recovery-executor scenarios for Route 3. |
| `scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh` | Probe engine | Exercise hosted payload preview/raw-product and downlink behavior. |

### Hosted Chapter 5 Routes And Simulator Control

| File | Role | Purpose |
|---|---|---|
| `scripts/chapter5_routes/lib/common.sh` | Library | Stage logging and isolated runtime-root helpers shared by route runners. |
| `scripts/chapter5_routes/lib/eps_control_client.py` | Library | Shared EPS simulator control client used by hosted Route 1 and Route 2 stages. |
| `scripts/chapter5_routes/hosted/run_route1_hosted.sh` | Entry point | Compose the maintained hosted Route 1 stages. |
| `scripts/chapter5_routes/hosted/route1_prepare_and_capture.sh` | Route stage | Run sequence admission, payload capture, product creation, and extraction. |
| `scripts/chapter5_routes/hosted/route1_soc_fallback_check.sh` | Route stage | Validate Route 1 SoC admission and fallback behavior. |
| `scripts/chapter5_routes/hosted/run_route2_hosted.sh` | Entry point | Compose the maintained hosted Route 2 stages. |
| `scripts/chapter5_routes/hosted/route2_mode_ttc_entry.sh` | Route stage | Validate mode transitions and TTC-window admission. |
| `scripts/chapter5_routes/hosted/route2_link_recovery_and_hk.sh` | Route stage | Compose UHF recovery and housekeeping continuity checks. |
| `scripts/chapter5_routes/hosted/run_route3_hosted.sh` | Entry point | Run the scoped ADCS and EPS recovery-executor scenarios. |
| `scripts/chapter5_routes/adcs_sim_control.py` | Operator utility | Query or change ADCS simulator state through its control socket. |
| `scripts/chapter5_routes/eps_sim_control.py` | Operator utility | Query or change EPS SoC and load state through its control socket. |

### Target And Laboratory Implementation

| File | Role | Purpose |
|---|---|---|
| `scripts/package_rpi_bundle.sh` | Entry point | Assemble the target executable, launch assets, service units, metadata, and keystore. |
| `scripts/install_rpi_bundle.sh` | Entry point | Upload, verify, stage, and atomically activate a target bundle. |
| `scripts/sync_rpi_workspace.sh` | Evidence deployment | Synchronize the exact target workspace selected by the formal campaign. |
| `scripts/bootstrap_rpi_workspace.sh` | Evidence deployment | Build the synchronized workspace and record framework/project versions on the target. |
| `scripts/install_rpi_comm_csp_autostart.sh` | Entry point | Render and install the current OBC COMM/CSP systemd unit. |
| `scripts/rpi_comm_csp_status.sh` | Entry point | Report OBC COMM/CSP service state and journal. |
| `scripts/install_lab_can_services.sh` | Entry point | Render and install shared laboratory CAN units. |
| `scripts/lab_can_status.sh` | Entry point | Report the shared CAN-unit state. |
| `scripts/sync_subsystem_sim_workspace.sh` | Entry point | Synchronize source required by the subsystem simulator host. |
| `scripts/bootstrap_subsystem_sim_workspace.sh` | Entry point | Create the subsystem host virtual environment and build products. |
| `scripts/install_subsystem_comm_csp_services.sh` | Entry point | Render and install subsystem simulator and COMM systemd units. |
| `scripts/subsystem_comm_csp_status.sh` | Entry point | Report subsystem service state and journals. |
| `scripts/run_subsystem_csp_service.sh` | Service launcher | Dispatch the requested EPS, ADCS, S-band, or UHF service target. |
| `scripts/ensure_target_comm_lab_baseline.sh` | Baseline entry | Shell contract for target/subsystem baseline ownership. |
| `scripts/ensure_ground_dual_gds_baseline.sh` | Baseline entry | Shell contract for ground baseline ownership. |
| `scripts/comm_verification/lib/ensure_target_comm_lab_baseline.py` | Baseline engine | Reconcile target package, service profile, CAN, and subsystem services. |
| `scripts/comm_verification/lib/ensure_ground_dual_gds_baseline.py` | Baseline engine | Reconcile dual GDS, gateway, listener, port, and manifest state. |
| `scripts/run_target_secure_auth_proof.sh` | Probe entry | Run the integrated target secure-auth command scenario. |
| `scripts/run_target_autonomous_uhf_failover_probe.sh` | Probe entry | Run the integrated target autonomous failover scenario. |
| `scripts/run_rpi_target_hardware_watchdog_reset_probe.sh` | Probe entry | Run the integrated target watchdog-reset scenario. |
| `scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh` | Probe entry | Run the integrated target payload/downlink scenario. |
| `scripts/comm_verification/lib/run_target_can_matrix_probe.py` | Shared target engine | Provide target service, GDS, CSP, command, file, and observation primitives used by maintained probes. |
| `scripts/comm_verification/lib/run_target_secure_auth_command_path_probe.py` | Shared target engine | Establish authenticated target sessions and verify command/readback paths. |
| `scripts/comm_verification/lib/run_target_autonomous_uhf_failover_probe.py` | Probe engine | Drive and evaluate autonomous target failover. |
| `scripts/comm_verification/lib/target_autonomous_uhf_probe_common.py` | Probe library | Share failover scenario state, assertions, and cleanup. |
| `scripts/comm_verification/lib/managed_sband_unavailable_window.py` | Probe library | Create and restore a bounded S-band-unavailable window. |
| `scripts/comm_verification/lib/run_rpi_target_hardware_watchdog_reset_probe.py` | Probe engine | Drive watchdog expiry and validate restart state. |
| `scripts/comm_verification/lib/route3_target_probe_common.py` | Probe library | Share reboot, status capture, and recovery-history helpers used by watchdog validation. |
| `scripts/comm_verification/lib/run_payload_e2e_downlink_closure_v1_target_probe.py` | Probe engine | Drive capture, preview/raw product, authenticated downlink, and extraction. |
| `scripts/comm_verification/lib/payload_image_sanity.py` | Probe library | Validate payload image dimensions, luminance, and flip behavior. |

### Target Chapter 5 Evidence Routes

| File | Role | Purpose |
|---|---|---|
| `scripts/chapter5_routes/target/run_route1_target.sh` | Evidence runner | Compose target Route 1 sequence/payload and SoC-fallback stages. |
| `scripts/chapter5_routes/target/route1_prepare_and_capture.sh` | Evidence stage | Own A/B preparation and the target sequence-driven payload scenario. |
| `scripts/chapter5_routes/target/route1_soc_fallback_check.sh` | Evidence stage | Own A/B preparation and launch the target SoC-fallback scenario. |
| `scripts/chapter5_routes/target/route1_soc_fallback_check.py` | Evidence engine | Drive authenticated commands and evaluate target SoC fallback. |
| `scripts/comm_verification/lib/run_route1_sequence_payload_target_probe.py` | Evidence engine | Drive target sequence upload, execution, payload capture, downlink, and artifact checks. |
| `scripts/chapter5_routes/target/run_route2_target.sh` | Evidence runner | Compose target Route 2 mode/TTC and link-recovery stages. |
| `scripts/chapter5_routes/target/route2_mode_ttc_entry.sh` | Evidence stage | Own A/B preparation for target mode and TTC validation. |
| `scripts/chapter5_routes/target/route2_mode_ttc_entry.py` | Evidence engine | Drive authenticated target mode transitions and TTC-window assertions. |
| `scripts/chapter5_routes/target/route2_link_recovery_and_hk.sh` | Evidence stage | Own A/B preparation and compose target link recovery with housekeeping checks. |
| `scripts/chapter5_routes/target/route2_secure_auth_link_recovery.sh` | Evidence stage | Reuse the maintained autonomous failover proof for Route 2. |
| `scripts/chapter5_routes/target/run_route3_target.sh` | Evidence runner | Compose target ADCS, EPS, and watchdog recovery stages. |
| `scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh` | Evidence stage | Own A/B preparation and run the ADCS first-fault recovery scenario. |
| `scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh` | Evidence stage | Own A/B preparation and run the EPS recovery/safe-fallback scenario. |
| `scripts/chapter5_routes/target/route3_watchdog_reboot_and_postcheck.sh` | Evidence stage | Own A/B preparation and run watchdog reboot/postcheck. |
| `scripts/comm_verification/lib/run_target_route3_pre_reboot_recovery_probe.py` | Evidence engine | Drive ADCS/EPS faults and evaluate target pre-reboot recovery state. |
| `scripts/comm_verification/lib/managed_adcs_state_drop_window.py` | Evidence library | Create and restore the bounded ADCS-state loss used by Route 3. |
| `scripts/comm_verification/lib/managed_eps_status_drop_window.py` | Evidence library | Create and restore the bounded EPS-status loss used by Route 3. |

### Formal Route 1 Campaign

| File | Role | Purpose |
|---|---|---|
| `scripts/chapter5_routes/run_route1_sequence_formal_rerun.sh` | Campaign entry | Build hosted targets, synchronize/deploy the target, run both surfaces, and assemble the campaign. |
| `scripts/chapter5_routes/check_route1_target_revision_provenance.py` | Provenance gate | Bind local source, synchronized workspace, remote build, installed bundle, service unit, and executable hashes. |
| `scripts/chapter5_routes/import_formal_artifacts.py` | Artifact importer | Copy an attempt's declared artifacts into a stable campaign layout with hashes. |
| `scripts/chapter5_routes/check_formal_attempt_artifacts.py` | Artifact gate | Validate attempt manifests, paths, byte counts, hashes, and retained-file sets. |
| `scripts/chapter5_routes/write_route1_campaign_manifest.py` | Manifest writer | Record campaign identity, attempts, verdicts, and artifact references. |
| `scripts/check_route1_evidence_bundle.py` | Evidence gate | Validate the frozen Route 1 evidence bundle and decoded artifact relationships. |

### Manual Operator Surface

| File | Role | Purpose |
|---|---|---|
| `scripts/manual_ops/manual_secure_ops.py` | Operator utility | Authenticated command, sequence, file, and readback CLI for hosted and target surfaces. |
| `scripts/manual_ops/hosted/start_hosted_manual_surface.sh` | Entry point | Start the hosted interactive surface in a detached owner process. |
| `scripts/manual_ops/hosted/status_hosted_manual_surface.sh` | Entry point | Report hosted surface state. |
| `scripts/manual_ops/hosted/stop_hosted_manual_surface.sh` | Entry point | Stop hosted surface-owned processes. |
| `scripts/manual_ops/target/start_target_manual_baseline.sh` | Entry point | Establish target services for interactive operation. |
| `scripts/manual_ops/target/status_target_manual_baseline.sh` | Entry point | Report target baseline state. |
| `scripts/manual_ops/target/stop_target_manual_baseline.sh` | Entry point | Release target baseline session state. |
| `scripts/manual_ops/target/start_target_manual_ground_surface.sh` | Entry point | Start target-facing local GDS and gateway processes. |
| `scripts/manual_ops/target/status_target_manual_ground_surface.sh` | Entry point | Report target-facing ground state. |
| `scripts/manual_ops/target/stop_target_manual_ground_surface.sh` | Entry point | Stop target-facing ground processes. |
| `scripts/manual_ops/lib/__init__.py` | Package marker | Define the shared manual-operations library package. |
| `scripts/manual_ops/lib/common.py` | Library | Shared manifest, port, process, command, and path utilities. |
| `scripts/manual_ops/lib/detached_owner_launcher.py` | Library | Launch a surface owner outside the caller process session. |
| `scripts/manual_ops/lib/surface_owner.py` | Runtime owner | Own hosted or target manual-surface processes, manifests, and cleanup. |
| `scripts/manual_ops/lib/beacon_sidecar.py` | Runtime helper | Capture raw beacon frames for an interactive surface. |
| `scripts/manual_ops/examples/sample-sequence.bin` | Test fixture | Minimal compiled sequence consumed by the Mission Console hosted probe. |

### Mission Console

| File | Role | Purpose |
|---|---|---|
| `scripts/mission_console/__init__.py` | Package marker | Define the Mission Console package. |
| `scripts/mission_console/app.py` | Application entry | Serve the Mission Console web application and API. |
| `scripts/mission_console/probe_client.py` | Probe client | Exercise Mission Console HTTP and workflow behavior. |
| `scripts/mission_console/gateway/__init__.py` | Package marker | Define the Mission Console gateway package. |
| `scripts/mission_console/gateway/actions.py` | Gateway service | Execute commands, sequences, files, and readback actions. |
| `scripts/mission_console/gateway/beacon.py` | Gateway service | Manage beacon capture and decoded beacon state. |
| `scripts/mission_console/gateway/catalog.py` | Gateway service | Load command and telemetry dictionaries for the UI. |
| `scripts/mission_console/gateway/listeners.py` | Gateway service | Own telemetry, event, and channel listener processes. |
| `scripts/mission_console/gateway/locks.py` | Gateway service | Serialize operations that share a band surface. |
| `scripts/mission_console/gateway/packet_lab.py` | Gateway service | Build, inspect, and submit diagnostic packets. |
| `scripts/mission_console/gateway/parsers.py` | Gateway service | Parse GDS output, events, telemetry, files, and status data. |
| `scripts/mission_console/gateway/registry.py` | Gateway service | Discover hosted and target surfaces from their manifests. |
| `scripts/mission_console/gateway/sequence_authoring.py` | Gateway service | Render, compile, and validate sequence source. |
| `scripts/mission_console/gateway/snapshots.py` | Gateway service | Persist bounded console snapshots and trend samples. |
| `scripts/mission_console/static/mission-console.css` | Web asset | Mission Console layout and presentation. |
| `scripts/mission_console/static/mission-console.js` | Web asset | Browser interaction, polling, formatting, and chart behavior. |
| `scripts/mission_console/templates/base.html` | Web template | Shared page shell and navigation. |
| `scripts/mission_console/templates/dashboard.html` | Web template | Mission overview dashboard. |
| `scripts/mission_console/templates/ops.html` | Web template | Command and operational action view. |
| `scripts/mission_console/templates/beacon.html` | Web template | Beacon inspection view. |
| `scripts/mission_console/templates/packet_lab.html` | Web template | Packet construction and inspection view. |
| `scripts/mission_console/templates/readback.html` | Web template | Command and telemetry readback view. |
| `scripts/mission_console/templates/sequences.html` | Web template | Sequence authoring and execution view. |
| `scripts/mission_console/templates/surfaces.html` | Web template | Hosted and target surface status view. |
| `scripts/mission_console/templates/trends.html` | Web template | Time-series telemetry trend view. |

### Repository Governance And Focused Tests

| File | Role | Purpose |
|---|---|---|
| `scripts/check_repo_consistency.py` | Governance | Validate OpenSpec, reconciliation, and documentation consistency. |
| `scripts/check_public_release.py` | Governance | Validate public files, catalog coverage, evidence, credentials, licenses, and checksums. |
| `scripts/check_documentation_governance.py` | Governance | Validate the compact documentation surface and required navigation. |
| `scripts/check_transport_mtu_apid_contract.py` | Governance | Validate transport-size and APID ownership contracts. |
| `scripts/check_component_test_baseline.py` | Governance | Validate classic component-test coverage and inventory. |
| `scripts/check_legacy_zmq_retired.py` | Governance | Reject retired legacy ZMQ runtime paths. |
| `scripts/bootstrap_dev_config.sh` | Setup helper | Create a local command-auth configuration from the public example. |
| `scripts/run_verification_ci.sh` | Verification entry | Run the complete hosted build, test, and repository-governance gate. |
| `scripts/classify_change_scope.sh` | CI helper | Select lightweight or full CI from the changed-file set. |
| `scripts/generate_command_authority_catalog.py` | Build helper | Generate the command-authority opcode catalog consumed by F Prime tests. |
| `scripts/generate_reconciliation_matrix_md.py` | Governance helper | Render the canonical reconciliation matrix. |
| `scripts/generate_publication_manifest.py` | Release helper | Generate the source-to-public publication manifest. |
| `scripts/generate_verification_manifest.py` | Release helper | Generate the executable verification manifest. |
| `scripts/public-allowlist.txt` | Release policy | Enumerate every permitted file under `scripts/`. |
| `scripts/verification-manifest.json` | Release inventory | Record executable role, environment, ownership, and digest. |
| `scripts/verification_inventory_lib.py` | Governance library | Inventory component tests, helper tests, probes, and evidence records. |
| `scripts/test_decode_ccsds_capture.py` | Focused test | Exercise CCSDS decoder success and malformed-input paths. |
| `scripts/test_manual_surface_owner_launcher.py` | Focused test | Exercise detached owner launch semantics. |
| `scripts/test_mission_console_display_format.js` | Focused test | Exercise browser-side value formatting. |
| `scripts/test_mission_console_phase1.py` | Focused test | Exercise Mission Console gateway, API, persistence, and workflow behavior. |
| `scripts/test_payload_image_sanity.py` | Focused test | Exercise payload-image validation rules. |
| `scripts/test_target_baseline_canfd_ownership.py` | Focused test | Exercise CAN FD ownership and baseline reconciliation. |
| `scripts/test_target_beacon_sidecar_baseline.py` | Focused test | Exercise target beacon-sidecar baseline behavior. |
| `scripts/test_target_manual_beacon_cleanup.py` | Focused test | Exercise target manual-surface beacon cleanup. |
| `scripts/test_target_secure_command_readback_retry.py` | Focused test | Exercise secure-command readback retry behavior. |
| `scripts/test_managed_sim_drop_window.py` | Focused test | Exercise managed ADCS/EPS drop-window restoration and failure handling. |
| `scripts/test_import_formal_artifacts.py` | Focused test | Exercise formal-attempt import, manifest, and corruption checks. |
| `scripts/test_route1_evidence_bundle.py` | Focused test | Exercise Route 1 wrapper contracts and, with `ROUTE1_CAMPAIGN_ROOT`, evidence corruption detection. |
| `scripts/test_route1_target_revision_provenance.py` | Focused test | Exercise source, build, install, service-unit, and executable provenance gates. |
