## Context

The repo now has a clean node-`5` S-band access-gating baseline: pre-auth
quiet, post-auth live visibility, and close on session loss or primary-link
switch. What remains broad is the content inside the auth-gated live stream.
Current scheduled refresh paths emit:

- `EPS`: full battery/solar/PDU telemetry plus `EPS_STATUS_RECEIVED`
- `GPS`: full fix/navigation telemetry plus `GPS_STATE_UPDATED`
- `ADCS`: full quaternion/rate/magnetometer telemetry
- `RADIO`: full raw radio telemetry on every successful poll
- `STORAGE`: full storage-root telemetry plus `STORAGE_SCAN_UPDATED`

Those families are all useful, but not at the same cadence and not all as
continuous pass-time live surfaces. The repo already contains the better
operator primitive for many of them: fresh command-triggered status commands
that can return a bounded readback instead of requiring ambient background
telemetry.

## Design

### Current tier model

This change fixes the current content model to four tiers:

1. `keep-live summary`
   - post-auth fields or events operators need to watch continuously
   - critical transition or fault events
2. `fresh GET-driven bounded readback`
   - explicit command-triggered fresh status/readback
3. `onboard cached truth`
   - current onboard state that is not conceptually a fresh external sample
4. `non-baseline live`
   - runtime surfaces that may still exist but are not current operator
     baseline truth

### Readback semantics

This change fixes one repo-level distinction:

- `onboard cached truth` commands may remain cached by design
- external observation or scan commands are `fresh-by-default`

That means:

- `MODE_GET`, `BOOT_STATUS`, `GET_RESET_CAUSE`, `GET_BOOT_COUNT`,
  `GET_PERSISTENT_FAULT_HISTORY`, and similar commands remain cached/current
  onboard truth
- `EPS_GET_STATUS`, `GPS_GET_STATE`, `ADCS_GET_ATTITUDE`, `RADIO_GET_STATUS`,
  and `STORAGE_GET_STATUS` are treated as fresh observation/readback commands

### Component ownership

`CommController` and `CommEgressMux` do not gain content filtering
responsibility. Each family owns its own split between:

- scheduled summary publication
- explicit GET/readback detail publication

This keeps the implementation local to the source component and avoids
inventing a new summary router or generic telemetry schema.

### Family decisions

#### EPS

- scheduled path keeps summary channels only:
  - `EPS_VBAT`
  - `EPS_SOC`
  - `EPS_TEMP_BAT`
  - `EPS_PDU_STATUS`
- scheduled path keeps threshold and transition events:
  - `EPS_LOW_BATTERY`
  - `EPS_CRITICAL_BATTERY`
  - `EPS_OVERTEMP`
  - `EPS_PDU_CHANGE`
  - `EPS_COMM_ERROR`
- scheduled path stops using `EPS_STATUS_RECEIVED` as broad live baseline
- explicit `EPS_GET_STATUS` and control commands still publish detailed EPS
  telemetry and may emit `EPS_STATUS_RECEIVED` as bounded readback closure

#### GPS

- scheduled path keeps summary telemetry only:
  - `GPS_SOURCE_MODE`
  - `GPS_HAVE_SAMPLE`
  - `GPS_FIX_VALID`
  - `GPS_SAT_COUNT`
- these summary channels remain the GPS keep-live family when scheduled polls
  produce parseable samples; the maintained live-UART target path does not
  assume an ambient sample exists at every proof moment
- scheduled path keeps fix/fault events:
  - `GPS_FIX_ACQUIRED`
  - `GPS_FIX_LOST`
  - `GPS_PARSE_ERROR`
  - `GPS_SOURCE_ERROR`
- scheduled path stops using `GPS_STATE_UPDATED` as broad live baseline
- `GPS_GET_STATE` still performs a fresh poll and emits full detailed telemetry
  plus bounded update closure

#### ADCS

- scheduled path keeps summary telemetry only:
  - `ADCS_MODE`
  - `ADCS_POINTING_ERR`
- scheduled path keeps existing mode/fault/transition events:
  - `ADCS_MODE_CHANGE`
  - `ADCS_DETUMBLE_COMPLETE`
  - `ADCS_POINTING_ACQUIRED`
  - `ADCS_SENSOR_FAULT`
  - `ADCS_COMM_ERROR`
- `ADCS_GET_ATTITUDE`, `ADCS_SET_MODE`, `ADCS_SET_TARGET`, and
  `ADCS_CALIBRATE` still publish full detailed telemetry

#### RADIO

- scheduled path keeps summary telemetry only:
  - `RADIO_ENABLED`
  - `RADIO_STATUS_SAMPLE_AVAILABLE`
  - `RADIO_STATUS_AGE_TICKS`
  - `RADIO_STATUS_RESULT`
- `RADIO_GET_STATUS` and radio setter commands use a unified apply/readback
  helper that publishes full detailed radio telemetry on explicit readback
- raw radio fields remain `RadioController`-owned cached raw observation and
  do not become `COMM` or provider policy input

#### STORAGE

- scheduled scan keeps cache freshness and reduced summary telemetry only:
  - `STORAGE_HAVE_SCAN`
  - `STORAGE_WARNING_ACTIVE`
  - `STORAGE_WARNING_MASK`
  - `STORAGE_DEGRADED_MASK`
  - `STORAGE_DATA_PRODUCTS_QUOTA_STATUS`
  - `STORAGE_DATA_PRODUCTS_RETENTION_STATUS`
- scheduled scan keeps root/failure threshold events:
  - `STORAGE_ROOT_MISSING`
  - `STORAGE_SCAN_FAILED`
  - `STORAGE_WARNING_THRESHOLD_EXCEEDED`
- scheduled path stops using `STORAGE_SCAN_UPDATED` as broad live baseline
- `STORAGE_GET_STATUS` becomes the fresh operator readback command and
  publishes the full detailed storage-root telemetry set
- `STORAGE_SCAN_NOW` is retired because it becomes redundant once
  `STORAGE_GET_STATUS` is fresh-by-default

### Maintained proof and readiness adjustments

Any maintained probe or readiness check that currently infers system health
from broad scheduled `EPS_STATUS_RECEIVED` or `GPS_STATE_UPDATED` chatter must
be updated. Those markers are no longer valid baseline oracles once the
scheduled path is intentionally quieter.

The new maintained node-`5` hosted and target probes must prove:

- post-auth live summary still appears
- representative detailed channels remain absent from ambient live visibility
  until explicit GET/readback
- representative GET/readback commands surface fresh detailed values

## Risks And Mitigations

- Risk: scheduled telemetry reduction silently breaks repo-owned readiness or
  timing scripts that were relying on broad chatter.
  Mitigation: update shared readiness/oracle scripts in the same change and
  document any residual `non-baseline live` surfaces explicitly.

- Risk: removing `STORAGE_SCAN_NOW` breaks external habits.
  Mitigation: treat it as an explicit public command-surface retirement in the
  artifacts, docs, authority catalog/policy, and closeout.

- Risk: detailed GET/readback becomes stale or misleading.
  Mitigation: use fresh sample/scan for external observation commands and keep
  cached-only semantics explicit for onboard-truth commands.
