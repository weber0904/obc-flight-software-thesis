# Mission Console Phase 1 Runbook

Status: active local operator runbook
Updated: 2026-07-05
Traditional Chinese thesis demonstration:
[`thesis-demo-routes.zh-TW.md`](thesis-demo-routes.zh-TW.md)

This runbook is the operator-facing entrypoint for the repo-owned
`Mission Console Phase 1` Flask surface under
[`scripts/mission_console/`](../../scripts/mission_console/).

Current interface and authority assumptions are recorded in
[`docs/interfaces.md`](../interfaces.md) and the Mission Console OpenSpec
capability. This runbook is the canonical operator procedure.

## Scope

Phase 1 provides:

- hosted-first Mission Gateway + Dashboard
- a branch-local `/beacon` workspace for bounded UHF beacon
  side-channel/sidecar viewing on supported manual surfaces
- repo-owned `/trends` workspace with bounded history charts for curated live channels
- dictionary-backed command catalog plus schema-driven `/ops` workspace
- repo-owned auth / secure command / governed upload / sequence action routes
- gateway-owned `fprime-cli events/channels` listeners
- structured dashboard and saved readback viewer
- governed `/sequences` authoring workspace with official `.seq -> fprime-seqgen -> .bin`
- bounded `packet-lab` negative packet demo surface

Phase 1 does not provide:

- stock GDS replacement for every engineering tab
- one-GDS aggregation
- pass planner / mission scheduler
- second command authority

## How To Use This Runbook

Use this file when your goal is:

- start from a clean shell
- prepare the maintained hosted or target manual dual-GDS baseline
- launch Mission Console
- open the UI and drive tests from `/ops`, `/readback`, `/beacon`, `/trends`, `/sequences`, and `/packet-lab`

This file intentionally duplicates the minimum end-to-end order needed for a
real operator demo or thesis rehearsal so you do not need to reconstruct the
startup sequence from multiple documents.

Use the deeper baseline runbooks only when you need lifecycle detail that is
outside the Mission Console flow:

- hosted manual dual-GDS detail:
  [`docs/operator/hosted-manual-dual-gds-runbook.md`](hosted-manual-dual-gds-runbook.md)
- target manual dual-GDS detail:
  [`docs/operator/target-manual-dual-gds-runbook.md`](target-manual-dual-gds-runbook.md)
- target A/B/C ownership and readiness policy:
  [`docs/operator/target-proof-abc-governance.md`](target-proof-abc-governance.md)

## Preconditions

- run from repo root
- use the repository virtualenv:
  - `fprime-venv/bin/python`
- keep stock `fprime-gds` as the fallback / engineering surface
- treat manual-surface `manifest.json`, `status.json`, and
  `secure-state/<band>.json` as the source of truth

Default roots:

- hosted surface: `/tmp/manual-dual-gds/hosted`
- target ground surface: `/tmp/manual-dual-gds/target-ground`
- target baseline snapshot: `/tmp/manual-dual-gds/target-baseline`
- mission console runtime: `/tmp/mission-console-phase1`

Common environment overrides:

- `MISSION_CONSOLE_HOSTED_ROOT`
- `MISSION_CONSOLE_TARGET_GROUND_ROOT`
- `MISSION_CONSOLE_TARGET_BASELINE_ROOT`
- `MISSION_CONSOLE_ROOT`
- `MISSION_CONSOLE_PORT`

Target/lab network overrides when `.local` is not the right route:

- `OBC_SSH_TARGET`
- `SUBSYSTEM_SIM_SSH_TARGET`

## Recommended Startup Order

Do not let Mission Console be the first thing you start. Phase 1 layers a Flask
operator UI above the maintained manual dual-GDS authority path; it does not
replace that baseline.

Choose exactly one branch:

- hosted local demo path
- target/lab path

### Hosted Branch

Use this when you want the fastest self-test or oral-defense demo path with the
least hardware dependency.

1. Return the local machine to a clean ground-ready state.

```bash
bash scripts/ensure_ground_dual_gds_baseline.sh
```

2. Start the hosted manual surface.

```bash
GDS_UI_MODE=ui \
bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

3. Open a second terminal and confirm the hosted surface is ready.

```bash
bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh
cat /tmp/manual-dual-gds/hosted/manifest.json
```

Expected truth:

- lifecycle is `running`
- shared hosted OBC runtime is up
- fresh `GROUND_LINK_UP` has already been observed by the hosted owner

If you launched under a custom root, keep using that same root for later
status/stop commands:

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted-demo \
bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh
```

### Target/Lab Branch

Use this when you need the maintained target parity path rather than the hosted
demo path.

1. If your target network is not using the default mDNS names, export the
current SSH targets first.

Example:

```bash
export OBC_SSH_TARGET=operator@obc.local
export SUBSYSTEM_SIM_SSH_TARGET=operator@subsystem.local
```

2. Prepare the shared target baseline through the canonical `A` manager.

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
```

3. Return the local machine to a clean ground-ready state through the canonical
`B` manager.

```bash
bash scripts/ensure_ground_dual_gds_baseline.sh
```

4. Start the local target baseline snapshot wrapper.

```bash
bash scripts/manual_ops/target/start_target_manual_baseline.sh
```

5. Start the local target dual-GDS ground surface.

```bash
GDS_UI_MODE=ui \
bash scripts/manual_ops/target/start_target_manual_ground_surface.sh
```

6. Confirm the target surface is ready.

```bash
bash scripts/manual_ops/target/status_target_manual_baseline.sh
bash scripts/manual_ops/target/status_target_manual_ground_surface.sh
cat /tmp/manual-dual-gds/target-baseline/manifest.json
cat /tmp/manual-dual-gds/target-ground/manifest.json
```

Expected truth:

- target baseline snapshot exists
- target ground lifecycle is `running`
- the local ground surface has attached to the maintained target path
- the surface manifest exposes valid `guiUrl`, `gdsPort`, `gdsTtsPort`, and
  `southbound` entries

If you launched the ground surface under a custom root, keep using that same
root for later status/stop commands:

```bash
MANUAL_TARGET_GROUND_ROOT=/tmp/manual-dual-gds/target-ground-demo \
bash scripts/manual_ops/target/status_target_manual_ground_surface.sh
```

## Start Mission Console

Run the Flask app from repo root:

```bash
fprime-venv/bin/python scripts/mission_console/app.py
```

Default local URL:

- `http://127.0.0.1:5080`

If you want an isolated runtime root or a different local port:

```bash
MISSION_CONSOLE_ROOT=/tmp/mission-console-phase1-demo \
MISSION_CONSOLE_PORT=5081 \
fprime-venv/bin/python scripts/mission_console/app.py
```

## Open The UI

Open the browser at:

- `http://127.0.0.1:5080`

If you overrode `MISSION_CONSOLE_PORT`, use that port instead.

Mission Console and stock GDS remain separate surfaces. If you still need the
stock GDS fallback, open the `guiUrl` entries from the manual-surface
`manifest.json`.

## Pages

- `/`: structured dashboard cards plus recent gateway-owned event timeline
  - includes `Clear context cache` for the selected context
  - includes `Clear current view` for the selected context/band recent-event ring
- `/beacon`: beacon-only page sourced from manual-surface beacon capability
  - `/dashboard` keeps only `Last Beacon Time` and `Sequence`
  - provenance, decode state, and history stay on `/beacon`
- `/ops`: auth, searchable command workspace, upload, sequence actions, advanced raw fallback
- `/trends`: bounded live charts for curated EPS / ADCS / Health channels
- `/readback`: saved viewer tabs over the latest structured readback cache
  - each saved card can re-run its own source `GET_*` / status command with the per-card refresh icon
- `/surfaces`: read-only surface registry, dictionary/manifests, and action history
- `/sequences`: governed sequence draft / compile / upload / validate workspace
- `/packet-lab`: bounded replay / stale-session / sequence / MAC error demo with structured evidence

If this session is specifically a beacon-viewer demo rather than a Route 1
payload demo, use:

- [`docs/operator/thesis-demo-routes.zh-TW.md`](thesis-demo-routes.zh-TW.md)

## `/ops` Workspace Quick Reference

Use `/ops` as the primary operator page.

Normal mission-facing flow:

1. keep `Show all commands` off
2. type a prefix such as `mode`, `comm`, `payload`, `boot`, or `watchdog`
3. select the command from the filtered result list
4. fill the generated parameter fields
5. submit and watch the structured result panel

What to expect:

- `MODE_GET` and `MODE_SET` appear quickly when searching `mode`
- `COMM_SET_ACTIVE` appears when searching `comm` and renders one band selector field
- `PAYLOAD_CAPTURE_RAW` renders two fields with type hints
- engineering-heavy commands stay hidden until `Show all commands` is enabled
- `Advanced Raw Command` remains available for weakly inferred or uncurated commands

Use `/surfaces` if you need to confirm which active
`AppTopologyDictionary.json` the current context is using.

## First Interactive Smoke Flow

After the baseline and Flask app are both up, the shortest operator smoke path
is:

1. Go to `/ops`.
2. Choose the correct `contextId` and `band`.
3. Press auth for the primary band:
   - hosted: `hosted-manual-dual-gds` + `sband`
   - target: `target-manual-ground-dual-gds` + `sband`
4. Stay on `/ops` and run `MODE_GET`.
5. Confirm `/readback` now shows the saved `OBC` result and `/` dashboard updates.

If auth fails, stop and inspect the underlying manual surface first. Mission
Console is not meant to repair a broken baseline silently.

## Recommended Manual Self-Test Before Submission

### Hosted Path

This is the cleanest operator-owned manual test sequence:

1. authenticate on `sband`
2. run `MODE_GET` from `/ops`
3. confirm `/readback` saved viewer updates
4. open `/trends` and confirm EPS / ADCS / Health charts move
5. run `PAYLOAD_GET_STATUS`
6. use `/sequences` to compile or upload a sample sequence, then run `SEQ_VALIDATE`
7. run one packet-lab case
8. send `COMM_SET_ACTIVE`
9. confirm S-band session invalidates
10. re-auth on `uhf-primary-after-failover`

This matches the maintained hosted probe intent closely enough that a manual run
should feel familiar when you compare against the automated closeout evidence.

### Target/Lab Path

Use the target path only after the hosted path is already clean.

Recommended order:

1. authenticate on `sband`
2. run `GET_HW_WATCHDOG_STATUS`
3. run `BOOT_STATUS`
4. run `GET_PERSISTENT_FAULT_HISTORY`
5. run one packet-lab case
6. confirm the dashboard still tracks the session and recent action result

This mirrors the maintained target parity slice that closed in the repository
probe path.

## Demo-Oriented Notes

### Dashboard Demo

Use `/` to explain the contribution visually instead of pointing the committee
at raw telemetry flood:

- `Satellite Status`
- `EPS Snapshot`
- `ADCS Snapshot`
- `Mission State`
- `Comm State`
- `Secure Session`
- `Sequence & Ops`
- `Payload & Artifacts`
- `Health Snapshot`

`Satellite Status`, `EPS Snapshot`, and `ADCS Snapshot` are the first cards to
use when you want to point directly at current OBC and subsystem truth without
dragging autonomy breadcrumbs into the main story:

- `SYS_MODE`
- `GPS_SOURCE_MODE`
- `EPS_PDU_STATUS`
- `PAYLOAD_STATE`
- `ADCS_MODE`
- `ADCS_Q0`
- `ADCS_OMEGA_X`
- `EPS_VBAT`
- `EPS_IBAT`

### Command / Readback Demo

Use `/ops` and `/readback` together:

1. authenticate
2. in `/ops`, search by prefix, select a mission-facing command, and fill the generated fields
3. in `/readback`, review the saved structured result for that family
4. open `Proof / Debug Details` only when you need proof/fallback evidence rather than operator truth

Recommended examples:

- `MODE_GET`
- `COMM_SET_ACTIVE`
- `PAYLOAD_GET_STATUS`
- `GET_HW_WATCHDOG_STATUS`
- `BOOT_STATUS`
- `GET_PERSISTENT_FAULT_HISTORY`

Useful UX talking points during a demo:

- the command list comes from the active context dictionary, not a hand-maintained static menu
- mission-facing groups stay visible by default while engineering commands remain available behind `Show all commands`
- structured readback and packet-lab panels keep raw identifiers visible without forcing the operator to read raw JSON first

### Band Switch Demo

Use `/ops` to send `COMM_SET_ACTIVE` explicitly.

Expected behavior:

- Mission Console does not auto-switch for you
- after switch, the old band session is invalidated
- you must authenticate again on the new active band

This is intentional Phase 1 behavior, not a missing feature.

### Packet-Lab Demo

Use `/packet-lab` only as a bounded demo / diagnostic surface.

Recommended flow:

1. first prove ordinary auth + command/readback on the real path
2. then run one negative packet case
3. point at the packet summary highlight and observed evidence

Bounded cases:

- `replay-captured-raw`
- `replay-stale-session`
- `duplicate-sequence`
- `tampered-sequence`
- `tampered-mac`

Current closeout expectation:

- the maintained proof accepts either:
  - `packet-lab=explicit-reject`
  - `packet-lab=bounded-no-op`

Observed verdicts:

- `explicit-reject`: best demo result; the console observed explicit reject
  evidence that can be tied back to the injected malformed packet
- `bounded-no-op`: still acceptable negative proof when there is no explicit
  reject event, but the console can still show the packet was not accepted
- `inconclusive`: not demo-closeable; stop and inspect observability

Important retry rule:

- if `duplicate-sequence` or `tampered-sequence` fails during priming because
  no acceptance evidence appears, Mission Console now invalidates that session
  on purpose
- re-auth before retrying packet-lab on that band

## Troubleshooting Order

When something looks wrong, inspect these in order:

1. `status.json` of the manual surface
2. `manifest.json` of the manual surface
3. `secure-state/<band>.json`
4. Mission Console `/surfaces` and confirm the expected dictionary path / ports / lifecycle
5. stock GDS fallback UI or logs

For target/lab specifically, if the surface itself is not healthy, rerun:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
```

Do not debug Mission Console as though it were the primary source of truth; the
maintained manual dual-GDS owner still owns the baseline contract.

## Authority Boundaries

- Mission Console sends formal operations only through the maintained
  `scripts/manual_ops/manual_secure_ops.py` action layer.
- Mission Console does not intercept the stock GDS command tab.
- `COMM_SET_ACTIVE` remains an explicit operator command; the console does not
  auto-switch bands.
- `packet-lab` is lab/demo-only and still reuses the current TTS path instead
  of introducing a second uplink authority.

## Shutdown

Stop Mission Console with `Ctrl+C` in the Flask terminal.

Then stop the baseline branch you started.

Hosted:

```bash
bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh
```

Target ground surface:

```bash
bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh
```

Target baseline snapshot cleanup:

```bash
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```

The target baseline stop step retires local metadata and manual preflight
overrides; it does not claim ownership of shutting down shared remote target
services.
