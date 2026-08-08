## Why

Recent changes repeatedly reused two repository-specific workflows: governed change closeout and repository-owned hosted probes. Without repo-local Codex skills for these workflows, new sessions keep rediscovering the same rules and occasionally drift into inconsistent validation or delivery behavior.

## What Changes

- Add a repo-local `change-closeout` Codex skill for this repository's local-ready, push, PR, and CI-gated completion flow.
- Add a repo-local `hosted-probe-workflow` Codex skill for building and rerunning repository-owned hosted probes against governed runtime paths.
- Validate both skills using the official Codex skill initializer / metadata generator / validator flow, then record the result as repository evidence.

## Capabilities

### New Capabilities
- `codex-skills`: Repo-local Codex skills that stabilize repeated repository workflows without creating a large, catch-all skill.

### Modified Capabilities

## Impact

- Affected code and files:
  - `.codex/skills/change-closeout/`
  - `.codex/skills/hosted-probe-workflow/`
  - `evidence/records/codex-skills-v1/`
  - `openspec/specs/codex-skills/`
- No flight runtime behavior or target deployment behavior changes.
