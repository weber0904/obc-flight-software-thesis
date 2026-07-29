## Context

This repository now depends on a mix of pure file operations and runtime-bearing verification commands. The latter frequently start `fprime-gds`, ZMQ-backed CSP runtimes, repository-owned stack scripts, PTY bridges, UART- or SocketCAN-adjacent tooling, or networked GitHub / SSH operations. Those commands often fail under the default sandbox for reasons unrelated to the product change under test.

The current workflow documents describe what verification must run, but they do not explicitly classify which command families should skip sandbox-first execution. That gap causes repeated false starts during agent-driven work.

## Goals / Non-Goals

**Goals:**
- Make unrestricted-execution defaults explicit in the normative delivery workflow.
- Keep the rule scoped to command classes, not one-off command strings.
- Mirror the same rule in `AGENTS.md` and the human-readable workflow document.

**Non-Goals:**
- Introduce new verification gates or change which checks are required.
- Replace repository skills or tool policies with repo-local scripting.
- Define desktop-app sandbox internals outside the repository behaviors the workflow cares about.

## Decisions

- Add an `Execution Permission Classification` requirement to `delivery-workflow` rather than overloading the existing CI or delivery-gate requirements.
  - This keeps the rule focused on pre-execution command selection instead of changing gate semantics.
- Classify command families by observed repository behavior:
  - unrestricted by default for probes, stack scripts, shared verification, port-binding runtimes, device-facing commands, and networked operational CLIs
  - default sandbox allowed for static reads, file edits, OpenSpec validation, and repo-local consistency checks
- Record the same guidance in `AGENTS.md` and `obc-dev-spec/08_delivery_workflow.md` so the rule is visible both to future agents and to human operators.

## Risks / Trade-offs

- [Risk] The unrestricted list could become stale as new runtime scripts are added. → Mitigation: anchor the rule to command classes and representative repository scripts instead of an exhaustive hardcoded inventory.
- [Risk] Overusing unrestricted execution could widen command privileges unnecessarily. → Mitigation: keep pure file-oriented and static validation commands explicitly in the default-sandbox bucket.
- [Risk] Readers may mistake this as a replacement for tool-level permission policy. → Mitigation: state the rule as a repository workflow default for agent-driven execution, not as a platform security specification.

## Migration Plan

1. Add the new requirement in the delivery-workflow delta spec.
2. Update `AGENTS.md` and the narrative workflow document to mirror the rule.
3. Validate the change and archive it so the main spec becomes the new canonical baseline.

## Open Questions

- None for this v1 rule. Future refinement can expand the examples list if new runtime classes appear.
