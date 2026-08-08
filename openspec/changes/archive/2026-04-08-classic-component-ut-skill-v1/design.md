## Context

The repository has already formalized the rule that:

- real F' components need classic L2 component UT harnesses
- helper/library logic needs direct plain L1 tests

That rule now exists in the workflow/governance layer, but future implementation work still needs a narrow skill that answers the practical question:

> when adding or modifying a real F' component, how do I build the classic harness correctly in this repository?

The user explicitly rejected a "repair missing UT later" abstraction. The skill must therefore match normal development flow: component work includes the classic harness step as part of the implementation slice.

## Goals / Non-Goals

**Goals:**

- Add a narrow repo-local skill named `classic-component-ut-pattern`.
- Teach agents how to distinguish a real component from helper/support logic.
- Point agents at repository patterns for:
  - `register_fprime_ut()`
  - `UT_AUTO_HELPERS ON`
  - `TesterBase` / `GTestBase`
  - command/event/tlm/sched/output-port assertions
- Keep the skill compact and workflow-oriented instead of turning it into a large "component creation" super-skill.

**Non-Goals:**

- Do not create a giant all-in-one component creation skill in this change.
- Do not move the authoritative testing rules out of formal specs and checked-in docs.
- Do not change flight runtime behavior or target integration.

## Decisions

### 1. Create one narrow skill focused on classic component harnesses

The change adds only:

- `classic-component-ut-pattern`

This keeps the scope aligned with the user's correction: the skill should teach how to build a complete classic component harness, not how to patch missing work after discovery.

### 2. Treat the skill as a workflow step for real components

The skill will explicitly say it applies when:

- a class under `OBC/Components/` derives from `*ComponentBase`
- the agent is adding or modifying that component

It will also explicitly say it does **not** apply to helper/support logic such as parsers, stores, providers, and framing helpers.

### 3. Keep the skill instruction-only in v1

The skill will use:

- `SKILL.md`
- `agents/openai.yaml`

It will point at checked-in repository examples instead of shipping a large template pack. The current repo already has strong examples:

- `ModeManager`
- `GpsBridge`
- `HousekeepingArchive`

## Risks / Trade-offs

- [Risk] The skill could over-trigger for helper classes living near components.
  → Mitigation: make the trigger text explicitly require a real `*ComponentBase` class and call out helper exclusions.

- [Risk] The skill could duplicate too much of the formal testing rules.
  → Mitigation: keep workflow rules brief and point back to the existing formal docs.

- [Risk] A future broader component-creation skill could overlap with this one.
  → Mitigation: keep this skill narrowly centered on classic harness creation so it can later be reused by a larger workflow without being replaced.
