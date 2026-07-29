# Mission Console

Mission Console is a Flask-based operator interface over the repository's
manual S-band/UHF surfaces. It uses the active F Prime dictionary and preserves
the same authenticated command path as CLI operations.

## Start A Data Surface

For hosted operation:

```bash
GDS_UI_MODE=headless \
  bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

For target operation:

```bash
bash scripts/manual_ops/target/start_target_manual_baseline.sh
GDS_UI_MODE=headless \
  bash scripts/manual_ops/target/start_target_manual_ground_surface.sh
```

Confirm the relevant status script reports a ready surface before starting the
web application.

## Start The Application

```bash
MISSION_CONSOLE_ROOT=/tmp/mission-console \
MISSION_CONSOLE_PORT=5080 \
  fprime-venv/bin/python scripts/mission_console/app.py
```

Open `http://127.0.0.1:5080`.

## Workspaces

| Path | Purpose |
|---|---|
| `/` | mission dashboard, selected context, link state, and recent events |
| `/ops` | authentication, searchable command form, upload, and advanced command input |
| `/readback` | saved structured results with per-card refresh |
| `/sequences` | draft, compile, upload, validate, and execute F Prime sequences |
| `/trends` | bounded live EPS, ADCS, and health plots |
| `/beacon` | UHF beacon history, decode state, and provenance |
| `/surfaces` | dictionaries, manifests, contexts, ports, and action history |
| `/packet-lab` | replay, stale-session, sequence, and MAC-error experiments |

## First Operator Flow

1. Open `/ops`.
2. Select the hosted or target context and `sband`.
3. Establish authentication.
4. Search for and send `MODE_GET`.
5. Confirm the result in `/readback`.
6. Open `/trends` and confirm EPS, ADCS, and health updates.
7. Run `PAYLOAD_GET_STATUS`.
8. Open `/sequences`, compile a sample, upload it, and run `SEQ_VALIDATE`.

For a link transition, issue the applicable communication action, select
`uhf-primary-after-failover`, and establish a UHF session before sending the
next secure command.

## Payload And File Flow

The payload workspace uses the same authenticated operations as the CLI:

1. check payload readiness;
2. request capture or run the Route 1 sequence;
3. observe capture status and data-product metadata;
4. download the preview and raw files;
5. inspect the stored source and decode provenance.

## Troubleshooting

Check the layers in order:

1. manual-surface status and manifest;
2. active dictionary path;
3. GDS data and TTS ports;
4. gateway and COMM-node logs;
5. OBC link-state event;
6. authentication result and service ID;
7. command result and event timeline.

Mission Console does not restart target or ground services. Use
[Hosted Operations](hosted.md) or [Target And Lab Operations](target-lab.md) to
repair the underlying surface.

## Shutdown

Stop Mission Console with `Ctrl-C`, then stop the matching surface:

```bash
bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh
```

or:

```bash
bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```
