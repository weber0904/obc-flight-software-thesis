## Why

Recent workflow wording updates introduced two problems: the formal `delivery-workflow` spec was not updated alongside the narrative and onboarding surfaces, and the new text blurred the repository's actual commit-boundary rule. The repository needs a formal, reviewable correction that preserves rollback checkpoints during local development while keeping the first-push single-commit rule explicit.

## What Changes

- Add a formal `delivery-workflow` requirement that distinguishes local rollback commits from the single Conventional Commit required before the first push.
- Clarify that review follow-ups after a branch is published default to focused fix commits rather than rewriting published history.
- Align the narrative workflow document, `AGENTS.md`, and the repo-local closeout skill with the same terminology and rule strength.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `delivery-workflow`: Clarify complex-change commit boundaries, first-push single-commit expectations, and post-push review-fix behavior.

## Impact

- Affected formal spec: `openspec/specs/delivery-workflow/spec.md`
- Affected narrative and onboarding surfaces:
  - `obc-dev-spec/08_delivery_workflow.md`
  - `AGENTS.md`
  - `.codex/skills/change-closeout/SKILL.md`
- No runtime code, external APIs, or product behavior changes.
