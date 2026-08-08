## Context

Mission Console Phase 1 already owns a repo-controlled Gateway, listener, and
snapshot layer, but the frontend still mirrors backend payloads almost
verbatim. The existing `/ops` page requires free-text command names and
space-separated arguments, while the other primary pages render raw JSON blocks
that make operator state and thesis-demo evidence harder to read.

This tranche keeps the current authority boundaries intact:

- Flask plus server-rendered HTML plus vanilla JS remains the frontend stack.
- Existing POST action APIs and `manual_secure_ops` authority semantics remain
  unchanged.
- The new UX layer must consume the existing Gateway truth instead of
  introducing a second command authority or parallel planner model.

## Goals / Non-Goals

**Goals:**

- Add a context-scoped command catalog backed by
  `AppTopologyDictionary.json` plus a curated Mission Console overlay.
- Turn `/ops` into a searchable mission-first command workspace with
  schema-driven parameter forms and a raw fallback path.
- Replace JSON-first rendering on the dashboard, readback, packet-lab, and
  surfaces pages with structured operator views.
- Preserve the existing backend job and authority flows so the UX uplift stays
  compatible with hosted and target Phase 1 proofs.

**Non-Goals:**

- Replacing stock `fprime-gds`
- Adding mission scheduling or pass-planner persistence
- Introducing React, Vite, WebSockets, or a UI automation framework
- Changing secure auth, command, upload, sequence, or packet-lab backend
  authority semantics

## Decisions

### 1. Add a dedicated gateway catalog module

The command metadata loader will live in a new Mission Console gateway module
instead of being embedded in `app.py` or the frontend. This keeps the
dictionary parser testable and allows the same catalog shape to serve both the
UI and future planner work.

Alternative considered:

- Parse the dictionary directly in the browser. Rejected because the
  dictionary lives in per-context build artifacts, the repo already has a
  Python-owned gateway boundary, and backend tests need deterministic catalog
  coverage.

### 2. Use dictionary JSON as broad truth and a curated overlay for operator UX

The dictionary is the broad source of truth for command names, opcodes, kinds,
and formal parameters. A curated overlay adds mission-first grouping,
visibility, descriptions, richer field hints, and a small number of explicit
field defaults or labels.

Alternative considered:

- Curate the full command set by hand. Rejected because it would drift quickly
  and would unnecessarily hide dictionary-backed engineering commands that are
  still useful behind a show-all toggle.

### 3. Keep existing POST action payloads unchanged

The frontend will serialize schema-driven field values into the current
`commandArgs` array and keep the existing upload, readback, sequence, and
packet-lab POST routes unchanged. This isolates the UX uplift from the verified
operator authority layer.

Alternative considered:

- Introduce richer structured command payloads per command. Rejected for this
  tranche because it would expand backend semantics beyond the approved scope.

### 4. Centralize rendering helpers in one shared JS/CSS layer

The page upgrade will use one shared JS bundle and one shared CSS file with
render helpers for badges, key/value lists, tables, timelines, and collapsible
raw sections. This preserves the lightweight Phase 1 stack while avoiding a
template-per-widget duplication pattern.

Alternative considered:

- Heavily custom per-page inline rendering. Rejected because it would duplicate
  formatting logic and make later polish or maintenance harder.

## Risks / Trade-offs

- [Dictionary metadata is incomplete for some engineering commands] ->
  Fallback to raw/text inputs for unsupported complex types and keep a visible
  raw fallback path.
- [Curated grouping can misclassify commands as the command surface evolves] ->
  Base visibility on a small prefix/group rule set and allow show-all access to
  the full dictionary-backed catalog.
- [Structured renderers can hide details operators still need] -> Keep raw
  identifiers visible in the structured views and retain collapsible raw JSON or
  hex details for secondary inspection.
- [Frontend uplift could accidentally perturb verified backend flows] ->
  Preserve POST APIs, extend backend coverage for the catalog route and schema
  logic, and rerun hosted plus target Mission Console proofs after the UI work.

## Migration Plan

1. Add the backend catalog loader and `GET /api/command-catalog`.
2. Rebuild the shared JS and templates to consume the catalog and structured
   page payloads.
3. Extend `scripts/test_mission_console_phase1.py` for catalog and metadata
   coverage.
4. Update the Mission Console operator runbook so human operators know how to
   use the new command workspace.
5. Rerun the existing Mission Console local, hosted, and target verification
   paths.

## Open Questions

- The current tranche will keep English UI labels, but later work may still
  want curated operator copy for a thesis-demo-specific skin.
- Some engineering commands may benefit from richer curated parameter
  descriptions in a later tranche, but that is not required to land this v1
  uplift.
