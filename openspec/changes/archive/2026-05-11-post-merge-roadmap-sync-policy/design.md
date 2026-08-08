## Context

The repository already distinguishes `local-ready`, push, PR review, merge, and CI-green completion, but it does not currently explain how agents should handle narrative files whose truthful wording depends on the already-merged `main` state. In practice, most documentation should still travel with the originating branch, while `docs/roadmap/*` is a high-frequency handoff layer that sometimes only becomes final after the merge order and mainline state are known.

The design therefore needs a narrow rule that resolves the roadmap timing ambiguity without weakening the broader review boundary for formal workflow, architecture truth, operator instructions, or verification evidence.

## Goals / Non-Goals

**Goals:**
- Tell agents which documentation layers must be updated before review on the originating branch.
- Allow `docs/roadmap/*` to be reconciled after merge when its truthful wording depends on the merged `main` state.
- Avoid forcing a second low-value PR solely for roadmap progress/order/handoff restatement.
- Keep the direct-to-`main` exception narrow and easy to audit.

**Non-Goals:**
- Creating a general direct-to-`main` bypass for repository documentation.
- Reclassifying formal specs, verification evidence, architecture truth, or operator runbooks as post-merge-only documents.
- Changing the existing push, PR, CI, or release gates for product-impacting work.

## Decisions

1. Introduce a distinct post-merge mainline reconciliation rule in `delivery-workflow`.
   - Rationale: the current `local-ready` and PR rules explain when a branch is reviewable, but not what to do when a narrative file must describe merged `main`.
   - Alternative considered: forcing all roadmap wording into the original PR. Rejected because it either produces inaccurate "already merged" wording or still requires a follow-on PR when merge order shifts.

2. Limit the post-merge shortcut to `docs/roadmap/*`.
   - Rationale: roadmap files are the repository's high-frequency sequencing and handoff layer and are already treated differently from formal truth layers.
   - Alternative considered: allowing all docs under `docs/**` to reconcile directly on `main`. Rejected because it would blur the review boundary for architecture, evidence, operator, and reporting docs.

3. Preserve the default preference for updating roadmap files inside the original branch whenever the post-merge state is stable enough to state in advance.
   - Rationale: the narrow exception should reduce unnecessary follow-on PRs, not encourage agents to defer routine documentation.
   - Alternative considered: requiring all roadmap edits to wait until after merge. Rejected because many roadmap/handoff updates are already obvious and reviewable in the feature PR.

## Risks / Trade-offs

- [Maintainer overuses the shortcut] → Mitigation: repeat the file-scope and content-scope limits in the formal spec, onboarding doc, and narrative doc.
- [Agents still defer too much documentation] → Mitigation: explicitly require branch-verifiable docs, and roadmap updates when stable, to remain in the original branch/PR.
- [Roadmap and formal truth drift] → Mitigation: keep `README.md`, `AGENTS.md`, and `docs/architecture/current-development-architecture.md` aligned on the same distinction between roadmap reconciliation and canonical truth layers.
