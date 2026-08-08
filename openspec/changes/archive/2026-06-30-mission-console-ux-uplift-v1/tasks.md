## 1. Backend Catalog And Metadata

- [x] 1.1 Add a Mission Console command catalog loader that reads `AppTopologyDictionary.json`, caches it by path plus mtime, resolves enum and alias-backed parameter metadata, and merges a curated overlay.
- [x] 1.2 Add `GET /api/command-catalog` plus any page boot metadata needed for readback grouping or sequence-form field visibility.

## 2. Mission Console UI Uplift

- [x] 2.1 Rebuild `/ops` into a searchable command workspace with mission-first grouping, schema-driven parameter forms, and a raw fallback path while keeping the existing command POST API unchanged.
- [x] 2.2 Replace JSON-first rendering on `/`, `/readback`, `/packet-lab`, and `/surfaces` with shared structured renderers, updated templates, and a single shared Mission Console styling layer.

## 3. Verification And Documentation

- [x] 3.1 Extend `scripts/test_mission_console_phase1.py` for catalog loading, overlay precedence, enum resolution, visibility, and route coverage without regressing existing packet-lab and auth behavior.
- [x] 3.2 Update the Mission Console operator documentation to match the new command workspace and rerun the required Mission Console local, hosted, and target verification flows.
