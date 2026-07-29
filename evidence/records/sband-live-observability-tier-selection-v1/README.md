# sband-live-observability-tier-selection-v1 Evidence

Status: fresh hosted and target evidence rerun on this branch on 2026-06-06.

## Scope

This record captures the first node-`5` S-band live-content selection proof
for `sband-live-observability-tier-selection-v1`.

It covers:

- hosted node-`5` pre-auth S-band quiet
- hosted node-`5` post-auth curated keep-live summary visibility
- hosted node-`5` bounded detailed authenticated readback for representative
  `EPS`, `GPS`, `ADCS`, `RADIO`, and `STORAGE` `GET_*` commands
- hosted authenticated cached-truth `GET_RESET_CAUSE` readback
- hosted live close on explicit switch away from S-band primary
- service-managed target node-`5` installed-release provenance
- service-managed target node-`5` post-auth curated keep-live summary for the
  deterministic `EPS`, `ADCS`, `RADIO`, and `STORAGE` families on the
  maintained path
- service-managed target node-`5` bounded detailed authenticated readback for
  the same representative `GET_*` commands
- service-managed target node-`5` live close on explicit switch to UHF primary
  plus restored S-band baseline on exit
- target secure-auth control regression to confirm the access gate baseline is
  still intact after content selection

It does **not** cover:

- pre-auth broad S-band live chatter as baseline truth
- auth-free `GET_*` summary or detailed readback on node `5`
- generic event/tlm packet filtering, `TlmPacketizer`, or a second command
  plane
- full removal of residual `systemResources`, transport/queue, or COMM
  internal runtime surfaces now classified as `non-baseline live`
- non-quiet UHF operator closure, RF, reliable transfer, or one-GDS
  aggregation behavior

## Commands

Hosted proof:

```bash
bash scripts/run_sband_observability_governance_hosted_probe.sh
```

Target install refresh:

```bash
bash scripts/bootstrap_rpi_workspace.sh
bash scripts/package_rpi_bundle.sh
bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-197-gc99119e5d-dirty/obc-rpi-v0.1.0-197-gc99119e5d-dirty.tar.gz
```

Target proof:

```bash
bash scripts/run_target_sband_observability_governance_probe.sh
```

Target secure-auth control regression:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

Focused UTs:

```bash
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_AdcsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RadioController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_StorageHealthBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
```

## Acceptance

The proof is accepted only when all of the following are true:

- pre-auth node-`5` S-band packetized live `event/tlm` remains quiet
- accepted S-band secure auth opens node-`5` live visibility
- scheduled `EPS`, `GPS`, `ADCS`, `RADIO`, and `STORAGE` surfaces visible on
  node `5` are reduced to curated keep-live summary rather than broad detail
- on the maintained target path, ambient GPS summary is not required as a
  proof oracle when live UART input is absent or malformed; the guaranteed GPS
  operator closure is fresh `GPS_GET_STATE`
- representative detailed channels remain absent from ambient live visibility
  until explicit authenticated `GET_*`
- representative authenticated `GET_*` commands return one bounded detailed
  readback
- authenticated cached-truth `GET_RESET_CAUSE` readback remains available
- explicit switch away from S-band primary closes old node-`5` live visibility
- target proof restores S-band baseline before cleanup completes

## Fresh Evidence

Hosted observability tier-selection proof:

- Command: `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- Verdict: `PASS`
- Probe root: `/tmp/sband-observability-governance-hosted.8GW9Cz`
- Summary markers:
  - `case-pre-auth-sband-live-quiet=PASS`
  - `case-post-auth-sband-live-open=PASS`
  - `case-post-auth-summary-live-curated=PASS`
  - `case-eps-get-bounded-detailed-readback=PASS`
  - `case-gps-get-bounded-detailed-readback=PASS`
  - `case-adcs-get-bounded-detailed-readback=PASS`
  - `case-radio-get-bounded-detailed-readback=PASS`
  - `case-storage-get-bounded-detailed-readback=PASS`
  - `case-authenticated-get-reset-cause-summary-readback=PASS`
  - `case-primary-switch-closes-sband-live-observability=PASS`
- Key artifacts:
  - events: `/tmp/sband-observability-governance-hosted.8GW9Cz/combined-stack/sband-ground/logs/events.log`
  - channels: `/tmp/sband-observability-governance-hosted.8GW9Cz/combined-stack/sband-ground/logs/channels.log`
  - OBC log: `/tmp/sband-observability-governance-hosted.8GW9Cz/combined-stack/logs/obc.log`
  - S-band capture: `/tmp/sband-observability-governance-hosted.8GW9Cz/combined-stack/captures/sband-southbound-to-gds.bin`

Target installed-release refresh:

- Rebuilt remote workspace with `bash scripts/bootstrap_rpi_workspace.sh`
- Packaged current branch bundle with `bash scripts/package_rpi_bundle.sh`
- Installed `build-artifacts/packages/rpi/v0.1.0-197-gc99119e5d-dirty/obc-rpi-v0.1.0-197-gc99119e5d-dirty.tar.gz`
- Result: `$OBC_HOME/obc-deploy/current` now points at the branch build used
  by the final target proof

Target observability tier-selection proof:

- Command: `bash scripts/run_target_sband_observability_governance_probe.sh`
- Verdict: `PASS`
- Probe root: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.M8wuJ6`
- Summary markers:
  - `case-installed-release-keystore-provenance=PASS`
  - `case-pre-auth-sband-live-quiet=PASS`
  - `case-post-auth-sband-live-open=PASS`
  - `case-post-auth-summary-live-curated=PASS`
  - `case-eps-get-bounded-detailed-readback=PASS`
  - `case-gps-get-bounded-detailed-readback=PASS`
  - `case-adcs-get-bounded-detailed-readback=PASS`
  - `case-radio-get-bounded-detailed-readback=PASS`
  - `case-storage-get-bounded-detailed-readback=PASS`
  - `case-authenticated-get-reset-cause-summary-readback=PASS`
  - `case-primary-switch-closes-sband-live-observability=PASS`
  - `case-restore-sband-primary=PASS`
- Key artifacts:
  - summary: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.M8wuJ6/diagnostics/target-sband-observability-governance-summary.json`
  - checkpoints: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.M8wuJ6/diagnostics/checkpoints.jsonl`
  - OBC journal: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.M8wuJ6/diagnostics/journal-snapshots/target-sband-observability-governance-obc.log`
  - S-band service journal: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.M8wuJ6/diagnostics/journal-snapshots/target-sband-observability-governance-sband-service.log`

Target probe note:

- The target proof temporarily sets `OBCApp.gpsBridge.GPS_SET_SOURCE_MODE FAKE`
  before `GPS_GET_STATE`, then restores `LIVE_UART`.
- This is a probe-owned determinism control so the bounded detailed GPS
  readback is proven even when live UART input is absent at that moment.
- A failed closeout rerun exposed that the maintained target path does not
  guarantee an ambient post-auth GPS summary sample on demand when live UART
  input is absent or malformed, so the target oracle was tightened before the
  final `M8wuJ6` PASS run.
- The bounded target claim for this change is therefore narrower than hosted:
  GPS summary channels still define the family keep-live surface when
  scheduled polls produce parseable data, but the guaranteed target operator
  closure is fresh `GPS_GET_STATE`.

Target secure-auth control regression:

- Command: `bash scripts/run_target_secure_auth_command_path_probe.sh`
- Verdict: `PASS`
- Probe root: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.MKoD8q`
- Summary markers:
  - `case-installed-release-keystore-provenance=PASS`
  - `case-sband-apid-00fe-secure-auth=PASS`
  - `case-sband-secure-command-get-reset-cause-sequence=41`
  - `sband-secure-command-source=target-journal`
- Key artifact:
  - summary: `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.MKoD8q/diagnostics/secure-auth-command-path-summary.json`

Focused UTs:

- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 8 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 8 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_AdcsBridge_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 7 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RadioController_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 10 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_StorageHealthBridge_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 6 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`
  - verdict: `PASS` (`[  PASSED  ] 86 tests.`)
- `build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`
  - verdict: `PASS`

## Observed Current Truth

The fresh hosted and target proofs now support the following current truth:

- node-`5` S-band still starts quiet until accepted secure auth
- post-auth live visibility remains open only during the authenticated session
- the scheduled family set in this change now behaves as curated keep-live
  summary rather than broad detailed chatter
- detailed observation for `EPS`, `GPS`, `ADCS`, `RADIO`, and `STORAGE`
  remains available through explicit authenticated fresh bounded readback
- cached-truth readback such as `GET_RESET_CAUSE` remains separate from
  fresh observation/readback

## Current Tiering Matrix

The current implementation on this branch uses four descriptive tiers inside
the first node-`5` content-selection slice. These tiers match current
component-owned scheduled-versus-explicit-readback behavior and current
operator-baseline wording; they are not a repo-wide central packet classifier
or a generic `tlm/event` whitelist engine:

### `keep-live summary`

These remain continuously visible on the authenticated node-`5` S-band live
surface during a pass.

- `EPS`
  - `EPS_VBAT`
  - `EPS_SOC`
  - `EPS_TEMP_BAT`
  - `EPS_PDU_STATUS`
  - critical transition/fault events such as `EPS_LOW_BATTERY`,
    `EPS_CRITICAL_BATTERY`, `EPS_OVERTEMP`, `EPS_PDU_CHANGE`,
    `EPS_COMM_ERROR`
- `GPS`
  - `GPS_SOURCE_MODE`
  - `GPS_HAVE_SAMPLE`
  - `GPS_FIX_VALID`
  - `GPS_SAT_COUNT`
  - transition/fault events such as `GPS_FIX_ACQUIRED`, `GPS_FIX_LOST`,
    `GPS_PARSE_ERROR`, `GPS_SOURCE_ERROR`
  - target proof note: these remain the GPS keep-live summary family when
    scheduled polls produce parseable data, but the maintained live-UART target
    path does not guarantee that an ambient GPS summary sample exists at every
    proof moment
- `ADCS`
  - `ADCS_MODE`
  - `ADCS_POINTING_ERR`
  - transition/fault events such as `ADCS_MODE_CHANGE`,
    `ADCS_DETUMBLE_COMPLETE`, `ADCS_POINTING_ACQUIRED`,
    `ADCS_SENSOR_FAULT`, `ADCS_COMM_ERROR`
- `RADIO`
  - `RADIO_ENABLED`
  - `RADIO_STATUS_SAMPLE_AVAILABLE`
  - `RADIO_STATUS_AGE_TICKS`
  - `RADIO_STATUS_RESULT`
  - power/temperature transition events remain component-owned
- `STORAGE`
  - `STORAGE_HAVE_SCAN`
  - `STORAGE_WARNING_ACTIVE`
  - `STORAGE_WARNING_MASK`
  - `STORAGE_DEGRADED_MASK`
  - `STORAGE_DATA_PRODUCTS_QUOTA_BYTES`
  - `STORAGE_DATA_PRODUCTS_QUOTA_STATUS`
  - `STORAGE_DATA_PRODUCTS_RETENTION_STATUS`
  - warning/failure events such as `STORAGE_ROOT_MISSING`,
    `STORAGE_SCAN_FAILED`, `STORAGE_WARNING_THRESHOLD_EXCEEDED`

### `fresh GET-driven bounded readback`

These commands now provide the detailed observation surface instead of relying
on broad ambient live chatter.

- `EPS_GET_STATUS`
  - fresh transport poll
  - detailed telemetry such as `EPS_IBAT`, `EPS_VSOLAR`, `EPS_ISOLAR`,
    `EPS_POWER_OUT`
  - bounded `EPS_STATUS_RECEIVED` closure
- `GPS_GET_STATE`
  - fresh source poll
  - detailed telemetry such as `GPS_LAT_DEG`, `GPS_LON_DEG`, `GPS_ALT_M`,
    `GPS_SPEED_MPS`, `GPS_COURSE_DEG`, `GPS_HDOP`, UTC fields, sentence
    counters
  - bounded `GPS_STATE_UPDATED` closure
- `ADCS_GET_ATTITUDE`
  - fresh transport poll
  - detailed telemetry such as `ADCS_Q*`, `ADCS_OMEGA_*`, `ADCS_MAG_*`
- `RADIO_GET_STATUS`
  - fresh transport poll
  - detailed telemetry such as `RADIO_TX_POWER`, `RADIO_FREQ`, `RADIO_TEMP`,
    `RADIO_RSSI`
- `STORAGE_GET_STATUS`
  - fresh scan
  - detailed telemetry such as `STORAGE_SCAN_COUNT`, `STORAGE_SCAN_ERRORS`,
    per-root file counts/bytes, data-product scan status, watermark bytes
  - bounded `STORAGE_SCAN_UPDATED` closure

### `onboard cached truth`

These remain explicit cached readback surfaces and are not redefined as fresh
external observation.

- `GET_RESET_CAUSE`
- `GET_BOOT_COUNT`
- `GET_PERSISTENT_FAULT_HISTORY`
- `GET_RECOVERY_STATUS`
- `MODE_GET`
- `BOOT_STATUS`

### `non-baseline live`

These runtime surfaces may still exist, but this change does not claim them as
current pass-time operator baseline truth.

- `systemResources.*`
- transport / queue / driver internals
- timing-only support telemetry such as `STORAGE_SCHED_TICKS`
- remaining COMM internal counters and similar support telemetry
- any residual runtime channel or event not explicitly promoted above into
  `keep-live summary`, `fresh GET-driven bounded readback`, or
  `onboard cached truth`

## Non-Claims

This evidence does **not** claim that:

- `systemResources`, transport/queue/driver internals, or COMM internal
  counters are fully removed from runtime output
- every residual runtime surface has already been individually classified,
  filtered, or retired by a central runtime rule
- target GPS live UART availability is guaranteed during every tier-selection
  proof run
- UHF command-paced observability is implemented as part of this change
