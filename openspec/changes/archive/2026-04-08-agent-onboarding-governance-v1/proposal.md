## Why

The repository now has formal specs, archived changes, test records, and repo-local Codex skills, but it still lacks a single repo-root entrypoint that future agents can read first. Without that entrypoint, new sessions keep rediscovering workflow rules from chat history instead of following one checked-in onboarding surface.

## What Changes

- Add a repo-root `AGENTS.md` that tells future agents what this repository is, which files to read first, which workflow to follow, which validation paths must stay distinct, and where repo-local skills fit.
- Add a narrow repo-local checker that verifies `AGENTS.md` exists and still points at the required checked-in workflow and validation sources.
- Update the formal and narrative delivery workflow docs so the agent-entrypoint rule is part of the governed workflow rather than an unwritten convention.
- Update repository entrypoint documents to point contributors and automation toward `AGENTS.md` instead of relying on chat-only onboarding.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: require a repo-root `AGENTS.md` onboarding entrypoint and a repo-owned check that keeps its required references in sync with the governed workflow.

## Impact

- Affected code: new repo-local onboarding checker script.
- Affected systems: agent onboarding, repo entrypoint docs, and formal workflow governance.
- No change to flight runtime behavior, simulators, or hardware integration.
