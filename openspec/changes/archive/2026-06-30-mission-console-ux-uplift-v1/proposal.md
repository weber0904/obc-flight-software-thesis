## Why

Mission Console Phase 1 already closes the operator-authority gap, but the
current UI still behaves like a thin JSON and form shell. Operators must know
exact command names ahead of time, manually count arguments, and read back
results from raw JSON blocks, which weakens both day-to-day usability and demo
clarity.

## What Changes

- Add a repo-owned command metadata layer that loads
  `AppTopologyDictionary.json`, caches it by path and mtime, and merges it with
  a curated Mission Console overlay.
- Add a new `GET /api/command-catalog` API that returns context-scoped command
  metadata with grouped visibility, labels, descriptions, and parameter schema
  hints.
- Rebuild `/ops` into a searchable command workspace with grouped mission-first
  command discovery, schema-driven parameter forms, and a raw fallback path for
  uncurated engineering commands.
- Replace JSON dump rendering on `/`, `/readback`, `/packet-lab`, and
  `/surfaces` with structured, mission-control-style views while preserving the
  existing Phase 1 POST action APIs and authority boundaries.
- Keep the current Flask plus server-rendered HTML plus vanilla JS architecture
  and do not introduce React, Vite, scheduler work, one-GDS aggregation, or a
  second command authority.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `mission-console`: expand the Mission Console requirement set so Phase 1
  includes dictionary-backed command discovery, schema-driven operator forms,
  and structured page rendering across dashboard, readback, packet-lab, and
  surfaces views.

## Impact

- Affected code: `scripts/mission_console/app.py`,
  `scripts/mission_console/gateway/`, `scripts/mission_console/templates/`,
  `scripts/mission_console/static/`, and
  `scripts/test_mission_console_phase1.py`.
- Affected API surface: adds `GET /api/command-catalog`; keeps all existing
  POST operator APIs unchanged.
- Affected docs/specs: Mission Console OpenSpec capability and operator
  runbooks need to reflect the new command workspace and structured views.
