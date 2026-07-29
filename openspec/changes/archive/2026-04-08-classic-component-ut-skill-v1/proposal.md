## Why

The repository now has a formal rule that every real F' component under `OBC/Components/` must carry classic L2 component harness coverage. That rule is now recorded in the workflow and verification specs, but future sessions still need a narrow repo-local skill that shows how to build the harness correctly as part of normal component work instead of treating it as an after-the-fact repair.

## What Changes

- Add a repo-local `classic-component-ut-pattern` Codex skill under `.codex/skills/`.
- Update the `codex-skills` capability so the repository formally records this skill and its intended use.
- Record validation evidence showing the skill folder uses Codex-compatible structure and passes the official quick validator.

## Capabilities

### New Capabilities

### Modified Capabilities

- `codex-skills`: add a skill that teaches how to build classic F' component unit-test harnesses for real repository components while keeping helper tests separate.

## Impact

- Affected code and files:
  - `.codex/skills/classic-component-ut-pattern/`
  - `AGENTS.md`
  - `evidence/records/classic-component-ut-skill-v1/`
  - `openspec/specs/codex-skills/`
- No flight runtime behavior or target deployment behavior changes.
