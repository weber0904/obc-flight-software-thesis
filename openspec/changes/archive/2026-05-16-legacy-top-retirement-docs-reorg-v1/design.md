## Context

The repository baseline now treats `OBC/TopCcsds` plus the `OBC` deployment as
the maintained hosted and Raspberry Pi target path. The older `OBC/Top` /
`OBC_ComFprimeLegacy` path still appears in build registration, scripts, command
policy catalogs, verification inventory helpers, and current-facing documents.
That leaves a retired topology buildable enough to confuse operators and
reviewers even though recent target and UHF work has moved to the active CCSDS
packaging path.

The documentation tree has the same problem at the narrative layer: current
architecture, roadmap handoff, thesis material, historical reporting packages,
and redirect stubs are interleaved. Some roadmap files are done-heavy and some
planning redirect folders still look like active entrypoints.

## Goals

- Remove the maintained legacy `OBC/Top` / `OBC_ComFprimeLegacy` build,
  runtime, script, policy, and current-documentation surface.
- Keep archived evidence reviewable while making current indexes explicit that
  legacy Top evidence is historical only.
- Rebuild the `docs/` information architecture around active truth,
  operations, roadmap handoff, verification evidence, thesis, reporting
  archives, and retired material.
- Refresh repo-tracked thesis content so it reflects service-managed R2 process
  restart closure and the remaining hardware-watchdog reset proof gap.

## Non-Goals

- Do not rewrite old command transcripts or historical test records just because
  they cite the legacy topology.
- Do not edit external thesis notes outside the repository.
- Do not remove active `TopCcsds` dependencies that happen to use upstream
  `ComFprime` services internally, such as router or buffer-manager building
  blocks required by the active CCSDS topology.
- Do not prove new hardware behavior, RF behavior, timing/WCET closure, or
  hardware watchdog reset in this change.

## Decisions

### Hard retire the legacy deployment

The change removes the legacy topology source and deployment registration rather
than leaving a hidden build option. This makes the supported deployment boundary
clear: `OBC` is the active maintained payload and uses `TopCcsds`.

### Port only current operator surfaces

Scripts whose only purpose is legacy ComFprime regression are deleted. Current
operator or lab scripts that still reference legacy dictionary names or command
prefixes are ported to `OBC` / `OBCApp.*` when they still describe a current
validation or operations path.

### Keep historical evidence immutable

Old evidence records remain in place even when they cite deleted scripts,
commands, or binaries. Current indexes and registry entries are updated to mark
those paths as historical, and new evidence for this change records the cleanup
gate instead of editing old transcripts.

### Make docs homes explicit

Current truth remains under `docs/architecture/`, `docs/operator/`,
`docs/test-records/`, and `docs/verification-path-registry.md`. Roadmap handoff
is compressed under `docs/roadmap/`. Repo-tracked thesis material moves to
`docs/thesis/`. `docs/reporting/` becomes an index for archived or generated
reporting packages rather than the home for active thesis content. Retired
redirect folders that no longer carry useful current context are removed.

## Risks and Trade-Offs

- Removing buildable legacy sources can break stale local workflows. The trade
  is intentional: stale workflows should fail visibly instead of presenting
  legacy Top as maintained.
- Link churn is likely across docs. The mitigation is repo-wide link/reference
  searches and a new evidence record for the cleanup.
- Some historical evidence will reference scripts that no longer exist. The
  mitigation is explicit historical labeling in indexes and registry entries
  rather than transcript edits.
- Active `TopCcsds` still uses some upstream `ComFprime` service names as
  implementation details. The cleanup must distinguish those dependencies from
  the retired `OBC/Top` deployment.
