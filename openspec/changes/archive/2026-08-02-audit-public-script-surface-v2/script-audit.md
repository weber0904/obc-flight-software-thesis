# Script Surface Audit

## Method

The audit read all 180 tracked files under `scripts/` in full: 179 text files
containing 61,245 lines and one binary sequence fixture, for 2,529,274 bytes in
total. Classification used file contents, shell call chains, Python imports,
Mission Console asset loading, CI/CMake references, operator documentation,
the verification-path registry, and governing evidence records.

The retained disposition is recorded file by file in `scripts/CATALOG.md`.
The catalog identifies role, workflow owner, and purpose for every retained
file; `scripts/check_public_release.py` requires catalog paths, exact allowlist
paths, and files on disk to remain identical.

## Removed Files

| File | Disposition reason |
|---|---|
| `scripts/chapter5_routes/README.md` | Duplicated the canonical operator guides and script catalog. |
| `scripts/chapter5_routes/hosted/route1_downlink_and_extract.sh` | Unused wrapper; the retained Route 1 capture stage performs product extraction directly. |
| `scripts/chapter5_routes/hosted/route3_adcs_r3_hosted.sh` | Parameter-only wrapper folded into the retained Route 3 runner. |
| `scripts/chapter5_routes/hosted/route3_eps_r3_safe_hosted.sh` | Parameter-only wrapper folded into the retained Route 3 runner. |
| `scripts/chapter5_routes/hosted/route3_recovery_chain_pre_reboot.sh` | Redundant wrapper layer folded into the retained Route 3 runner. |
| `scripts/chapter5_routes/target/route1_downlink_and_extract.sh` | Unused wrapper; target Route 1 performs downlink and extraction in its maintained C-stage engine. |
| `scripts/chapter5_routes/target/route2_manual_playbook.md` | Duplicated the target operator guide and was not an executable dependency. |
| `scripts/chapter5_routes/target/route2_target_file_downlink.sh` | Unused intermediate probe; the retained payload/downlink and Route 2 paths provide the integrated closures. |
| `scripts/chapter5_routes/target/route3_recovery_chain_pre_reboot.sh` | Redundant aggregate; the retained target Route 3 runner owns all three stages. |
| `scripts/install_rpi_autostart.sh` | Superseded by the maintained COMM/CSP service installer. |
| `scripts/rpi_autostart_status.sh` | Superseded by the maintained COMM/CSP service status command. |
| `scripts/manual_ops/README.md` | Duplicated the hosted, target, and Mission Console operator guides. |
| `scripts/manual_ops/examples/route1-demo.seq` | No maintained caller; evidence routes create their governed sequence inputs. |
| `scripts/manual_ops/examples/sample-sequence.seq` | Unused source fixture; the compiled fixture required by the Mission Console probe remains. |
| `scripts/manual_ops/target/route2_ttc_gps_replay.py` | Unused one-route manual helper; no current operator or evidence path invokes it. |
| `scripts/run_gds_stack.sh` | Duplicated the maintained integrated hosted launcher with only printed manual instructions. |
| `scripts/run_hosted_sband_stock_ground_stack.sh` | Consolidated into `run_hosted_stock_ground_stack.sh sband`. |
| `scripts/run_hosted_uhf_stock_ground_stack.sh` | Consolidated into `run_hosted_stock_ground_stack.sh uhf`. |
| `scripts/run_hosted_per_band_stock_ground_stacks.sh` | Consolidated into `run_hosted_stock_ground_stack.sh combined`. |

## Retention Boundaries

- Hosted runtime, focused hosted integration, and all three hosted Chapter 5
  routes remain complete.
- Target packaging, installation, systemd provisioning, A/B baselines, manual
  surfaces, four representative end-to-end probes, all three target Chapter 5
  evidence routes, and the formal Route 1 campaign remain complete.
- Formal Route 1 artifact import, provenance checking, campaign manifests, and
  focused corruption-path tests remain because they validate current
  evidence-registry claims.
- Mission Console remains as one application closure, including templates,
  static assets, gateway modules, probe client, and focused tests.
