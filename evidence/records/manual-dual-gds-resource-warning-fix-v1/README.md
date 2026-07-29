# manual-dual-gds-resource-warning-fix-v1 Evidence

Status: branch-owned verification record for the hosted current-RSS fix and
resource-warning de-spam behavior.

## Summary

This record captures the 2026-06-19 verification run for
`fix/manual-dual-gds-resource-warning` after correcting hosted resident-memory
sampling and changing `WatchdogSupervisor` resource warnings to
threshold-crossing behavior.

The scope is intentionally bounded:

- prove the hosted headless manual dual-GDS path no longer reports false
  `SYS_LOW_MEMORY` from historical peak RSS growth
- confirm `SYS_MEM_RSS_MB` now stays in a live tens-of-megabytes range rather
  than jumping above the `256 MB` threshold during normal hosted use
- rerun a bounded target manual dual-GDS auth plus command check without
  claiming target redeployment

## Code Verification

Fresh local gate:

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
  - gate covered generate/build, UT generate/build, `fprime-util check --all`,
    repo consistency, documentation governance, component-test baseline, legacy
    ZMQ retirement, and `openspec validate --specs`

Fresh focused test results from `build-fprime-automatic-native-ut`:

- `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`
  - result: `PASS`
  - added coverage proves resident memory can rise and fall around a mapped
    allocation
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_WatchdogSupervisor_ut_exe`
  - result: `9 tests passed`
  - added coverage proves resource warnings fire once on threshold enter, stay
    quiet while the sample remains above threshold, fire again only after a
    later re-crossing, and still report correctly after monitoring is
    re-enabled from a disabled state

## Hosted Manual Dual-GDS Verification

Fresh hosted verification surface:

- owner root:
  `/tmp/manual-dual-gds/hosted-rss-fix-closeout`
- runtime root:
  `/tmp/manual-dual-gds/runtime-rss-fix-closeout/combined`
- manifest:
  `/tmp/manual-dual-gds/hosted-rss-fix-closeout/manifest.json`

Observed steps:

1. Auth established on S-band via `manual_secure_ops.py`.
2. Sent one normal secure command:
   `OBCApp.modeManager.MODE_GET`.
3. Observed `SYS_MEM_RSS_MB` through the S-band headless GDS surface:
   - `2026-06-19T09:39:43`: `33.859375 MB`
   - `2026-06-19T09:39:48`: `36.515625 MB`
4. Observed current local process RSS for the hosted `OBC` process:
   - `ps` sample: `41904 KB`
5. Searched the fresh hosted `obc.log` for
   `SYS_LOW_MEMORY`, `SECURE_AUTH_ESTABLISHED`, and command completion.

Results:

- hosted `obc.log` showed `SECURE_AUTH_ESTABLISHED` and `MODE_GET`
  dispatch/completion
- hosted `obc.log` showed **no** `SYS_LOW_MEMORY`
- hosted `SYS_MEM_RSS_MB` remained in a live tens-of-megabytes range and did
  not recreate the earlier false `256+ MB` climb caused by `ru_maxrss`
- the corrected telemetry did not numerically match `ps` exactly, but both
  stayed far below the `256 MB` threshold and no longer diverged into the
  earlier `321.9375 MB` false-warning regime

Fresh hosted log checked:

- `/tmp/manual-dual-gds/hosted-rss-fix-closeout/stack/logs/obc.log`

## Target Manual Dual-GDS Spot Check

Bounded target spot-check surface reused the existing prepared baseline:

- baseline root:
  `/tmp/manual-dual-gds/target-baseline-closeout`
- ground root:
  `/tmp/manual-dual-gds/target-ground-closeout`

Observed steps:

1. Cleared any stored local secure state for target S-band.
2. Re-established target S-band auth.
3. Sent one normal secure command:
   `OBCApp.modeManager.MODE_GET`.
4. Observed target `SYS_MEM_RSS_MB` through the target S-band headless GDS
   surface with a bounded wait.
5. Observed `SYS_LOW_MEMORY` events through the same surface with a bounded
   wait.

Results:

- target auth succeeded in this rerun
- target command send succeeded
- target `SYS_MEM_RSS_MB` observed:
  - `2026-06-19T09:44:24`: `8.83203125 MB`
- target `SYS_LOW_MEMORY` observed:
  - none during the bounded wait window

Scope note:

- this target check did **not** redeploy the remote installed OBC release
- it serves only as a regression check for the maintained helper / observation
  flow, not as proof that the target binary itself now contains the hosted RSS
  sampling change

## Residual Notes

- The earlier exploratory session saw transient target auth instability, but
  that instability was not needed to validate this resource-warning fix and is
  intentionally kept out of scope here.
- Threshold policy remains unchanged at `256 MB`. A follow-on threshold change
  is only justified if corrected current-RSS sampling still crosses that value
  during normal hosted or deployed operation.
