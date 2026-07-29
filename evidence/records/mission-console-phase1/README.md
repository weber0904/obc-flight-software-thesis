# Mission Console Phase 1 Evidence

Status: archived closeout evidence for `2026-06-20-mission-console-phase1`.  
Date: 2026-06-20.

## Scope

This record captures the closeout state for `Mission Console Phase 1` as
archived in `openspec/changes/archive/2026-06-20-mission-console-phase1/`.

It covers:

- fresh local verification status
- hosted Mission Console end-to-end closure on the maintained hosted manual
  dual-GDS baseline
- bounded target Mission Console parity closure on the maintained target manual
  dual-GDS baseline
- the bounded debugging path used to improve the earlier target blocker
- the current target auth failure snapshot for later rerun comparison:
  - [target-auth-failure-snapshot-2026-06-20.md](../../../evidence/records/mission-console-phase1/target-auth-failure-snapshot-2026-06-20.md)

It does not claim a new reusable verification-path registry entry; this record
is branch/archive evidence for the Mission Console Phase 1 change itself.

## 2026-07-09 Beacon Viewer Current-Note

Mission Console 的 beacon viewer 已在後續 branch-local change 中獨立收斂，
目前 operator-facing current note 與 fresh API evidence 請直接看：

- [evidence/records/mission-console-beacon-viewer-v1/README.md](../mission-console-beacon-viewer-v1/README.md)

這份 archived Phase 1 record 仍是 command/auth/readback/sequence/dashboard
主體 closeout 的來源；它不是現在 beacon viewer 的最新專用證據入口。

## 2026-06-30 UX Uplift v1 Addendum

This is the current quick reference for the active
`feature/mission-console-ux-uplift-v1` branch. Treat it as newer than the
older post-review maintenance bullets below when the question is "what is the
current Mission Console operator surface and proof status on this branch?"

The corresponding OpenSpec change is now archived at:

- `openspec/changes/archive/2026-06-30-mission-console-ux-uplift-v1/`

Current UX delta on top of the archived Phase 1 baseline:

- `/ops` now loads the active context `AppTopologyDictionary.json` and merges a
  curated Mission Console overlay
- `/ops` now provides searchable mission-first command discovery, schema-driven
  argument fields, a `Show all commands` engineering toggle, and an advanced
  raw fallback path
- `/`, `/readback`, `/packet-lab`, and `/surfaces` now render structured
  operator views instead of defaulting to raw JSON blocks
- dashboard now foregrounds a `Mode Overview` card so OBC / subsystem
  mode-state truth is visible before the lower-level telemetry summary cards
- current UI intentionally excludes the older `radioController` surface because
  the maintained node `5/6` link baseline does not use it as an active operator
  surface

Latest review-follow-up fixes that changed operator-visible behavior:

- `/ops` command filtering no longer drops every entry after visibility/group
  checks; the mission-facing command list now renders correctly again
- command catalog `source` now means "explicit curated overlay entry" instead
  of "any grouped command", so dictionary-only commands no longer display a
  misleading curated/source badge

Current verification reruns on this branch head closed as follows after those
follow-up fixes:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `76 tests OK`
- fresh local gate:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
- fresh hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-auth-sband=PASS`
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-payload-status=PASS`
    - `hosted-upload=PASS`
    - `hosted-seq-validate=PASS`
    - `hosted-packet-lab=explicit-reject`
    - `hosted-switch-uhf=PASS`
    - `hosted-sband-invalidated=PASS`
    - `hosted-auth-uhf-primary=PASS`
    - `hosted-uhf-boot-command=PASS`
    - `hosted-history-cache=PASS`
    - `mission-console-hosted: PASS`
- fresh target rerun:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed summary:
    - `target-auth-sband-attempt-1=FAILED`
    - `target-auth-sband-retry=2`
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=explicit-reject`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`

Current operator-facing implications:

- the maintained negative-packet demo now closes on `explicit-reject` for both
  hosted and target reruns; older `bounded-no-op` examples below remain useful
  as historical classifier context, but they are no longer the current quick
  reference expectation
- the current Mission Console manual operator flow is documented in:
  - [`docs/operator/mission-console.md`](../../../docs/operator/mission-console.md)
  - [`docs/operator/mission-console.md`](../../../docs/operator/mission-console.md)

## 2026-07-01 MODE_GET Forced-Resend Validation Addendum

This addendum records the first flight-side proof for the active
`mission-console-observability-bootstrap-v1` follow-up:

- `MODE_GET` now forces a resend on the existing shared telemetry channels
  instead of only updating local flight-side truth and hoping `update on
  change` will emit later
- the validated channel set is:
  - `SYS_MODE`
  - `SYS_UPTIME_SEC`
  - `SYS_REBOOT_COUNT`
- the validated scenario is explicitly:
  - do not send `MODE_SET`
  - send `MODE_GET`
  - still observe the current mode-state values on the ground

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `85 tests OK`
  - new coverage includes byte-offset-safe native log readback searches and
    repeated `MODE_GET` refresh expectations
- fresh hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary still includes:
    - `hosted-readback-mode-get=PASS`
    - `mission-console-hosted: PASS`
- focused hosted manual Mission Console validation:
  - launched a fresh temporary hosted manual dual-GDS surface
  - authenticated on S-band
  - sent `MODE_GET` without any prior `MODE_SET`
  - observed structured readback success with:
    - `SYS_MODE=SAFE`
    - `SYS_UPTIME_SEC=45`
    - `SYS_REBOOT_COUNT=1`
  - observed readback provenance:
    - `family=channel-refresh-based`
    - `channelSource=native-log-late`
  - this confirms the ground path now sees fresh mode-state truth even when the
    value itself did not change

Implementation/debugging note:

- the remaining manual-path bug was not in `ModeManager`; the missing-readback
  symptom came from Mission Console native log searches slicing decoded text by
  byte offsets
- once native channel/event/completion searches were changed to read log suffix
  bytes first and decode afterwards, the hosted manual path and the repo-owned
  hosted probe both closed again

## 2026-07-01 EPS_GET_STATUS And Hosted Tick-Rate Addendum

This addendum records the current branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
same forced-resend model into `EpsBridge`.

Flight-side closure on this branch head:

- `EpsBridge` now uses a three-layer operator-facing telemetry model:
  - continuous:
    - `EPS_VBAT`
    - `EPS_IBAT`
    - `EPS_SOC`
    - `EPS_TEMP_BAT`
  - change-driven plus explicit resend:
    - `EPS_PDU_STATUS`
    - `EPS_HEATER_ENABLED`
    - `EPS_OVERCURRENT_FLAGS`
  - explicit refresh only:
    - `EPS_VSOLAR`
    - `EPS_ISOLAR`
    - `EPS_POWER_OUT`
- `EPS_GET_STATUS` now forces a resend of the operator-facing EPS status set
  even when the current values did not change
- successful `EPS_SET_PDU`, `EPS_SET_HEATER`, and `EPS_RESET` now reuse the
  same explicit refresh helper

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `87 tests OK`
- focused EpsBridge unit coverage:
  - `fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target OBC_Components_EpsBridge_ut_exe -j4`
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^OBC_Components_EpsBridge_ut_exe$' --output-on-failure`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-payload-status=PASS`
    - `hosted-readback-eps-status=PASS`
    - `mission-console-hosted: PASS`
  - hosted probe root:
    - `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/mission-console-hosted.U973UU`

Hosted debugging conclusion from this rerun:

- the initial hosted failure was not an `EPS_GET_STATUS` parser bug and not a
  secure-auth regression
- the actual blocker was that the hosted manual surface had started the shared
  hosted OBC runtime with `tick_ms=250` instead of the maintained baseline
  `tick_ms=1000`
- with `tick_ms=250`, background telemetry volume rose enough to drive:
  - `ComCcsds.comQueue.comQueueDepth` steadily upward
  - `ComCcsds.comQueue.QueueOverflow`
  - false readback timeouts that looked like Mission Console logic failures
- after restoring the shared runtime to `tick_ms=1000`, the hosted rerun:
  - kept queue depth in low single digits
  - no longer emitted `QueueOverflow`
  - closed `hosted-readback-eps-status=PASS`

## 2026-07-01 EPS Operator Cadence Default Uplift Addendum

This addendum records the immediate follow-up after the earlier EPS closure:
the maintained hosted default for `EpsBridge` operator telemetry cadence is now
set to `EPS_OPERATOR_TLM_PERIOD_TICKS=1`.

Current hosted baseline under this branch head:

- `tick_ms=1000`
- `EPS_OPERATOR_TLM_PERIOD_TICKS=1`
- `ADCS_OPERATOR_TLM_PERIOD_TICKS=1`

This means the maintained hosted operator background surfaces now emit:

- EPS continuous fields about once per second:
  - `EPS_VBAT`
  - `EPS_IBAT`
  - `EPS_SOC`
  - `EPS_TEMP_BAT`
- ADCS continuous fields about once per second:
  - `ADCS_Q0..Q3`
  - `ADCS_OMEGA_X..Z`

Focused reruns after this default change closed as follows:

- focused unit coverage:
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_EpsBridge_ut_exe|OBC_Components_AdcsBridge_ut_exe)$' --output-on-failure`
  - result: `PASS`
- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `88 tests OK`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-mode-get=PASS`

## 2026-07-02 Snapshot provenance and hosted PTY isolation addendum

This addendum captures the next operator-facing Mission Console closure on the
active `mission-console-observability-bootstrap-v1` branch head.

Current delta on top of the earlier refresh-path closures:

- `SnapshotStore` now keeps operator-facing channel provenance:
  - `observationSource`
    - `live-update`
    - `refresh`
  - `observationAt`
  - `refreshCommand`
- dashboard and readback now render that provenance instead of treating every
  cached channel as an undifferentiated last value
- hosted `pty_pair_bridge` startup now carries a unique `--instance-label`, so
  the Mission Console hosted stack no longer stale-reaps every other bridge
  process solely by executable path

## 2026-07-04 Live Trends / Readback Viewer / Sequence Workspace addendum

This addendum records the current branch-head state for the active
`mission-console-live-trends-dashboard-cleanup-v1` follow-up.

Current UI delta on top of the archived Phase 1 baseline:

- `/trends` now exists as a repo-owned bounded-history chart workspace
  - first active selector tranche is limited to:
    - `EPS`
      - `EPS_VBAT`
      - `EPS_IBAT`
      - `EPS_SOC`
      - `EPS_TEMP_BAT`
    - `ADCS`
      - `ADCS_Q0..Q3`
      - `ADCS_OMEGA_X..Z`
    - `Health`
      - `SYS_CPU_USAGE`
      - `SYS_MEM_RSS_MB`
  - `GPS` is intentionally excluded from the first active trend selector set
- dashboard is now posture-first instead of `Mode Overview`-first
  - `Satellite Status`
  - `EPS Snapshot`
  - `ADCS Snapshot`
  - `Comm State`
  - `Secure Session`
  - `Mission State`
  - `Health Snapshot`
  - `Sequence & Ops`
- `/ops` now remains the dispatch surface for curated `GET_*` / status actions
- `/readback` is now a saved viewer
  - tabs:
    - `OBC`
    - `EPS`
    - `ADCS`
    - `Payload`
    - `Storage`
    - `Boot & Recovery`
    - `Sequence`
  - proof/debug evidence is still available, but only as a secondary
    `Proof / Debug Details` layer
- `/sequences` now exists as a governed authoring workspace
  - structured relative-time step editor
  - raw `.seq` fallback editor
  - official `fprime-seqgen` compile path
  - draft / generated `.seq` / latest `.bin` compile artifacts persist under
    `MISSION_CONSOLE_ROOT/sequence-drafts/`
- `packet-lab` now serializes and renders:
  - explicit `faultExplanation`
  - expected-versus-actual injected condition
  - decoded reject reason names while preserving the raw numeric code
  - separate `Injected Fault Model` versus `Observed Flight Rejection` wording

Current branch-head automated verification closed as follows:

- focused Mission Console tests:
  - `PYTHONPATH=scripts ./fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1`
  - result: `128 tests OK`
  - coverage now includes:
    - bounded trend-history retention and context/band separation
    - readback viewer tab grouping
    - trend catalog route constraints
    - sequence source rendering and compile helper routing
    - packet-lab reject-reason decoding and fault explanation shaping
- static Python compile:
  - `python3 -m py_compile scripts/mission_console/app.py scripts/mission_console/gateway/snapshots.py scripts/mission_console/gateway/parsers.py scripts/mission_console/gateway/packet_lab.py scripts/mission_console/gateway/sequence_authoring.py scripts/test_mission_console_phase1.py`
  - result: `PASS`
- bounded local browser inspection:
  - launched Mission Console locally on `http://127.0.0.1:5081`
  - verified the new `/trends`, `/readback`, `/ops`, `/sequences`, and `/packet-lab`
    pages render without immediate layout breakage

This addendum is branch-head implementation evidence only.
It does not yet claim new hosted branch-head proof for:

- trend charts with live hosted data
- sequence compile/upload/`SEQ_VALIDATE` on the maintained hosted surface
- packet-lab page sufficiency as hosted demo evidence

Those remain follow-up closeout items for the active change rather than
retroactive claims on the archived Phase 1 baseline.

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `PYTHONPATH=scripts ./fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1`
  - result: `93 tests OK`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-dashboard=PASS`
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-gps-state=PASS`
    - `hosted-readback-ttc-status=PASS`
    - `hosted-readback-payload-status=PASS`
    - `hosted-readback-eps-status=PASS`
    - `hosted-readback-comm-status=PASS`
    - `hosted-readback-storage-status=PASS`
    - `hosted-readback-adcs-attitude=PASS`
    - `hosted-packet-lab=bounded-no-op`
    - `mission-console-hosted-backpressure-check=PASS`
    - `mission-console-hosted: PASS`

Hosted conclusion from this rerun:

- the earlier `CSP_OWNER_TIMEOUT` / `GROUND_LINK_DOWN` fragment was not
  reproduced in the clean rerun
- the previously observed hosted instability was consistent with PTY bridge
  lifecycle interference between concurrent local hosted surfaces, not a new
  `COMM_GET_STATUS` regression
- Mission Console now closes refresh-based readback while also preserving the
  UI-facing distinction between background `live-update` state and operator
  `refresh` state
    - `hosted-readback-eps-status=PASS`
    - `hosted-readback-adcs-attitude=PASS`
    - `mission-console-hosted: PASS`

Hosted queue-pressure conclusion from this rerun:

- this `EPS 1s + ADCS 1s` hosted combination did not reproduce:
  - `ComCcsds.comQueue.QueueOverflow`
  - `CSP_OWNER_TIMEOUT`
  - `GROUND_LINK_DOWN`
- queue-depth channels still moved during workload, but the repo-owned hosted
  proof remained stable and closed end-to-end
- the repo-owned hosted probe now scans Mission Console listener `event.log`
  files and fails closed if any of those backpressure fragments appear during
  the proof window
- for the maintained hosted baseline, there is no current evidence that EPS
  must remain slower than ADCS to keep Mission Console usable

## 2026-07-05 Live Trends / Readback Viewer / Sequence Workspace Addendum

This addendum records the branch-head closure for the active
`mission-console-live-trends-dashboard-cleanup-v1` follow-up after adding:

- `/trends`
- saved `/readback` viewer tabs
- `/sequences` governed authoring workspace
- packet-lab fault explanation and reject-reason decoding support

Branch-head verification closed as follows:

- focused Mission Console tests:
  - `PYTHONPATH=scripts ./fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1`
  - result: `130 tests OK`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-trends-catalog=PASS`
    - `hosted-trends-history=PASS`
    - `hosted-readback-viewer=PASS`
    - `hosted-sequence-draft-save=PASS`
    - `hosted-sequence-compile=PASS`
    - `hosted-sequence-workspace-validate=PASS`
    - `hosted-packet-lab=bounded-no-op`
    - `hosted-packet-lab-fault-explanation=PASS`
    - `mission-console-hosted-backpressure-check=PASS`
    - `mission-console-hosted: PASS`

Hosted UI-facing conclusion on this branch head:

- Mission Console now keeps bounded trend history for curated `EPS`, `ADCS`,
  and `Health` channels and exposes those histories through the repo-owned
  `/trends` path instead of relying on stock GDS chart state
- `/ops` remains the dispatch surface for `GET_*`, while `/readback` now acts
  as a saved viewer over the existing readback cache rather than a button wall
  that discards the previous result on the next query
- `GET_*` and status commands sent from the normal `/ops` command workspace now
  route through the same readback-saving path as the older curated readback
  buttons, so a workspace `MODE_GET` populates `/readback` without requiring a
  separate duplicate dispatch panel
- `/sequences` can now save structured drafts under
  `MISSION_CONSOLE_ROOT/sequence-drafts/`, render official `.seq` source,
  compile through official `fprime-seqgen`, and hand the compiled `.bin` into
  the existing governed upload + `SEQ_VALIDATE` flow
- `packet-lab` now serializes a first-class `faultExplanation` block that
  identifies the intentionally corrupted field and the expected-versus-actual
  fault condition even when the observed result closes as bounded-no-op rather
  than explicit reject
- `/trends` no longer silently carries default EPS series into a later Health
  view unless cross-group overlay is explicitly enabled, and the chart now
  keeps a stable logical canvas height instead of re-scaling itself on every
  refresh
- browser-level hosted walkthrough now confirms the packet-lab page itself can
  explain a `tampered-mac` demo packet without switching to a separate event
  view:
  - `Injected Fault Model` shows the wrong field (`MAC / auth tag`)
  - `Observed Flight Rejection` shows the decoded reject reason (`BAD_MAC`, code `15`)
  - `Observed Reject Events` renders the fresh reject event as an inline
    timeline instead of leaking `[object Object]`

Remaining manual-evidence boundary:

- this addendum now closes the hosted browser-level walkthrough for the
  packet-lab page itself on the maintained hosted surface
- it still does not claim target-side UI parity proof for this tranche

Hosted probe pacing note:

- a zero-settle burst of back-to-back readback jobs was able to self-induce a
  transient hosted `CSP_OWNER_TIMEOUT` / `GROUND_LINK_DOWN` fragment before the
  UI-facing proof reached the later packet-lab and sequence steps
- the repo-owned hosted proof now inserts a bounded `1 s` settle between
  completed operator jobs so the branch-head evidence matches the intended
  Mission Console operator cadence instead of a synthetic closeout burst
- with that pacing, the hosted proof closed end-to-end without:
  - `ComCcsds.comQueue.QueueOverflow`
  - `CSP_OWNER_TIMEOUT`
  - `GROUND_LINK_DOWN`

## 2026-07-01 ADCS_GET_ATTITUDE Operator-Telemetry Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
same forced-resend model into `AdcsBridge`.

Flight-side closure on this branch head:

- `AdcsBridge` now uses a three-layer operator-facing telemetry model:
  - continuous plus explicit resend:
    - `ADCS_Q0`
    - `ADCS_Q1`
    - `ADCS_Q2`
    - `ADCS_Q3`
    - `ADCS_OMEGA_X`
    - `ADCS_OMEGA_Y`
    - `ADCS_OMEGA_Z`
  - change-driven plus explicit resend:
    - `ADCS_MODE`
  - explicit refresh only:
    - `ADCS_MAG_X`
    - `ADCS_MAG_Y`
    - `ADCS_MAG_Z`
    - `ADCS_POINTING_ERR`
- `ADCS_GET_ATTITUDE` now forces a resend of the operator-facing ADCS status
  set even when the current values did not change
- successful `ADCS_SET_MODE`, `ADCS_SET_TARGET`, `ADCS_CALIBRATE`, and
  `RESET` now reuse the same explicit refresh helper
- background operator telemetry cadence is now separated from internal ADCS
  polling; the maintained hosted default is `ADCS_OPERATOR_TLM_PERIOD_TICKS=1`

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `88 tests OK`
- focused AdcsBridge unit coverage:
  - `fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target OBC_Components_AdcsBridge_ut_exe -j4`
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^OBC_Components_AdcsBridge_ut_exe$' --output-on-failure`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-eps-status=PASS`
    - `hosted-readback-adcs-attitude=PASS`
    - `hosted-packet-lab=explicit-reject`
    - `hosted-auth-uhf-primary=PASS`
    - `hosted-uhf-boot-command=PASS`
    - `mission-console-hosted: PASS`

Hosted ADCS closure from this rerun:

- Mission Console `ADCS_GET_ATTITUDE` readback now closes on the real flight
  channel names instead of the older nonexistent aliases
- the maintained hosted proof now requires fresh:
  - `ADCS_MODE`
  - `ADCS_Q0`
  - `ADCS_OMEGA_X`
- `ADCS_POINTING_ERR` remains part of the explicit refresh set, but it is not
  used as the minimum closeout oracle for the Mission Console readback path
- this hosted rerun did not introduce a new `ComCcsds.comQueue.QueueOverflow`
  blocker; the earlier hosted queue-pressure issue remained closed by keeping
  the shared runtime at `tick_ms=1000`

## 2026-07-01 GPS_GET_STATE Forced-Refresh Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
same forced-refresh model into `GpsBridge`.

Flight-side closure on this branch head:

- `GPS_GET_STATE` now forces a resend of the operator-facing GPS status set
  even when the current values did not change
- the validated shared/sticky operator GPS channels are:
  - `GPS_SOURCE_MODE`
  - `GPS_HAVE_SAMPLE`
  - `GPS_FIX_VALID`
  - `GPS_SAT_COUNT`
- the validated explicit-refresh GPS detail channels are:
  - `GPS_LAT_DEG`
  - `GPS_LON_DEG`
  - `GPS_ALT_M`
  - `GPS_SPEED_MPS`
  - `GPS_COURSE_DEG`
  - `GPS_HDOP`
  - `GPS_UTC_SEC_OF_DAY`
  - `GPS_UTC_DATE_YMD`
  - `GPS_ACCEPTED_SENTENCES`
  - `GPS_REJECTED_SENTENCES`
- the final maintained implementation is:
  - keep the same existing GPS channels
  - keep scheduled poll semantics focused on cache/update logic
  - let `GPS_GET_STATE` directly resend the same channels on demand
  - do not rely on a second GPS status surface or event-only readback

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `89 tests OK`
- focused GpsBridge coverage:
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_GpsBridge_ut_exe|gps_bridge_contract_test|gps_bridge_cached_state_integration_test)$' --output-on-failure`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-gps-state=PASS`
    - `hosted-readback-eps-status=PASS`
    - `hosted-readback-adcs-attitude=PASS`
    - `mission-console-hosted: PASS`

Hosted GPS closure from this rerun:

- Mission Console `GPS_GET_STATE` readback now closes on fresh:
  - `GPS_SOURCE_MODE`
  - `GPS_FIX_VALID`
  - `GPS_LAT_DEG`
- the first raw-refresh attempt was intentionally abandoned because hosted
  native channel logs only exposed the detailed `always` GPS fields reliably,
  while the shared GPS posture fields did not close the readback oracle
- the stable branch-head fix was to make the shared GPS posture channels
  directly resendable on the same channel path, then let `GPS_GET_STATE`
  explicitly call the same `tlmWrite_*` outputs
- this hosted rerun did not introduce a new `ComCcsds.comQueue.QueueOverflow`
  blocker; the maintained hosted queue-pressure conclusion remains unchanged
  from the earlier EPS/ADCS reruns

## 2026-06-22 Post-Review Maintenance Addendum

The sections below preserve the original 2026-06-20 closeout narrative. They
are still useful for understanding how the branch originally closed, but they
are no longer the best quick reference for the current pre-submit operator
state after the PR review cycle.

Current operator-facing startup and manual self-test instructions now live in:

- [`docs/operator/mission-console.md`](../../../docs/operator/mission-console.md)
- [`docs/operator/mission-console.md`](../../../docs/operator/mission-console.md)

Current post-review regression reruns on the latest maintained branch head
(`51ea62e50`) closed as follows:

- focused unit coverage:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `66 tests OK`
- fresh local gate:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
- fresh hosted rerun:
  - `bash scripts/ensure_ground_dual_gds_baseline.sh`
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-auth-sband=PASS`
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-payload-status=PASS`
    - `hosted-upload=PASS`
    - `hosted-seq-validate=PASS`
    - `hosted-packet-lab=explicit-reject`
    - `hosted-switch-uhf=PASS`
    - `hosted-sband-invalidated=PASS`
    - `hosted-auth-uhf-primary=PASS`
    - `hosted-uhf-boot-command=PASS`
    - `hosted-history-cache=PASS`
    - `mission-console-hosted: PASS`
- fresh target rerun:
  - `bash scripts/ensure_target_comm_lab_baseline.sh`
  - `bash scripts/ensure_ground_dual_gds_baseline.sh`
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed summary:
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=explicit-reject`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`

Review-cycle behavior changes that matter for current manual testing:

- packet-lab hosted/target closeout now converges to `explicit-reject`, not the
  earlier `bounded-no-op` examples recorded in the original archive narrative
- `duplicate-sequence` / `tampered-sequence` priming now invalidates the
  current session if acceptance evidence never appears; the operator must
  re-auth before retrying that packet-lab path

## Fresh Local Gate

Fresh repository gate:

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
  - covered generate/build, UT generate/build, `fprime-util check --all`,
    repo consistency, documentation governance, component-test baseline,
    legacy ZMQ retirement, and `openspec validate --specs`

Focused unit coverage:

- `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `14 tests OK`
  - covers structured manual action helpers, registry parsing, listener parser
    formats, packet summary extraction, packet-lab result classification,
    ensure-auth flow logic, target auth journal-fallback acceptance logic,
    bounded CLI timeout handling, native listener readback fallback, and
    command-completion fallback extraction

## Hosted Mission Console Closure

Fresh hosted probe:

- `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`

Hosted probe scope closed on a fresh hosted manual dual-GDS surface plus a
fresh Mission Console runtime:

- Dashboard discovery over `/api/contexts` and `/api/dashboard`
- Mission Console S-band auth over `/api/auth/ensure`
- structured channel-refresh readback:
  - `OBCApp.modeManager.MODE_GET`
- structured event-based readback:
  - `OBCApp.payloadOpsController.PAYLOAD_GET_STATUS`
- governed upload through `/api/files/upload`
  - source: `scripts/manual_ops/examples/sample-sequence.bin`
- governed sequence preflight through `/api/sequences/validate`
- bounded packet-lab negative case:
  - `tampered-mac`
  - observed result: `bounded-no-op`
- explicit `COMM_SET_ACTIVE(UHF)` through Mission Console
- S-band secure-state invalidation after band switch
- `uhf-primary-after-failover` re-auth through Mission Console
- quiet-path UHF secure command proof:
  - `OBCApp.bootManager.BOOT_STATUS`
- history and readback-cache persistence

Observed hosted summary lines:

- `hosted-auth-sband=PASS`
- `hosted-readback-mode-get=PASS`
- `hosted-readback-payload-status=PASS`
- `hosted-upload=PASS`
- `hosted-seq-validate=PASS`
- `hosted-packet-lab=bounded-no-op`
- `hosted-switch-uhf=PASS`
- `hosted-sband-invalidated=PASS`
- `hosted-auth-uhf-primary=PASS`
- `hosted-uhf-boot-command=PASS`
- `hosted-history-cache=PASS`

Interpretation:

- the new Flask/Gateway/UI layer can drive the maintained hosted authority path
  without falling back to manual CLI auth
- the Gateway-owned listener/snapshot layer stays live enough to support
  dashboard, readback cache, history, and packet-lab evidence surfaces
- the final hosted UHF proof is intentionally command-path scoped, not a fake
  live readback claim, because the maintained quiet `uhf-primary-after-failover`
  baseline does not guarantee reviewable `BOOT_RECOVERY_STATUS` on the UHF
  `events.log` surface
- file upload required one implementation correction: Mission Console must run
  the existing GDS upload helper in a subprocess, because `IntegrationTestAPI`
  uses `signal` and cannot be awaited from a thread-pool worker

## Target Parity Attempts

Fresh target Mission Console parity probe:

- `bash scripts/run_mission_console_phase1_target_probe.sh`

Observed target results across bounded reruns before final closure:

- one bounded rerun reached:
  - `target-context=running`
  - `target-auth-sband=PASS`
  - `target-readback-watchdog=PASS`
  - `target-readback-boot-status=PASS`
  - `target-readback-fault-history=PASS`
  - then failed only because the earlier packet-lab classifier marked the
    target negative result as `inconclusive`
- after classifier correction, repeated target reruns failed earlier on S-band
  auth with:
  - `timed out waiting for CHALLENGE on .../target-ground/captures/sband-southbound-to-gds.bin`

The target-specific Mission Console blocker was then classified with the new
layer removed:

- direct baseline:
  - `bash scripts/manual_ops/target/start_target_manual_baseline.sh`
- direct ground surface:
  - `bash scripts/manual_ops/target/start_target_manual_ground_surface.sh`
- direct maintained helper:
  - `fprime-venv/bin/python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest ... auth establish`

Direct helper result:

- same failure:
  - `TimeoutError: timed out waiting for CHALLENGE on /private/tmp/manual-dual-gds/target-ground-mc-blocker/captures/sband-southbound-to-gds.bin`

Additional bounded diagnosis after Mission Console listener/native-log
hardening:

- Mission Console target S-band auth requests are definitely leaving the
  Gateway side:
  - `target-ground/captures/sband-gds-to-southbound.bin` grows during auth
    attempts
  - Mission Console `native-events/recv.bin` also grows with repeated APID
    `0x00FE` auth-request packets
- but the target path still does not return a usable handshake response:
  - `target-ground/captures/sband-southbound-to-gds.bin` remains size `0`
  - Mission Console `native-events/recv.bin` contains repeated outbound
    request packets only, with `CHALLENGE=0` and `AUTH_STATUS=0` when decoded
    through `load_handshake_messages_from_native_packet_log(...)`

Interpretation of the added diagnosis:

- Mission Console now has stronger evidence that the local operator surface is
  actively emitting auth requests on the maintained target path
- the maintained environment still fails to return target-side secure-auth
  challenge/status traffic on that path when the blocker manifests
- this strengthens the existing classification that the remaining parity
  blocker is outside the Mission Console UI/Gateway layer

Final target closure rerun after aligning the target auth watcher with the
current node-`5` lessons learned:

- target auth now prefers the maintained native-log-first watcher model instead
  of waiting only on wire capture
- when target journal indicates `SECURE_AUTH_CHALLENGE_ISSUED`, Mission Console
  gives the late challenge packet a bounded extra wait window before resending
  `REQ_AUTH`
- Mission Console listener ownership now de-duplicates canonical-band aliases
  so `uhf-backup` and `uhf-primary-after-failover` do not start redundant
  passive listeners on the same TTS port

Fresh target rerun result:

- `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`

Observed final target summary lines:

- `target-auth-sband=PASS`
- `target-readback-watchdog=PASS`
- `target-readback-boot-status=PASS`
- `target-readback-fault-history=PASS`
- `target-packet-lab=bounded-no-op`
- `target-dashboard=PASS`

Interpretation:

- Mission Console now closes the maintained target manual dual-GDS parity slice
  that this Phase 1 plan claimed
- the earlier target blocker was a proof/operator-surface alignment problem,
  not a new Mission Console product defect

Fresh-gate reruns then exposed a narrower remaining target baseline bug:

- fresh local gate:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
- fresh target rerun after the gate:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `FAIL`
  - observed summary:
    - `target-auth-sband-attempt-1=FAILED`
    - `target-auth-sband-attempt-2=FAILED`
    - `target-auth-sband-attempt-3=FAILED`
  - failure:
    - `timed out waiting for CHALLENGE on .../target-ground/captures/sband-southbound-to-gds.bin`
- one additional clean rerun on the same maintained wrapper family:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `FAIL`
  - same maintained-path `CHALLENGE` timeout symptom

Bounded diagnosis of that later target rerun showed the blocker was no longer
the Mission Console watcher/oracle layer itself:

- target journal still issued fresh S-band challenges
- the failing rerun showed:
  - `UnexpectedSequenceCount : ... Transmitted: 0 | Expected on board: 4`
  - repeated `SECURE_AUTH_CHALLENGE_ISSUED`
- the fresh local ground helper for each wrapper rerun was restarting its GDS
  generation from handshake APID `0x00FE` sequence `0`
- the maintained target baseline was not being explicitly reset between
  independent wrapper reruns, so auth-generation state could leak across runs

The final target fix was therefore split into the A-owned readiness path plus a
snapshot-only Mission Console attach path, instead of leaving reset/cleanup in
the C probe:

- `scripts/comm_verification/lib/ensure_target_comm_lab_baseline.py` now
  accepts a bounded `TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1` request
- `scripts/run_mission_console_phase1_target_probe.sh` now attaches to an
  A-prepared baseline and writes only snapshot metadata for Mission Console;
  it no longer restarts or retires shared target baseline state from `C`
- the target probe now records:
  - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`

Fresh target stability after the A-owned reset change:

- back-to-back target rerun #1:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed closeout:
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=bounded-no-op`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`
- back-to-back target rerun #2:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed closeout:
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=bounded-no-op`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`
- fresh post-gate target rerun:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`

Interpretation of the target-side closure:

- the remaining target blocker was a baseline-generation/reset-ownership
  problem, not a new Mission Console product defect
- target parity is now stable enough across independent wrapper reruns to keep
  the maintained S-band auth/readback/dashboard slice closed on the current
  branch
- the A/B/C contract is preserved because the postflight step reruns `A -> B`
  instead of letting `C` stop or reset the shared target baseline

Fresh hosted and target reruns after the final readback hardening now both
close again on the post-gate binary set:

- fresh hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed closeout:
    - `hosted-auth-sband=PASS`
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-payload-status=PASS`
    - `hosted-upload=PASS`
    - `hosted-seq-validate=PASS`
    - `hosted-packet-lab=bounded-no-op`
    - `hosted-switch-uhf=PASS`
    - `hosted-sband-invalidated=PASS`
    - `hosted-auth-uhf-primary=PASS`
    - `hosted-uhf-boot-command=PASS`
    - `hosted-history-cache=PASS`
    - `mission-console-hosted: PASS`
- fresh target rerun:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed closeout:
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=bounded-no-op`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`

Final readback hardening that closed the fresh reruns:

- hosted representative readback no longer relies only on polling timing;
  Gateway now falls back to its own native listener logs before bounded
  `fprime-cli --search`, so `MODE_GET` and `PAYLOAD_GET_STATUS` stay stable
  even when passive snapshot polling lags
- branch head has now moved `BOOT_STATUS` onto the same shared-channel
  forced-resend pattern used by the other maintained `GET_*` status queries,
  so boot truth closes on fresh `BOOT_*` telemetry rather than a bounded
  `BOOT_RECOVERY_STATUS` / command-completion fallback path

## Current Closeout State

- fresh local gate: `PASS`
- hosted Mission Console closure: `PASS`
- target Mission Console parity: `PASS`, including repeated reruns and one
  fresh post-gate rerun after the final readback hardening
- OpenSpec change archive:
  - `openspec archive mission-console-phase1 -y`
  - result: archived as `2026-06-20-mission-console-phase1`
- post-archive spec validation:
  - `openspec validate --specs`
  - result: `PASS` (`31 passed, 0 failed`)
- post-archive reconciliation:
  - `python3 scripts/generate_reconciliation_matrix_md.py`
  - result: `PASS`
  - `python3 scripts/check_repo_consistency.py`
  - result: `PASS`
- verification-path registry update: not required for this change
- `local-ready`: reached on the current branch/archive closeout state

Closeout interpretation:

- Mission Console Phase 1 is now archived with fresh local verification, fresh
  hosted closure, stable target parity, and post-archive governance checks all
  closed on the current branch state
- the maintained authority boundary remains unchanged: Mission Console reuses
  the current manual secure/operator surface rather than introducing a second
  command plane

## 2026-07-01 TTC And Storage Shared-Status Refresh Addendum

This addendum records the current branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
same forced-resend model into `TtcPassManager` and `StorageHealthBridge`.

Flight-side closure on this branch head:

- `TTC_GET_STATUS` now forces a resend on the existing shared TTC channels
  instead of only relying on background state churn
- successful `TTC_SET_POLICY`, `TTC_SET_PASS_WINDOW`, and
  `TTC_CLEAR_PASS_WINDOW` now reuse the same explicit refresh path
- scheduler-side TTC updates no longer resend the full policy detail set every
  tick; only operator-facing `2+3` fields emit on change
- `STORAGE_GET_STATUS` now forces a resend on the existing shared storage
  channels even when values do not change
- scheduled storage scans no longer use the old summary path to spray detailed
  storage state; only operator-facing `2+3` fields emit on change
- Mission Console now promotes successful readback `freshChannels` back into
  `SnapshotStore`, so dashboard cards can reuse the same fresh values without
  adding hidden polling

Verification on this branch head closed as follows:

- focused Mission Console tests:
  - `fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `91 tests OK`
- focused unit coverage:
  - `fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target OBC_Components_TtcPassManager_ut_exe OBC_Components_StorageHealthBridge_ut_exe -j4`
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_TtcPassManager_ut_exe|OBC_Components_StorageHealthBridge_ut_exe)$' --output-on-failure`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-mode-get=PASS`
    - `hosted-readback-gps-state=PASS`
    - `hosted-readback-ttc-status=PASS`
    - `hosted-readback-eps-status=PASS`
    - `hosted-readback-storage-status=PASS`
    - `hosted-readback-adcs-attitude=PASS`
    - `mission-console-hosted: PASS`

Current operator-facing implication:

- on the current branch head, a successful `TTC_GET_STATUS` or
  `STORAGE_GET_STATUS` no longer stops at the readback panel
- the same fresh channel samples are now available to the dashboard snapshot
  path, so previously `Unavailable` cards can fill in immediately after the
  operator explicitly requests current status

## 2026-07-01 COMM_GET_STATUS First-Tranche Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
same forced-resend model into `CommController`.

Flight-side closure on this branch head:

- `CommController` now exposes `COMM_GET_STATUS`
- `COMM_GET_STATUS` forces a resend on the existing shared comm posture
  channels even when values do not change
- the first operator-facing resend set is:
  - `COMM_ACTIVE_BAND`
  - `COMM_PRIMARY_COMMAND_LINK`
  - `COMM_PRIMARY_TELEMETRY_LINK`
  - `COMM_PRIMARY_FILE_LINK`
  - `COMM_S_BAND_AVAILABLE`
  - `COMM_UHF_AVAILABLE`
  - `COMM_S_BAND_AVAILABILITY_REASON`
  - `COMM_UHF_AVAILABILITY_REASON`
  - `COMM_FDIR_FAULT_LATCHED`
  - `COMM_FDIR_FAULT_KIND`
- this tranche intentionally does not promote:
  - `COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE`
  - `CommEgressMux` packet counters
  - `GroundLinkDriver` low-level transport counters
- command authority was extended so `COMM_GET_STATUS` is accepted as a
  `READ_STATUS` action on `COMM_LINK`, including the maintained UHF-backup
  allowance shape used by the current operator flow

Verification on this branch head closed as follows:

- focused Mission Console test:
  - `PYTHONPATH=scripts fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1.MissionConsolePhase1Test.test_channel_refresh_prefers_fresh_comm_channels_over_snapshot_only`
  - result: `PASS`
- focused CommController unit coverage:
  - `fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommController_ut_exe`
  - `fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^(command_authority_catalog_check|OBC_Components_CommController_ut_exe)$' --output-on-failure`
  - result: `PASS`
- focused hosted reruns:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result for the new comm readback step: `PASS`
  - observed summary:
    - `hosted-readback-comm-status=PASS`
  - observed action-history evidence:
    - `COMM_GET_STATUS` closed as `channel-refresh-based`
    - `channelSource=native-log`
    - fresh channels included:
      - `COMM_ACTIVE_BAND`
      - `COMM_S_BAND_AVAILABLE`
      - `COMM_UHF_AVAILABLE`
      - `COMM_FDIR_FAULT_LATCHED`

Hosted closeout boundary for this tranche:

- on the current branch head, `COMM_GET_STATUS` itself is proven on hosted
  runtime
- the full hosted probe rerun did not close end-to-end because the existing
  backpressure scan still found earlier listener evidence for:
  - `CSP_OWNER_TIMEOUT`

## 2026-07-03 Recovery + Watchdog Single-Event Readback Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after tightening the
first `single-event` Mission Console readback contract.

Mission Console closure on this branch head:

- `GET_RECOVERY_STATUS` now closes only on a fresh `RECOVERY_STATUS` event
- `GET_HW_WATCHDOG_STATUS` now closes only on a fresh `HW_WATCHDOG_STATUS`
  event
- `command completion` is preserved only as fallback evidence and does not
  close success for either command
- failed readbacks now keep their structured `fallback evidence` or
  `incomplete result` payload in the job result instead of dropping all
  evidence on failure
- the `/readback` view now distinguishes:
  - `Main Evidence`
  - `Fallback Evidence`
  - `Incomplete Result`

Verification on this branch head closed as follows:

- full Mission Console test suite:
  - `./fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-recovery-status=PASS`
    - `hosted-readback-watchdog-status=PASS`
    - `mission-console-hosted: PASS`
  - observed action-history evidence:
    - `GET_RECOVERY_STATUS` closed as `event-based`
    - `mainEvidence.kind=fresh-event`
    - `eventSource=native-log`
    - `GET_HW_WATCHDOG_STATUS` closed as `event-based`
    - `mainEvidence.kind=fresh-event`
    - `eventSource=native-log`

Current operator-facing implication:

- on the current branch head, recovery and watchdog query commands no longer
  accept stale event hits or bare completion as if they were full readback
  truth
- if a future path only observes completion or an incomplete event payload,
  the operator can now see that bounded fallback directly instead of getting a
  misleading success

## 2026-07-03 Payload Single-Event Readback Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after extending the
strict `single-event` readback contract into the payload query surface.

Mission Console closure on this branch head:

- `PAYLOAD_GET_STATUS` now closes only on a fresh, fully structured
  `PAYLOAD_STATUS` event
- `PAYLOAD_GET_CAPABILITIES` now closes only on a fresh, fully structured
  `PAYLOAD_CAPABILITIES` event
- `PAYLOAD_GET_LAST_CAPTURE_METADATA` now closes only on a fresh, fully
  structured `PAYLOAD_CAPTURE_METADATA` event
- a fresh payload event with missing required fields is now marked as
  `incomplete result` instead of succeeding on event-name match alone
- `command completion` remains visible only as fallback evidence and does not
  close success for these payload queries

Verification on this branch head closed as follows:

- full Mission Console test suite:
  - `./fprime-venv/bin/python scripts/test_mission_console_phase1.py`
  - result: `PASS`
- focused hosted rerun:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - result: `PASS`
  - observed summary:
    - `hosted-readback-payload-status=PASS`
    - `hosted-readback-payload-capabilities=PASS`
    - `hosted-readback-payload-capture-metadata=PASS`
    - `mission-console-hosted: PASS`
  - observed action-history evidence:
    - `PAYLOAD_GET_STATUS` closed as `event-based`
    - `PAYLOAD_GET_CAPABILITIES` closed as `event-based`
    - `PAYLOAD_GET_LAST_CAPTURE_METADATA` closed as `event-based`
    - all three reported `mainEvidence.kind=fresh-event`

Current operator-facing implication:

- on the current branch head, payload query commands no longer succeed just
  because a payload-named event showed up; the event now has to carry a
  complete structured result for the current query window
- if a future path only observes completion or a malformed payload event, the
  operator can distinguish that bounded fallback from full payload truth
  - paired `GROUND_LINK_DOWN` / `GROUND_LINK_UP`
- in the captured reruns, those fragments appeared before the
  `COMM_GET_STATUS` dispatch/completion window, so this branch-head evidence
  does not attribute that hosted backpressure fragment to the new comm refresh
  path itself

## 2026-07-03 Persistent + Boot Readback Closure Addendum

This addendum records the next branch-head closure for the active
`mission-console-observability-bootstrap-v1` follow-up after finishing the
remaining `persistent + boot` readback family.

Mission Console closure on this branch head:

- `BOOT_STATUS` now closes as `channel-refresh-based`
- `BOOT_STATUS` no longer depends on `command completion` fallback; it forces
  a resend of the existing boot telemetry family even when values did not
  change
- `GET_RESET_CAUSE` now closes only on a fresh structured
  `BOOT_RECOVERY_STATUS` event
- `GET_BOOT_COUNT` now closes only on a fresh structured
  `BOOT_RECOVERY_STATUS` event
- `GET_PERSISTENT_FAULT_HISTORY` now closes only on a fresh
  `PERSISTENT_FAULT_HISTORY_STATUS` plus the required fresh
  `PERSISTENT_FAULT_HISTORY_RECORD` group, or on a fresh zero-record status
  when the query legitimately returns an empty set
- identical duplicate event hits are now deduplicated in Mission Console
  freshness classification before single-event or multi-event closing is
  evaluated

Verification on this branch head closed as follows:

- focused component unit tests:
  - `./fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R 'BootManager|PersistentFaultManager' --output-on-failure`
  - result: `PASS`
- full Mission Console Python test suite:
  - `PYTHONPATH=scripts ./fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1`
  - result: `PASS`
- focused hosted rerun for the relevant readback family:
  - `bash scripts/run_mission_console_phase1_hosted_probe.sh`
  - relevant observed summary before the wrapper hit an unrelated later
    assertion:
    - `hosted-readback-boot-status=PASS`
    - `hosted-readback-reset-cause=PASS`
    - `hosted-readback-boot-count=PASS`
    - `hosted-readback-fault-history=PASS`
  - relevant observed action-history evidence:
    - `BOOT_STATUS` closed as `channel-refresh-based`
    - fresh channels included:
      - `BOOT_RESET_CAUSE`
      - `BOOT_BOOT_COUNT`
      - `BOOT_SAFE_FALLBACK_REQUIRED`
    - `GET_RESET_CAUSE` closed as `event-based`
    - `mainEvidence.kind=fresh-event`
    - `GET_BOOT_COUNT` closed as `event-based`
    - `mainEvidence.kind=fresh-event`
    - `GET_PERSISTENT_FAULT_HISTORY` closed as `event-based`
    - `mainEvidence.kind=fresh-event-group`

Current operator-facing implication:

- on the current branch head, boot query commands are no longer mixed into a
  generic event fallback family; `BOOT_STATUS` is now a proper shared-channel
  status query, while `GET_RESET_CAUSE` and `GET_BOOT_COUNT` remain strict
  single-event queries
- persistent fault history no longer closes on partial or stale evidence; the
  operator now gets either a complete fresh group, a valid fresh empty result,
  or an explicit incomplete result
- the full hosted wrapper rerun still ended later at the pre-existing
  `hosted-switch-uhf` `sband secure state was not invalidated` assertion, but
  that occurred after the four boot/persistent readback steps above had
  already closed successfully and is not attributed here to the new
  boot/persistent readback changes

## 2026-07-03 `4.3c` Branch-Head Verification Addendum

This addendum records the closeout rerun used to finish `4.3c` for the active
`mission-console-observability-bootstrap-v1` change.

Branch-head verification scope for this rerun:

- authoritative full local gate on the current change content
- target installed-release provenance confirmation before probe execution
- governed target A/B/C rerun on the maintained target manual dual-GDS path

Observed branch-head verification steps:

- target installed release provenance before rerun:
  - `$OBC_HOME/obc-deploy/current -> $OBC_HOME/obc-deploy/releases/v0.1.0-233-gc48d36942`
  - `meta/version.json.project_version = v0.1.0-233-gc48d36942`
  - `manifest.json.release_id = v0.1.0-233-gc48d36942`
- authoritative full local gate:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - result: `PASS`
- target A baseline:
  - `bash scripts/ensure_target_comm_lab_baseline.sh`
  - result: `REPAIRED`
  - repair applied:
    - removed stale override `58-sband-ingress-diagnostics.conf`
- target B baseline:
  - `bash scripts/ensure_ground_dual_gds_baseline.sh`
  - result: `READY`
- target C probe:
  - `bash scripts/run_mission_console_phase1_target_probe.sh`
  - result: `PASS`
  - observed summary:
    - `target-auth-sband=PASS`
    - `target-readback-watchdog=PASS`
    - `target-readback-boot-status=PASS`
    - `target-readback-fault-history=PASS`
    - `target-packet-lab=explicit-reject`
    - `target-dashboard=PASS`
    - `target-baseline-after=PASS source=ensure-target-comm-lab-baseline`
    - `mission-console-target: PASS`

Readback-specific closeout implications from this rerun:

- `BOOT_STATUS` now closes on the maintained target path as
  `channel-refresh-based`
- `GET_PERSISTENT_FAULT_HISTORY` now closes on the maintained target path as a
  complete `fresh event group`
- the final target blockers before this addendum were not flight-side
  `BOOT_STATUS` or `GET_PERSISTENT_FAULT_HISTORY` product behavior; they were
  Mission Console native log searches assuming the suffix after the readback
  marker always contains every field or event of the current command window in
  stable order
- native channel search now backfills any still-missing requested field by
  rescanning the same native listener log bounded by command-time
- native event search now applies the same bounded full-log backfill so
  multi-event groups such as persistent fault history do not fail just because
  part of the group landed earlier than the current suffix marker

## 2026-07-05 Mission Console UI State / Dashboard / Trends Polish Addendum

This addendum records the branch-head UI walkthrough for the active
`mission-console-live-trends-dashboard-cleanup-v1` change after the selector,
dashboard, trends, readback-viewer, and history/session polish landed.

Observed branch-head validation on this branch head:

- static/frontend sanity:
  - `node --check scripts/mission_console/static/mission-console.js`
  - result: `PASS`
- focused Mission Console automated coverage:
  - `PYTHONPATH=scripts ./fprime-venv/bin/python -m unittest scripts.test_mission_console_phase1`
  - result: `134 tests OK`
- change artifact validation:
  - `openspec validate mission-console-live-trends-dashboard-cleanup-v1`
  - result: `PASS`
- hosted desktop UI walkthrough:
  - surface:
    - `bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh`
    - result: hosted manual dual-GDS surface `running`
  - Mission Console app:
    - `MISSION_CONSOLE_PORT=5081 fprime-venv/bin/python scripts/mission_console/app.py`
  - browser walkthrough scope:
    - `/ops`
    - `/dashboard`
    - `/trends`
    - `/readback`
    - `/surfaces`
    - `/packet-lab`

Observed operator-facing closure from this walkthrough:

- selector persistence:
  - switched hosted band from `sband` to `uhf-backup`
  - navigated across `/ops` and `/readback`
  - selected band stayed on `uhf-backup`
  - switching to `target-manual-ground-dual-gds` with no active bands produced
    the bounded empty-band placeholder, then switching back to hosted restored
    the remembered hosted band
- dashboard semantics:
  - dashboard top note now states that `Satellite Status`, `EPS`, `ADCS`,
    `Comm`, and `Health` are context-latest summaries while `Secure Session`
    remains selected-band truth
  - current populated dashboard fields show their own secondary `Via <band>`
    provenance instead of silently pretending the whole page is single-band
- ops/readback split:
  - `/ops` `Job Result` is now a compact panel inside `Secure Command Workspace`
    so it stays close to the main operator controls without using a separate
    sticky right-column sidebar
  - `MODE_GET` dispatched from the normal command workspace is saved into the
    `/readback` viewer on the same `MODE_GET` card; it is not a special-case
    path that requires a separate legacy curated readback button wall
- readback viewer:
  - first tab is now `OBC`
  - saved viewer no longer presents proof/debug as the primary reading surface
  - `Proof / Debug Details` stays open across polling once expanded
  - each saved card can quick-refresh its own source `GET_*` / status command
    without switching back to `/ops`
- trends workspace:
  - default desktop layout now starts with two trend panels
  - panel-local legend rows replace the earlier permanent right-hand legend
    and chart-notes sidebars
  - selecting only `Memory RSS (MB)` in its own panel kept the line visible and
    correctly scaled on that panel's own Y-axis
- dashboard maintenance actions:
  - `/dashboard` remains dispatch-free
  - the selected context now exposes `Clear context cache`
  - the selected context/band `Recent Events` ring exposes `Clear current view`
  - verbose provenance such as `Background update / Flight sampled / Gateway observed / Via ...`
    now lives in hover tooltip metadata instead of expanding the card body
- packet-lab wording:
  - the main result page now separates ground-side `Injected Fault Model` from
    flight-side `Observed Flight Rejection`
  - history defaults to the current console session and exposes `show all`,
    `clear current session`, and `clear all`

This addendum is hosted branch-head UI evidence only.
It does not introduce a new target UI parity claim and does not claim any new
flight-side observability owner beyond the bootstrap work already closed
separately.
