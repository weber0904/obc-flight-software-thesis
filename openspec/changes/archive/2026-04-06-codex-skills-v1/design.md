## Context

The repository now has enough repeated experience across `gps-subsystem-v1`, `storage-health-v1`, and `copilot-followup-fixes-v1` to extract two narrow Codex skills:

- change closeout
- repository-owned hosted probe workflow

Those three rounds showed that:

- closeout behavior is stable across new feature slices and merged-review follow-ups
- repository-owned hosted probes follow a common hosted-first pattern, but should remain narrow and not absorb every testing concern
- a large `subsystem-slice` skill is still premature

The repository already has formal workflow and verification governance in:

- `obc-dev-spec/08_delivery_workflow.md`
- `openspec/specs/delivery-workflow/spec.md`
- `evidence/verification-path-registry.md`
- `docs/verification-debugging-lessons.md`

The new skills should point agents at those sources instead of duplicating them in large reference packs.

## Goals / Non-Goals

**Goals:**

- Add a narrow `change-closeout` skill that encodes the repository's governed closeout order.
- Add a narrow `hosted-probe-workflow` skill that encodes the repository's hosted-probe design and rerun rules.
- Keep both skills small, workflow-based, and grounded in existing repository documents.
- Generate skill metadata in Codex-compatible format and validate the resulting skill folders.

**Non-Goals:**

- Do not create a giant `subsystem-slice` or `component/topology/CMake/tests` super-skill.
- Do not move repo workflow knowledge out of the existing repository documents.
- Do not add new probe scripts or new runtime capabilities as part of this change.

## Decisions

### 1. Create exactly two small workflow skills

The change creates only:

- `change-closeout`
- `hosted-probe-workflow`

This matches the strongest overlap across the recent dev logs and avoids premature abstraction.

Alternative considered:
- Build a single large "repo-development" skill.

Why rejected:
- The recent dev logs explicitly show that large catch-all skills are not yet justified.
- Overly broad skills would be harder to trigger correctly and easier to let drift.

### 2. Keep the skills instruction-only in v1

The skills will primarily consist of `SKILL.md` plus generated `agents/openai.yaml`.

Alternative considered:
- Add bundled scripts or large reference files immediately.

Why rejected:
- The current workflows are already anchored in repository documents and scripts.
- The immediate need is repeatable guidance and triggering, not new automation code.

### 3. Use the official Codex skill tooling

Initialize the skill folders with `init_skill.py`, generate `agents/openai.yaml`, and validate with `quick_validate.py`.

The skill frontmatter intentionally keeps only `name` and `description`, because the Codex skill-creator guidance treats those as the canonical frontmatter fields and places UI metadata in `agents/openai.yaml`.

Alternative considered:
- Create the folders by hand and stop at `SKILL.md`.

Why rejected:
- The user explicitly asked for standard-format skills.
- Using the official tooling is the cleanest way to keep format drift low.

## Risks / Trade-offs

- [Risk] The skills may become stale if the repository workflow changes.
  → Mitigation: point the skills at the repository source documents instead of duplicating every rule inline.

- [Risk] The hosted-probe skill may over-trigger for generic testing requests.
  → Mitigation: keep the description narrow and centered on repository-owned hosted probes.

- [Risk] The closeout skill could be misused before implementation is actually complete.
  → Mitigation: make the trigger and guardrails explicitly post-implementation.
