## 1. Planning Surfaces

- [x] 1.1 Add `docs/planning/README.md` with non-normative planning rules and required note fields.
- [x] 1.2 Add `docs/planning/comm-roadmap.md` reconciled against current COMM specs, registry entries, and archived evidence.

## 2. Snapshot Package Warnings

- [x] 2.1 Update `docs/architecture-review/README.md` with a prominent point-in-time snapshot warning while preserving links.
- [x] 2.2 Update `docs/reporting/README.md` with a prominent point-in-time reporting artifact warning while preserving links.

## 3. Spec Deltas And Evidence

- [x] 3.1 Prepare OpenSpec delta specs so `planning-docs`, `architecture-review`, and `project-reporting` can reflect the new freshness rules at archive.
- [x] 3.2 Record governance evidence naming the checked-in truth sources, review surfaces updated, and validation commands.

## 4. Validation

- [x] 4.1 Run `openspec validate planning-roadmap-and-snapshot-docs-v1`.
- [x] 4.2 Run `openspec validate --specs`.
- [x] 4.3 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.4 Run `git diff --check`.
