## Context

`docs/architecture-review/` and `docs/reporting/` were produced as checked-in review artifacts for earlier repository states. They still have historical and review value, but their wording can imply that the package is always the current normative baseline. That creates risk for future agents because the active baseline can move through code, topology, scripts, archived OpenSpec changes, evidence records, main specs, and the verification-path registry after a review package was generated.

The change introduces `docs/planning/` as an intentionally non-normative space for roadmap notes and handoff planning. Planning notes are useful for sequencing multiple future changes, but they must remain below formal sources and must be allowed to become stale.

The governing truth priority for this change is:

1. code, topology, and scripts
2. archived OpenSpec changes and `evidence/records/`
3. current main specs under `openspec/specs/`
4. `evidence/verification-path-registry.md`
5. narrative, snapshot, reporting, and planning docs

## Goals / Non-Goals

**Goals:**

- Add a clear non-normative planning directory with a required note structure.
- Add a COMM roadmap note reconciled against current formal COMM specs and evidence.
- Mark architecture-review and reporting package indexes as point-in-time snapshots unless explicitly refreshed.
- Update specs so the snapshot warnings are consistent with formal requirements.

**Non-Goals:**

- No runtime, topology, script, simulator, or component behavior changes.
- No deletion or relocation of `docs/architecture-review/` or `docs/reporting/`.
- No new validation path and no claim that physical COMM downlink, RF, file/downlink, shared CAN FD COMM participation, target OBC migration, or ScenarioBridge behaviors are proven.
- No broad revival of `ttc-over-comm-end-to-end-v1` as a single catch-all change.

## Decisions

- Create `docs/planning/` instead of reusing `docs/architecture-review/` for roadmaps.
  - Rationale: architecture-review packages are review snapshots, while roadmaps need to carry active and deferred follow-up sequencing without becoming baseline truth.
  - Alternative considered: update the architecture-review package directly as the active roadmap. That would keep the stale-current ambiguity in place.

- Require every planning note to declare freshness and authoritative sources.
  - Rationale: planning notes can be useful even when stale, but stale status must be visible at the top of each note.
  - Alternative considered: rely on git history. That is weaker for future agents because it requires reconstructing intent from commit timing.

- Keep architecture-review and reporting packages in place with prominent warnings.
  - Rationale: existing links, baseline reconciliation records, and review history may reference these paths. Deleting or moving them would create avoidable churn.
  - Alternative considered: move them out of the repository. That would break checked-in review traceability and contradict the requested boundary.

- Update the specs to allow point-in-time package freshness status.
  - Rationale: README warnings are not enough if formal specs still require those packages to be always-current baseline summaries.
  - Alternative considered: only edit README files. That would leave a governance contradiction.

## Risks / Trade-offs

- A future agent may still quote an old snapshot without reading its warning -> Mitigation: put warnings near the top of each package index and require freshness declarations in specs.
- Planning notes may become stale -> Mitigation: staleness is allowed, but each note must record the last reconciled main commit/date and formal sources checked.
- A roadmap can make future scope look more committed than it is -> Mitigation: the planning README and COMM roadmap explicitly say formal sources win and the note is not a formal baseline.
