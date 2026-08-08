## Context

GitHub required checks can remain pending when an entire workflow is skipped by workflow-level path filtering, so the repository should not use `paths` or `paths-ignore` to avoid CI for docs-only pull requests. The current safer design already keeps one required `baseline-gate` job and classifies inside the workflow, but the allowlist is too narrow: only `docs/architecture-review/**` can avoid the F' build.

## Goals / Non-Goals

**Goals:**

- Keep `baseline-gate` as the single required job that always starts for pull requests.
- Add a conservative `lightweight` mode for documentation and governance-only changes.
- Run OpenSpec and static governance checks in lightweight mode without requiring F' venv, submodules, apt build dependencies, or F' build output.
- Make active OpenSpec change workspaces, workflow files, scripts, code, build config, and unknown paths default to full verification.

**Non-Goals:**

- No change to branch protection settings outside the repository.
- No workflow-level `paths` or `paths-ignore` filters.
- No new product/runtime validation path.
- No disposable proof PR after merge; the next real docs/governance PR will exercise the lightweight path.

## Decisions

- Use one `lightweight` mode instead of separate prose and governance modes.
  - Rationale: static governance checks are fast and avoid edge cases where prose changes also touch generated governance surfaces.

- Implement classification in `scripts/classify_change_scope.sh`.
  - Rationale: CI YAML stays small, and developers can test classification locally with changed-file lists.

- Use a whitelist strategy.
  - Rationale: new paths should never skip build/test until explicitly reviewed and added to the safe set.

- Keep Python, Node, and OpenSpec setup outside the full-only branch.
  - Rationale: lightweight mode still runs Python static checks and `openspec validate --specs`.

- Keep submodules, system build packages, and F' venv creation full-only.
  - Rationale: lightweight mode does not build, run F' tests, or invoke `fprime-util`.

## Risks / Trade-offs

- Lightweight classification could become too broad.
  - Mitigation: only explicit documentation/governance paths are allowed; unknown paths, active change workspaces, scripts, and workflows return `full`.

- Lightweight mode could miss formatting drift.
  - Mitigation: `git diff --check` remains a local closeout check; CI lightweight focuses on structural governance checks to avoid PR diff-range complexity.

- This PR cannot prove lightweight in hosted CI because it edits workflow/scripts.
  - Mitigation: local classifier and lightweight-mode checks prove behavior, and hosted CI should intentionally run full for this change.
