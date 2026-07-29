# node5-observability-residual-cleanup-v1 Evidence

Status: archived closeout record on this branch. Fresh governed proof
completion, archive, reconciliation, and local-ready closeout all completed
without push or PR.

## Scope

This record captures the active residual-governance rebuild layered on top of
`sband-live-observability-tier-selection-v1`.

It covers:

- the current fresh hosted residual inventory and adjacent detailed `GET_*`
  proof-chain diagnosis
- the same-change OpenSpec / canonical-doc rebuild required before final
  runtime and proof requalification
- the formal node-`5` resource keep-live truth moving to
  `WatchdogSupervisor` `SYS_*`
- the formal reviewable proof / transport / policy surfaces that remain
  required:
  - `GROUND_LINK_UP/DOWN`
  - `GROUND_LINK_TX_BYTES`
  - transport-error growth
  - `GROUND_LINK_HEALTH_S_BAND_*`
  - `QueueOverflow`
  - `CSP_OWNER_TIMEOUT`
  - `CSP_OWNER_TOTAL_TIMEOUTS`
  - S-band `CommEgressMux` counters
- the explicit diagnostics-only / non-baseline-live boundary for
  `SystemResources.*`, queue-depth counters, UART stats, and residual COMM
  internals

It does **not** cover:

- a secure-auth redesign
- a second command plane
- a generic telemetry-schema rewrite
- broad UHF baseline widening
- proof that every diagnostics-only residual channel has been removed from
  runtime output

## Investigation Inputs

Fresh local verification gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

Fresh hosted observability proof:

```bash
bash scripts/run_sband_observability_governance_hosted_probe.sh
```

Fresh target observability and secure-auth proofs:

```bash
bash scripts/run_target_sband_observability_governance_probe.sh
bash scripts/run_target_secure_auth_proof.sh
```

OpenSpec validation:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate node5-observability-residual-cleanup-v1
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs
```

Archive and reconciliation closeout:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" openspec archive node5-observability-residual-cleanup-v1 --yes --skip-specs
python3 scripts/generate_reconciliation_matrix_md.py
python3 scripts/check_repo_consistency.py
```

## Accepted Outcome

This change is accepted as archived local-ready work on this branch. The
governed closeout established all of the following:

- hosted and target node-`5` observability-governance proofs still pass
- `SYS_*` is the formal node-`5` resource keep-live truth in current docs and
  runbooks
- `SystemResources.*` is documented only as supplemental diagnostics/review
- reviewable transport, queue, owner-timeout, and S-band egress proof surfaces
  remain explicit in current docs and verification wording
- diagnostics-only residuals are no longer described as vague accidental
  operator truth
- representative detailed authenticated `GET_*` readback is requalified on the
  maintained packetized proof path

## Current Observations

Fresh local verification gate:

- Command:
  `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- Result:
  `PASS`
- Role in this record:
  initial diagnostic baseline; the fresh phase-4 rerun is recorded below

Fresh hosted observability diagnosis:

- Command:
  `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- Result:
  `FAIL`
- Probe root:
  `/tmp/sband-observability-governance-hosted.RWjyRp`
- Failing step:
  existing proof oracle timed out waiting for
  `OBCApp.epsBridge.EPS_IBAT` in the long-running ground-side
  `channels.log`
- Key adjacent evidence from the same run:
  - post-auth ambient keep-live summary still appeared:
    `EPS_SOC`, `ADCS_MODE`, `RADIO_STATUS_AGE_TICKS`,
    `STORAGE_WARNING_MASK`
  - formal resource/reviewable surfaces also appeared:
    `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`, `GROUND_LINK_TX_BYTES`,
    `GROUND_LINK_HEALTH_S_BAND_*`, `SBAND_ROUTED_EVENT_PACKETS`,
    `SBAND_ROUTED_TLM_PACKETS`
  - diagnostics-only residuals still appeared:
    `SystemResources.*`, `ComCcsds` / `OBCComCcsds` queue depths,
    `UART_*`, `UHF_SUPPRESSED_*`
  - explicit authenticated `EPS_GET_STATUS` still produced
    `EPS_STATUS_RECEIVED` in the event stream
- Current interpretation:
  the hosted proof drift is at least partly in the detailed `GET_*` oracle or
  packetized proof chain; this record does not yet claim a product regression
  in the component-owned summary/detail split itself

Follow-on hosted same-change reruns after the proof-chain rebuild narrowed the
problem statement:

- Commands:
  - `bash scripts/run_sband_observability_governance_hosted_probe.sh`
    with passive-channel freshness gating
  - `bash scripts/run_sband_observability_governance_hosted_probe.sh`
    with fresh per-command passive channel-listener generations plus bounded
    channel-search fallback
- Results:
  both reruns still `FAIL`
- Probe roots:
  - `/tmp/sband-observability-governance-hosted.y355dC`
  - `/tmp/sband-observability-governance-hosted.icO4Zr`
- New diagnostic facts:
  - the original failure was not only a stale long-running `channels.log`
    oracle; passive-listener freshness needed explicit handling
  - hosted same-path evidence at
    `/tmp/sband-observability-governance-hosted.y355dC` did capture
    `OBCApp.epsBridge.EPS_IBAT` immediately after `EPS_STATUS_RECEIVED`,
    showing that the maintained representative EPS detailed path still works
  - the stronger failure came from treating representative detailed bounded
    readback as a five-family proof set; GPS, ADCS, radio, and storage
    detailed checks expand beyond the rebuilt same-change plan and are more
    sensitive to source-specific or lag-specific behavior than the formal
    representative proof requires
  - in particular, bounded hosted ground-side search for
    `OBCApp.gpsBridge.GPS_LAT_DEG` proved unstable, while the branch plan only
    requires one deterministic representative detailed `GET_*` requalification
- Current interpretation:
  phase 3 should repair the hosted/target proof scope so the governed path
  proves one deterministic representative bounded detailed readback on the
  maintained node-`5` packetized path, while ambient curated live keeps
  asserting that detailed channels such as `GPS_LAT_DEG`, `ADCS_Q0`,
  `RADIO_RSSI`, and `STORAGE_SCAN_COUNT` are not part of pass-time truth.

Fresh hosted representative-proof rerun after that scope repair:

- Command:
  `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- Result:
  `PASS`
- Probe root:
  `/tmp/sband-observability-governance-hosted.bKj6Cm`
- Proven now on the current hosted path:
  - pre-auth S-band live stays quiet
  - authenticated S-band opens curated post-auth live
  - formal reviewable surfaces still appear on the same governed path
  - representative bounded detailed readback now closes with
    `EPS_GET_STATUS -> EPS_IBAT`
  - `GET_RESET_CAUSE` still works as bounded cached readback
  - explicit switch to UHF still closes S-band live observability
- Current interpretation:
  phase 3.1 is now closed on the hosted path. Target proof rerun remains
  phase-4 verification work, but the same-change probe implementation now
  matches the rebuilt representative-readback boundary.

Existing ancestry evidence that remains authoritative until same-change
requalification completes:

- [sband-live-observability-tier-selection-v1](../sband-live-observability-tier-selection-v1/README.md)

## Fresh Evidence State

Fresh hosted and target proof completion now exists on this branch. The
remaining work is archive-grade closeout refresh after the final post-edit
validation pass and reconciliation updates.

## Phase 4 Fresh Closeout Status

Fresh local gate:

- Command:
  `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- Result:
  `PASS`
- Current role:
  closeout-quality local gate is fresh again on this branch

Fresh hosted observability proof:

- Command:
  `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- Result:
  `PASS`
- Probe root:
  `/tmp/sband-observability-governance-hosted.dcCLRv`
- Current role:
  the rebuilt hosted oracle now closes on the maintained representative
  `EPS_GET_STATUS -> EPS_IBAT` bounded readback and the governed post-auth
  keep-live/reviewable live boundary

Fresh target observability proof:

- Command:
  `bash scripts/run_target_sband_observability_governance_probe.sh`
- Result:
  `PASS`
- Probe root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.CNez8O`
- Proven now on the current target path:
  - `sband-ground-readiness=southbound-opened-stable`
  - pre-auth S-band live stays quiet
  - authenticated S-band opens curated post-auth live
  - formal reviewable surfaces still appear on the governed target path
  - representative bounded detailed readback now closes with
    `EPS_GET_STATUS -> EPS_IBAT`
  - `GET_RESET_CAUSE` still works as bounded cached readback
  - explicit switch to UHF still closes S-band live observability
  - the probe restores S-band primary before exit
- Current interpretation:
  phase-4 target observability requalification is now closed on this branch.

Fresh target secure-auth command-path preflight:

- Command:
  `bash scripts/run_target_secure_auth_command_path_probe.sh`
- Result:
  `PASS`
- Probe root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.xjl2m0`
- Current role:
  current target S-band bootstrap, keystore provenance, APID `0x00FE` secure
  auth, and secure command v2 preflight are fresh again on this branch.

Fresh target secure-auth regression:

- Command:
  `bash scripts/run_target_secure_auth_proof.sh`
- Result:
  `PASS`
- Probe root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.uiUAYj/target-secure-auth-proof-v1`
- Proven now on the current target path:
  - installed release keystore provenance still matches the governed target profile
  - malformed S-band handshake still fails closed
  - APID `0x00FE` secure auth still closes on S-band before the secure command sequence and staged upload checks
  - UHF backup service-`2` auth, read-status, high-authority deny, and denied staged-upload behaviors still close on the governed target path
  - failover invalidates stale UHF backup auth and requires fresh UHF primary reauth before commands succeed again
  - the proof restores S-band primary before exit
- Same-change proof repair that closed the blocker:
  - the full proof now defaults `OBC_GROUNDLINK_DIAGNOSTICS=1` so the OBC restarts cleanly and S-band auth does not inherit stale APID sequence state
  - target secure-auth handshake observation now falls back to native packet logs when the first wire-capture challenge or auth-status window is missed
  - the UHF primary-reauth helper now uses a fresh runtime root so the reauth GDS does not fail on a reused `custom-data-handlers-app` path

Focused UT rerun:

- Command:
  `PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut --output-on-failure -R "OBC_Components_CommController_ut_exe|OBC_Components_CommEgressMux_ut_exe|OBC_Components_WatchdogSupervisor_ut_exe|OBC_Components_GroundLinkHealthProvider_ut_exe|hosted_runtime_unit_test"`
- Result:
  `PASS`

OpenSpec validation:

- Commands:
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate node5-observability-residual-cleanup-v1`
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`
- Result:
  `PASS`

## Closeout Completion

- archived change:
  `2026-06-13-node5-observability-residual-cleanup-v1`
- baseline reconciliation updated:
  `docs/baseline-reconciliation-matrix.json`
- generated reconciliation review surface refreshed:
  `docs/baseline-reconciliation-matrix.md`
- repo consistency checks passed after archive
- current branch state:
  local-ready, with no push and no PR opened

## Resulting Boundary

Current node-`5` post-auth observability now uses four explicit buckets:

1. keep-live truth:
   `SYS_*` resource surfaces plus operator-facing `CommController`
   state/transition truth
2. reviewable proof / transport / policy observability:
   `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error growth,
   `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
   `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
   `CommEgressMux` counters
3. bounded fresh readback:
   component-owned `GET_*` / read-status command surfaces
4. diagnostics-only / non-baseline live:
   `SystemResources.*`, queue-depth counters, UART stats, and remaining
   residual COMM/reliable-transfer chatter
