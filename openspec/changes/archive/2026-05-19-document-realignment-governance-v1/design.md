# Design

This change is a documentation-governance realignment, not a runtime feature.
The design goal is to keep the repository's current-truth entrypoints aligned
with the source-priority model that already exists in the codebase and specs,
while avoiding a second competing governance system.

## Decision: Add one explicit documentation-governance capability

The current spec set already governs roadmap freshness, architecture-review
snapshot boundaries, reporting-package freshness, the interface index, and the
repo-root `AGENTS.md` entrypoint in isolated places. What is still missing is a
single capability that:

- defines the repo-root onboarding pair (`README.md` + `AGENTS.md`) as
  low-churn entrypoints
- defines current-facing narrative docs that must carry freshness metadata
- defines current non-canonical snapshot families that must carry scope and
  snapshot markers
- requires a repo-local checker for those rules

This avoids overloading `delivery-workflow` with document-family details while
still keeping the normative rules in `openspec/specs/`.

Rejected alternative:

- Put all of these rules directly into `AGENTS.md` or `README.md`. Rejected
  because those files are onboarding surfaces, not the normative spec layer.

## Decision: Keep repo-root entrypoints short and defer high-churn truth

`README.md` and `AGENTS.md` currently duplicate substantial portions of current
architecture and workflow truth. That duplication increases drift risk and
forces every baseline refresh to touch multiple large files.

The implementation will keep both files as entrypoints, but narrow them to:

- repository scope and current maintained deployment summary
- source-priority and read-first guidance
- formal workflow and validation-path entrypoints
- document-family map and update-routing rules

Detailed current-baseline facts stay in the existing canonical current-truth
docs such as `docs/architecture/current-development-architecture.md`,
`docs/interfaces.md`, `evidence/verification-path-registry.md`, and
`docs/roadmap/`.

Rejected alternative:

- Rewrite `README.md` into another full architecture narrative. Rejected because
  it would preserve the current duplication problem.

## Decision: Use family-specific metadata rules with one checker

Not every document family should carry the same markers.

- Current/canonical narrative docs need `Status:` plus a freshness or
  reconciliation marker.
- Snapshot/current-review package entrypoints need explicit scope and
  point-in-time markers so readers do not misread them as current baseline.
- Historical/archive documents do not need to be rewritten wholesale; only the
  entrypoints or misleading moved-path references need repair.

The checker will validate the minimum metadata or boundary markers per document
family instead of trying to lint every Markdown file with one generic rule.

## Decision: Integrate the checker into existing governance checks

The repository already uses `check_repo_consistency.py` and the shared
verification gate as the main static-governance enforcement surface.

The new documentation checker will be:

- a standalone script with focused, readable failures
- invoked from `check_repo_consistency.py`
- invoked by `scripts/run_verification_ci.sh` through the existing consistency
  check path

This keeps one reviewable gate instead of creating a parallel validation flow.

## Implementation Notes

1. Add `documentation-governance` delta spec with requirements for:
   - repo-root entrypoints
   - current-facing metadata
   - snapshot/non-canonical package markers
   - checker enforcement
2. Add a `delivery-workflow` delta requiring the new checker in repository
   gating.
3. Refresh current/canonical docs first, then current non-canonical package
   entrypoints, then targeted archive/index repairs.
4. Implement the checker last enough to match the final document rules, but
   early enough to use it during local verification.
