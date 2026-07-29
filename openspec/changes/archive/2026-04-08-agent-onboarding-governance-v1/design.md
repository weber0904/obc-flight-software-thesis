## Context

The repository already has a strong formal workflow, but that workflow is distributed across `README.md`, `obc-dev-spec/08_delivery_workflow.md`, `openspec/specs/`, `evidence/verification-path-registry.md`, and repo-local skills. Humans who have followed the project over time can reconstruct the intended order, but new AI agents currently have no single repo-root document that says "start here, then follow these formal sources." The result is repeated re-discovery, path confusion, and workflow drift.

## Goals / Non-Goals

**Goals:**
- create a single repo-root onboarding entrypoint for future agents
- make that entrypoint explicitly subordinate to the formal workflow and spec documents rather than a competing rulebook
- add a narrow repo-local checker so the entrypoint cannot silently drift away from the required formal sources
- update existing repo entry docs to reference the new entrypoint clearly

**Non-Goals:**
- creating parallel agent-specific files such as `CLAUDE.md`
- moving the real workflow rules out of `openspec/specs/` or narrative docs
- changing branch, PR, CI, or OpenSpec semantics beyond making the onboarding path explicit
- adding automation for every possible agent platform

## Decisions

### Decision: Use one repo-root `AGENTS.md` as the universal onboarding surface

The repository should have one checked-in `AGENTS.md` at the root. That file should be short, directive, and link outward to canonical sources such as `README.md`, `delivery-workflow`, the verification-path registry, and the repo-local skills.

Alternative considered:
- add multiple agent-specific files (`CLAUDE.md`, `agents.md`, etc.)
  - rejected because it would create governance drift and duplicate instructions

### Decision: Keep `AGENTS.md` as an index, not a second workflow spec

`AGENTS.md` should summarize the repo purpose and required workflow, but the canonical details must remain in the existing formal specs and narrative docs. The file should point agents to those sources instead of re-encoding every rule independently.

Alternative considered:
- copy all workflow details into `AGENTS.md`
  - rejected because it would create a second maintenance-heavy governance document

### Decision: Add a narrow entrypoint checker

The first checker should verify that `AGENTS.md` exists and that it still references the required checked-in sources. It should not try to semantically lint the entire markdown file.

Alternative considered:
- validate the entire prose body of `AGENTS.md`
  - rejected because it would be brittle and would over-constrain helpful wording changes

## Risks / Trade-offs

- [Risk] `AGENTS.md` could drift into a second workflow spec. -> Mitigation: keep it short and explicitly subordinate to the formal sources.
- [Risk] The checker could become too shallow to be useful. -> Mitigation: lock it to the critical required references only.
- [Risk] Future contributors may still look only at `README.md`. -> Mitigation: update repository entrypoint documents to point back to `AGENTS.md`.

## Migration Plan

1. Add `AGENTS.md` with explicit references to canonical workflow and validation sources.
2. Add a narrow repo-local checker for the required onboarding references.
3. Update `README.md`, `.github/README.md`, `docs/README.md`, and `scripts/README.md` as needed so they point back to the new entrypoint.
4. Add the delivery-workflow delta spec, validate, archive, and commit.
