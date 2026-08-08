## Context

The merged planning roadmap identified `comm-baseline-doc-reconciliation-v1` as the next low-risk step before additional COMM runtime proof. The current formal registry now distinguishes hosted gateway-backed COMM TT&C, physical UART preflight, subsystem-origin acquisition, and physical lab serial uplink ingress, but some current-facing narrative text still implies that physical lab serial ingress is not established.

Separately, the delivery workflow says changes remain incomplete until CI is green. That is still correct, but it leaves ambiguous whether an agent should open a draft PR and whether it should keep polling CI after opening a PR. The intended collaboration pattern is to open a ready-for-review PR by default so reviewer automation can run, then report the PR/check state and stop unless the developer explicitly asks the agent to monitor or act on CI results.

## Goals / Non-Goals

**Goals:**

- Align current-facing COMM documentation with the verification-path registry.
- Preserve older architecture-review/reporting packages as point-in-time snapshots by warning rather than rewriting their historical claims.
- Record the accepted next COMM sequence in the planning roadmap.
- Add formal delivery-workflow wording that pushed review-ready work should open as a non-draft PR by default.
- Add formal delivery-workflow wording for CI-wait handoff behavior.

**Non-Goals:**

- No runtime code, topology, script, simulator, or component changes.
- No new COMM validation path.
- No claim that physical lab serial event/channel downlink, file/downlink over COMM, RF, target OBC migration, or COMM shared CAN FD participation is proven.
- No change to GitHub Actions mechanics or required CI gates.

## Decisions

- Reconcile the top-level README directly.
  - Rationale: `README.md` is current-facing onboarding material and must not contradict registered evidence.
  - Alternative considered: leave README alone because the planning roadmap is clearer. That would still let future agents start from stale current-facing text.

- Add snapshot warnings to old package roots rather than rewriting every old package paragraph.
  - Rationale: v1 packages are now explicitly point-in-time artifacts. Rewriting internal claims as if the whole package were freshly audited would be misleading unless a full package refresh is in scope.
  - Alternative considered: deeply edit the old report package. That is larger, higher-risk, and better suited to a future reporting refresh.

- Add CI-wait handoff as a formal delivery-workflow requirement.
  - Rationale: it preserves the CI-green completion rule while matching the established collaboration pattern: agent stops at pending CI unless asked to keep monitoring.
  - Alternative considered: leave it as conversational convention. That would not help future agents.

- Add ready-for-review PR creation as a formal delivery-workflow requirement.
  - Rationale: reviewer automation may only trigger on ready PRs, so draft PRs should not be the default for already reviewable branches.
  - Alternative considered: allow the PR creation tool default to decide. That is too implicit for automation-dependent review.

## Risks / Trade-offs

- Older snapshot files may still be opened directly -> Mitigation: add warnings at the package root and to the known communication realignment prep note.
- Combining COMM doc reconciliation and CI handoff governance touches two areas -> Mitigation: keep both changes documentation-only and record them under one evidence record.
- CI handoff wording could be misread as weakening the CI gate -> Mitigation: explicitly state that formal completion, merge, release, and tag still require CI green.
- Ready PR wording could be misread as banning draft PRs entirely -> Mitigation: allow draft only when the developer explicitly requests it or the branch is intentionally not review-ready.
