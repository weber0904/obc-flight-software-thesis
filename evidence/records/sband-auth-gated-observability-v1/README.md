# sband-auth-gated-observability-v1 Evidence

Status: fresh hosted and target evidence rerun on this branch on 2026-06-05.

## Scope

This record captures the bounded node-`5` S-band observability-governance
proofs added by `sband-auth-gated-observability-v1`.

It covers:

- hosted node-`5` S-band pre-auth quiet
- hosted node-`5` post-auth live event/channel visibility
- hosted node-`5` bounded authenticated `GET_RESET_CAUSE` summary readback
- hosted node-`5` live close on explicit switch away from S-band primary
- service-managed target node-`5` S-band pre-auth quiet
- service-managed target node-`5` post-auth live event/channel visibility
- service-managed target node-`5` bounded authenticated `GET_RESET_CAUSE`
  summary readback
- service-managed target node-`5` live close on explicit switch to
  `uhf-primary-after-failover`, plus restored S-band baseline on exit

It does **not** cover:

- pre-auth broad S-band live chatter as a baseline requirement
- auth-free `GET_*` summary readback on node `5`
- post-auth node-`5` event/channel filtering or fine-grained live-tier
  selection inside the auth-gated stream
- generic summary-routing redesign or a second command plane
- broad non-quiet UHF operator closure
- RF, reliable transfer, one-GDS aggregation, or one-gateway simultaneous
  multiplexer behavior
- replacement of the broader hosted/target secure-auth authority proofs

## Commands

Hosted proof:

```bash
bash scripts/run_sband_observability_governance_hosted_probe.sh
```

Target proof:

```bash
bash scripts/run_target_sband_observability_governance_probe.sh
```

Hosted sequencing historical-wrapper status check:

```bash
bash scripts/run_official_sequencing_system_resources_v1_probe.sh
```

Target secure-auth control regression:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

## Acceptance

The proof is accepted only when all of the following are true:

- pre-auth S-band packetized live `event/tlm` remains quiet on node `5`
- accepted S-band secure auth opens node-`5` live event/channel visibility
- bounded authenticated `GET_RESET_CAUSE` summary readback remains visible on
  the same node-`5` path
- explicit switch away from S-band primary closes old node-`5` live
  observability again
- target proof restores S-band baseline before cleanup completes

## Fresh Evidence

Hosted observability governance:

- Command: `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- Verdict: `PASS`
- Probe root: `/tmp/sband-observability-governance-hosted.ong9k0`
- Summary markers:
  - `case-pre-auth-sband-live-quiet=PASS`
  - `case-post-auth-sband-live-open=PASS`
  - `case-authenticated-get-reset-cause-summary-readback=PASS`
  - `case-primary-switch-closes-sband-live-observability=PASS`
- Key artifacts:
  - events: `/tmp/sband-observability-governance-hosted.ong9k0/combined-stack/sband-ground/logs/events.log`
  - channels: `/tmp/sband-observability-governance-hosted.ong9k0/combined-stack/sband-ground/logs/channels.log`
  - OBC log: `/tmp/sband-observability-governance-hosted.ong9k0/combined-stack/logs/obc.log`
  - S-band capture: `/tmp/sband-observability-governance-hosted.ong9k0/combined-stack/captures/sband-southbound-to-gds.bin`

Target observability governance:

- Target install refresh:
  - `bash scripts/bootstrap_rpi_workspace.sh`
  - `bash scripts/package_rpi_bundle.sh`
  - `bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-196-gef75fa221-dirty/obc-rpi-v0.1.0-196-gef75fa221-dirty.tar.gz`
- First rerun result:
  - pre-auth quiet failure on stale installed release proved the service-managed
    path was still running an older `$OBC_HOME/obc-deploy/current`
  - after install refresh, pre-auth quiet passed and the remaining proof issue
    reduced to a target-probe oracle mismatch on boolean channel formatting
- Final command: `bash scripts/run_target_sband_observability_governance_probe.sh`
- Final verdict: `PASS`
- Probe root: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.EqP939`
- Summary markers:
  - `case-installed-release-keystore-provenance=PASS`
  - `case-pre-auth-sband-live-quiet=PASS`
  - `case-post-auth-sband-live-open=PASS`
  - `case-authenticated-get-reset-cause-summary-readback=PASS`
  - `case-primary-switch-closes-sband-live-observability=PASS`
  - `case-restore-sband-primary=PASS`
- Key artifacts:
  - summary: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.EqP939/diagnostics/target-sband-observability-governance-summary.json`
  - checkpoints: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.EqP939/diagnostics/checkpoints.jsonl`
  - OBC journal: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.EqP939/diagnostics/journal-snapshots/target-sband-observability-governance-obc.log`

Target secure-auth control regression:

- Command: `bash scripts/run_target_secure_auth_command_path_probe.sh`
- Verdict: `PASS`
- Probe root: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.UIdWhB`
- Summary markers:
  - `case-installed-release-keystore-provenance=PASS`
  - `case-sband-apid-00fe-secure-auth=PASS`
  - `case-sband-secure-command-get-reset-cause-sequence=41`
  - `sband-secure-command-source=target-journal`
- Key artifact:
  - summary: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.UIdWhB/diagnostics/secure-auth-command-path-summary.json`

Hosted official sequencing/system-resources historical-wrapper status:

- Command: `bash scripts/run_official_sequencing_system_resources_v1_probe.sh`
- Verdict: `FAIL` before probe runtime setup
- Failure:
  - `RuntimeError: legacy command-envelope v1 tuple material is retired from the tracked command-auth keystore contract`
- Interpretation:
  - this matches the current runbook truth that the wrapper is supplemental
    historical evidence after `legacy-command-envelope-retirement-v1`
  - this change does not rewrite that historical wrapper into a new secure
    command-plane implementation

Focused UTs:

- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 86 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommEgressMux_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 13 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 67 tests.`)
