# Test Record: mode-model-v2-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-07
- Archived change: `openspec/changes/archive/2026-05-06-mode-model-v2/`

## Scope

This record covers the primary mode model v2 replacement baseline:

- `SatMode` primary modes are `SAFE = 0`, `IDLE = 1`, `HELL = 2`, `PAYLOAD = 3`, and `TTC = 4`.
- `ModeManager.MODE_SET`, `SYS_MODE` telemetry, `SYS_MODE_CHANGE` events, runtime CLI parsing/help/status, onboard-state snapshots, reduced state, live beacon mode encode/decode, and HK trend mode records use the v2 enum.
- Active topology no longer instantiates, schedules, or runtime-binds the provisional `MissionExecutive` low-power interface.
- `BeaconV1` wire size is unchanged and its schema version is bumped to `2` so pre-v2 mode bytes are not silently reinterpreted.
- `HkTrendRecord` product name/id remain stable and the payload schema advances to `HkTrendRecordV3` / `version = 3` so pre-mode-model-v2 V2 `.fdp` files are not same-version compatible.

## Out Of Scope

- COMM, CCSDS, storage-policy behavior, storage-health behavior, or file/downlink changes.
- FDIR, MissionExecutive/autonomy threshold policy, HELL/SAFE/IDLE hysteresis, and load-shedding behavior.
- UHF/S-band, RF, real radio, target hardware, reliable transfer, and pass scheduler behavior.
- Deleting historical evidence that described the pre-v2 `LOW_POWER` autonomy baseline.

## Commands

Formal change validation:

```bash
openspec validate mode-model-v2
openspec validate --specs
```

Fresh repository gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-model-v2
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-model-v2-review-fixes
```

Focused hosted mode probe:

```bash
bash scripts/run_mode_model_v2_hosted_probe.sh
bash scripts/run_onboard_state_data_hosted_probe.sh
```

Archive and post-archive governance:

```bash
openspec archive mode-model-v2 --yes
openspec validate --specs
python3 scripts/check_repo_consistency.py
python3 scripts/check_component_test_baseline.py
```

## Evidence

Repository verification CI:

```text
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-model-v2:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-model-v2-review-fixes:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
```

OpenSpec:

```text
openspec validate mode-model-v2: PASS
openspec validate --specs: PASS, 23 passed, 0 failed
openspec archive mode-model-v2 --yes: PASS, archived as 2026-05-06-mode-model-v2
post-archive openspec validate --specs: PASS, 23 passed, 0 failed
post-archive python3 scripts/check_repo_consistency.py: PASS, 75 archived changes checked
post-archive python3 scripts/check_component_test_baseline.py: PASS, 18 real components and 7 helper/support modules checked
```

Focused hosted probe:

```text
mode_model_v2_hosted_probe: PASS log=/tmp/mode-model-v2-hosted.10frZE/obc-mode-smoke.log
onboard-state-data-fdp-parity-v1-hosted-probe: PASS
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0x397104af
beacon-decode=PASS version=2 mode=SAFE json=/tmp/onboard-state-data-hosted.8wLXxQ/beacon-decoded.json
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778101702_00027034.fdp
fdp-received-byte-match=PASS source=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778101702_00027034.fdp received=/tmp/onboard-state-data-hosted.8wLXxQ/gds-downlink/fprime-downlink/_tmp_onboard-state-data-runtime-hosted_data-products_Dp_268693505_1778101702_00027034.fdp
fdp-decode=PASS version=3 json=/tmp/onboard-state-data-hosted.8wLXxQ/fdp-decode/_tmp_onboard-state-data-runtime-hosted_data-products_Dp_268693505_1778101702_00027034.json
```

## Notes

- The hosted probe starts the native `OBC` binary with `--ground-link disabled`, exercises `mode safe`, `mode idle`, `mode hell`, `mode payload`, and `mode ttc`, verifies runtime status/event observation for all v2 modes, and verifies retired `mode nominal`, `mode low-power`, `mode debug`, and `mode update` are rejected.
- Unit and aggregate repository tests cover the data surfaces not directly decoded by the hosted CLI probe, including beacon schema-version rejection, beacon valid/invalid mode decode behavior, and HK trend mode/version records.
- Historical `LOW_POWER` evidence remains in the repository as historical evidence only; the active v2 baseline replaces that primary mode claim.
- `openspec archive` emitted proposal-quality warnings about delta count and proposal requirement formatting, but task status was complete, spec sync succeeded, and post-archive validations passed.

## Review Follow-Up

Code review found that reusing beacon version `1` and HK trend payload version `2` would allow pre-mode-model-v2 artifacts to decode with new mode semantics. The follow-up keeps beacon size and HK product record name/id stable, but advances:

- beacon schema version to `2`, with decoder rejection for schema version `1`
- HK trend payload type/version to `HkTrendRecordV3` / `version = 3`
- hosted mode probe coverage to reject `nominal`, `low-power`, `debug`, and `update`

Additional PR review comments were addressed by simplifying contiguous v2 mode validation to a bounded range check and making the all-mode beacon test iterate the same contiguous enum range.

Additional verification:

```text
scripts/run_verification_ci.sh build-artifacts/verification-ci-mode-model-v2-review-comments:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
```
