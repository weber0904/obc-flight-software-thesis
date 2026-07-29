## Context

Recent verification work exposed a recurring failure mode: a new conversation could follow generic F' expectations and still choose the wrong validation path for this repository. The repository already has strong evidence under `docs/test-records/` and a formal delivery workflow, but there is no concise registry that says which paths are actually proven, what they prove, and which adjacent paths remain out of scope.

The same period also showed a process gap around branch discipline. The narrative workflow expected `feature/*`, `fix/*`, `docs/*`, or `hotfix/*` branches for formal work, yet recent feature slices were landed directly on `main`. The existing delivery documents already define push/CI/release gates; this change needs to make branch usage and evidence-path reuse equally explicit.

## Goals / Non-Goals

**Goals:**
- Create a repo-owned verification-path registry that names each formally proven command, telemetry, event, and ground-link path together with its scope and governing evidence.
- Strengthen the delivery workflow so formal changes begin on a dedicated branch instead of direct development on `main`.
- Require future evidence and debugging notes to name the exact path under test and distinguish it from adjacent but different paths.
- Preserve a lightweight, readable workflow so new conversations can re-anchor quickly without re-reading every archived change.

**Non-Goals:**
- Rework existing simulator, GDS, or command implementations.
- Introduce a new CI workflow or additional automated checks in this slice.
- Rewrite historical evidence files beyond the minimum additions needed to cite the new registry.

## Decisions

### Decision: Add a dedicated verification-path registry document

The repository will add a single documentation page under `docs/` that lists formally proven validation paths, their transport/port layering, and the archived evidence that proves each path. This is more direct than expecting every new conversation to infer the answer from scattered `docs/test-records/*` files.

Alternative considered:
- Rely only on `docs/test-records/` and `verification-debugging-lessons.md`.
  - Rejected because those files preserve history well but do not act as a quick, normative registry of proven paths.

### Decision: Treat repository evidence as the authority for path reuse

The delivery workflow and verification-evidence specs will explicitly say that generic upstream F' knowledge is not enough to declare a path “already proven” for this repository. Reuse of a path must cite repository evidence or the new registry.

Alternative considered:
- Keep this as an informal reviewer expectation.
  - Rejected because the recent CLI/TTS confusion showed that informal expectations are too easy to bypass.

### Decision: Make branch discipline explicit in the formal workflow

The narrative workflow already implied branch-based work, but the delivery-workflow spec did not state it clearly enough. This change will document that formal work starts from a dedicated branch and that `main` is for reviewed, green, merged history only.

Alternative considered:
- Leave branch discipline only in narrative docs.
  - Rejected because the mismatch between narrative guidance and recent practice is the exact problem being corrected.

## Risks / Trade-offs

- [Risk] The registry could become stale if later changes forget to update it. → Mitigation: require evidence and debugging records to cite the registry or archived evidence when reusing a path.
- [Risk] Additional workflow text could feel repetitive. → Mitigation: keep the registry concise and let archived evidence remain the detailed source.
- [Risk] This slice is governance-heavy. → Mitigation: anchor every rule to a concrete failure mode already seen in repository history.
