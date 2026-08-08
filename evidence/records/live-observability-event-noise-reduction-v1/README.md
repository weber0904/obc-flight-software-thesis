# live-observability-event-noise-reduction-v1 Evidence

Status: fresh local-ready, hosted manual-surface, and service-managed target
evidence rerun on this branch on 2026-06-07.

## Scope

This record captures the event-surface noise-reduction follow-up implemented by
`live-observability-event-noise-reduction-v1`.

It covers:

- `STATE_MONITOR_UPDATED` changing from a periodic activity event to a
  reduced-state mask-transition event
- healthy `CSP_PING_RESULT` success no longer appearing on the packetized
  ground event surface
- `HK_TREND_PRODUCT_WRITTEN` remaining on the packetized ground event surface
- `OBCApp.dpWriter.FileWritten` remaining visible in local text/journal output
  while being filtered from the packetized ground event surface
- `RateGroupCycleSlip` remaining visible on the packetized ground event surface
- hosted shared runtime `logs/` root creation so
  `STORAGE_ROOT_MISSING ... LOGS` no longer appears during manual dual-GDS
  bring-up

It does **not** cover:

- any new verification-path claim
- changes to auth policy, command authority, file/sequence authority, or band
  switching semantics
- elimination of real `RateGroupCycleSlip` warnings
- target RF behavior, reliable transfer, or non-governed UHF redesign

## Commands

Fresh local-ready gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

OpenSpec validation:

```bash
openspec validate live-observability-event-noise-reduction-v1
openspec validate --specs
```

Hosted manual surface bring-up:

```bash
GDS_UI_MODE=headless MANUAL_HOSTED_AUTO_PORTS=1 \
bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

Target refresh and manual surface bring-up:

```bash
bash scripts/bootstrap_rpi_workspace.sh
bash scripts/package_rpi_bundle.sh
FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh \
  build-artifacts/packages/rpi/v0.1.0-200-gf40d3e369-dirty/obc-rpi-v0.1.0-200-gf40d3e369-dirty.tar.gz

bash scripts/manual_ops/target/start_target_manual_baseline.sh
GDS_UI_MODE=headless MANUAL_TARGET_GROUND_AUTO_PORTS=1 \
bash scripts/manual_ops/target/start_target_manual_ground_surface.sh
```

Target S-band secure auth:

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /private/tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

## Acceptance

This change is accepted only when all of the following are true:

- `STATE_MONITOR_UPDATED` no longer floods the default hosted/target live event
  surface while masks are stable
- healthy `CSP_PING_RESULT` success is absent from the default hosted/target
  live event surface
- `HK_TREND_PRODUCT_WRITTEN` remains visible on the live event surface
- `RateGroupCycleSlip` remains visible on the live event surface
- `DpWriter.FileWritten` remains visible in local text/journal output
- hosted manual dual-GDS runtime no longer reports `Storage root missing LOGS`

## Fresh Evidence

Fresh local-ready gate:

- Command: `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- Verdict: `PASS`

Focused component and build validation:

- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_OnboardStateMonitor_ut_exe`
  - verdict: `PASS`
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CspBridge_ut_exe`
  - verdict: `PASS`
- `cmake --build build-fprime-automatic-native --target OBC`
  - verdict: `PASS`
- `Svc_EventManager_ut_exe`
  - not materialized in this repo because the maintained framework build policy
    keeps `FPRIME_ENABLE_FRAMEWORK_UTS=OFF`
  - compile of `Svc_EventManager` plus runtime validation below provided the
    closure for the default filtered-ID change

Hosted manual-surface validation:

- Command:
  - `bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh`
  - `GDS_UI_MODE=headless MANUAL_HOSTED_AUTO_PORTS=1 bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh`
- Verdict: `PASS`
- Key artifact:
  - `/private/tmp/manual-dual-gds/hosted/stack/logs/obc.log`
- Observed result:
  - `Storage roots: persistent=... staging=...`
  - no `STORAGE_ROOT_MISSING ... LOGS`

Target installed-release and event-surface validation:

- Refresh commands:
  - `bash scripts/bootstrap_rpi_workspace.sh`
  - `bash scripts/package_rpi_bundle.sh`
  - `FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-200-gf40d3e369-dirty/obc-rpi-v0.1.0-200-gf40d3e369-dirty.tar.gz`
- Installed release root:
  - `$OBC_HOME/obc-deploy/releases/v0.1.0-200-gf40d3e369-dirty`
- Required follow-up:
  - a stale `obc-comm-csp-stack.service` child was still executing the old
    release after install, so `sudo systemctl restart obc-comm-csp-stack.service`
    was required before collecting final evidence
- Final target secure-auth command:
  - `python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest /private/tmp/manual-dual-gds/target-ground/manifest.json auth establish`
- Final verdict: `PASS`
- Ground event observation window after re-auth:
  - total events: `10`
  - `OBCApp.hkTrendProductProducer.HK_TREND_PRODUCT_WRITTEN`: `1`
  - `OBCApp.rateGroup1Comp.RateGroupCycleSlip`: `7`
  - `OBCApp.rateGroup3Comp.RateGroupCycleSlip`: `2`
  - absent:
    - `OBCApp.cspBridge.CSP_PING_RESULT`
    - `OBCApp.onboardStateMonitor.STATE_MONITOR_UPDATED`
    - `OBCApp.dpWriter.FileWritten`
- Local target journal check:
  - `journalctl -u obc-comm-csp-stack.service --since "2026-06-07 22:48:41" --no-pager | grep FileWritten`
  - verdict: `PASS`
  - representative lines remained present at `22:49:12`, `22:49:42`,
    `22:50:13`, `22:50:43`, and `22:51:12`

OpenSpec validation:

- `openspec validate live-observability-event-noise-reduction-v1`
  - verdict: `PASS`
- `openspec validate --specs`
  - verdict: `PASS`
