## Overview

This change adds a checked-in technical architecture-review package for the repository's current `main` baseline. The package is distinct from the existing professor/PM-facing `project-reporting` package: it is optimized for technical release judgement, architecture audit, and future agent continuity rather than for a short presentation first.

## Design Decisions

### Truth Priority

The package is grounded in a fixed source priority:

1. code / topology / build / test inventory
2. archived OpenSpec changes + archived evidence
3. current main specs
4. narrative docs / reporting docs
5. git history / tags

When sources disagree, the package states the drift explicitly instead of smoothing it over.

### Package Shape

The package is split into four checked-in documents under `docs/architecture-review/current-baseline-assessment-v1/`:

1. `README.md`
2. `subsystem-and-capability-dossiers.md`
3. `verification-and-release-readiness.md`
4. `workflow-governance-audit.md`

An additional checked-in `thesis-summary-export.md` acts as the repo-owned source for repo-external export into thesis notes.

### Capability Scope

This slice introduces a new `architecture-review` capability instead of overloading `project-reporting`.

That separation is intentional:

- `project-reporting` remains professor/PM/demo oriented
- `architecture-review` is technical-audit and release-readiness oriented

Keeping them separate prevents future reporting changes from mixing audience layers, release judgement, and demo storytelling into one capability.

### Report Content Rules

The architecture-review package must:

- summarize current architecture and scope
- describe each subsystem/capability with a fixed template
- distinguish implemented, constrained, future, and retired items
- produce an explicit release judgement
- audit whether repo-only context is enough for future agents to maintain workflow quality

### Evidence Strategy

This change does not prove a new runtime validation path. Its evidence record instead captures:

- the non-mutating repo checks rerun to support the assessment
- the repository truth sources consulted
- the package artifacts produced
- the bounded conclusion that the current release recommendation is documentation derived from existing evidence rather than a new runtime proof

### Discoverability

The change updates documentation indices so reviewers can discover:

- the new technical architecture-review package
- the older professor/PM-facing reporting package

This avoids turning `docs/reporting/` into the accidental home for all review material.
