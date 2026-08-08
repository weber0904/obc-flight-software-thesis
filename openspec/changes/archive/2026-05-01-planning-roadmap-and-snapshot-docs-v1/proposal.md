## Why

The repository has checked-in architecture review and reporting packages that were accurate when produced but may now lag current `main`. Future agents need a governed place for non-normative planning notes and explicit freshness warnings so stale snapshots are not mistaken for current baseline truth.

## What Changes

- Add `docs/planning/` as a non-normative planning, roadmap, and multi-change handoff space.
- Add a COMM roadmap planning note based only on current formal specs, archived evidence, and the verification-path registry.
- Mark `docs/architecture-review/` and `docs/reporting/` as point-in-time snapshot/review packages unless explicitly refreshed by a future governed change.
- Update affected specs so snapshot package README warnings do not contradict formal requirements.
- Keep the existing architecture-review and reporting package files in place with links intact.

## Capabilities

### New Capabilities
- `planning-docs`: Non-normative planning notes and multi-change handoff roadmaps, including required freshness metadata and conflict handling.

### Modified Capabilities
- `architecture-review`: Allow checked-in architecture-review packages to be point-in-time snapshots with explicit freshness status instead of always-current baseline sources.
- `project-reporting`: Allow checked-in reporting packages to be generated point-in-time review artifacts with explicit freshness status and refresh expectations.

## Impact

- Documentation and OpenSpec governance only.
- No runtime code, topology, scripts, interfaces, external dependencies, or validation paths change.
- Validation is limited to OpenSpec checks, repo consistency checks, and whitespace checks.
