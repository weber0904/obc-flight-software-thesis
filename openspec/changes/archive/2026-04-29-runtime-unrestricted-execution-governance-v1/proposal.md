## Why

Repository-owned probes, stack scripts, and the shared verification gate now routinely start hosted runtimes that bind local ports, talk to local devices, or use networked tooling. When those commands are launched under the default sandbox first and only retried after failure, agents waste time on predictable permission errors and can misread sandbox failures as product regressions.

## What Changes

- Add a formal delivery-workflow requirement that classifies runtime-bearing and network-bearing commands before execution instead of discovering sandbox limits by trial and error.
- Define the repository command classes that SHALL default to unrestricted execution, including repository-owned probes, stack scripts, the shared verification gate, and networked CLI operations such as `gh` and `ssh`.
- Define the complementary command classes that MAY remain in the default sandbox, such as file inspection, file editing, OpenSpec validation, and repo-local consistency checks.
- Update the repo-root agent onboarding entrypoint and the narrative delivery-workflow document so the same rule is visible outside the normative spec.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `delivery-workflow`: add a formal execution-permission classification rule for agent-driven verification and operational commands

## Impact

- `openspec/specs/delivery-workflow/spec.md`
- `AGENTS.md`
- `obc-dev-spec/08_delivery_workflow.md`
- future agent-run verification and GitHub interaction flow in this repository
