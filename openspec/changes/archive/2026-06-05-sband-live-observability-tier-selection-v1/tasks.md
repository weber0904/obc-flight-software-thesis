## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `comm-subsystem`, `interface-contract-index`, `eps-subsystem`, `gps-subsystem`, `adcs-subsystem`, `storage-health`, `resource-storage`, `onboard-data-products-and-live-beacon`, and `verification-path-registry`.
- [x] 1.2 Validate the change artifacts before closeout with `openspec validate sband-live-observability-tier-selection-v1`.

## 2. Component Observability Split

- [x] 2.1 Refactor `EpsBridge`, `GpsBridge`, and `AdcsBridge` so scheduled refresh publishes only current keep-live summary plus critical transition/fault events, while explicit GET/control readback keeps detailed bounded telemetry.
- [x] 2.2 Refactor `RadioController` so scheduled refresh publishes only summary observation state and explicit GET/control readback publishes detailed radio status through one shared apply/readback path.
- [x] 2.3 Refactor `StorageHealthBridge` so `STORAGE_GET_STATUS` performs a fresh scan with detailed bounded readback, scheduled scan stays summary-oriented, and `STORAGE_SCAN_NOW` is removed.
- [x] 2.4 Update focused component UT coverage for the new scheduled-vs-readback split and the retired storage command surface.

## 3. Authority, Probes, And Baseline Oracles

- [x] 3.1 Remove `STORAGE_SCAN_NOW` from the command authority catalog/policy and update any focused command-policy tests impacted by the command-surface retirement.
- [x] 3.2 Update shared hosted/target readiness or oracle helpers that currently depend on broad scheduled `EPS_STATUS_RECEIVED` or `GPS_STATE_UPDATED` chatter.
- [x] 3.3 Add or update hosted and target node-`5` observability probes so they prove curated post-auth summary live visibility, GET-triggered detailed readback, and the continued auth-gated access boundary.

## 4. Current Docs, Specs, And Evidence

- [x] 4.1 Update current docs (`docs/interfaces.md`, current architecture, roadmap, and relevant thesis/reference pages) to describe the four-tier model, fresh-by-default external readback, retired `STORAGE_SCAN_NOW`, and `non-baseline live` surfaces.
- [x] 4.2 Update verification-path and evidence records so node-`5` hosted/target observability proof truth now includes curated live summary and explicit detailed readback rather than broad post-auth chatter.

## 5. Validation And Closeout

- [x] 5.1 Run focused component UTs for `EPS`, `GPS`, `ADCS`, `RADIO`, `STORAGE`, plus any command-authority test touched by the retired storage command.
- [x] 5.2 Run the hosted and target node-`5` observability probes plus the secure-auth control regression needed to prove the access gate still holds.
- [x] 5.3 Run `openspec validate sband-live-observability-tier-selection-v1` and `openspec validate --specs`, then update this task list with the completed results.

Completed verification on 2026-06-06:

- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_AdcsBridge_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RadioController_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_StorageHealthBridge_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`: `PASS`
- `bash scripts/run_sband_observability_governance_hosted_probe.sh`: `PASS`
- `bash scripts/run_target_sband_observability_governance_probe.sh`: `PASS`
- `bash scripts/run_target_secure_auth_command_path_probe.sh`: `PASS`
- `openspec validate sband-live-observability-tier-selection-v1`: `PASS`
- `openspec validate --specs`: `PASS`
