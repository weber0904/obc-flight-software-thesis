# classic-component-ut-skill-v1 Verification Record

## Scope

This record captures the creation and validation of the repo-local Codex skill:

- `classic-component-ut-pattern`

It proves:

- the skill exists under `.codex/skills/`
- the skill uses Codex-compatible folder structure and frontmatter
- the skill includes `agents/openai.yaml` matching the Codex metadata shape already used in this repository
- the skill passes the official quick validator
- the repository baseline gate still passes after adding the skill and updating the `codex-skills` capability

It does **not** prove:

- that a future broader component-creation skill already exists
- that the skill replaces the formal testing rules in repository specs

## Governing Change

- Change: [`classic-component-ut-skill-v1`](../../../openspec/changes/archive/2026-04-08-classic-component-ut-skill-v1)

## Validation Commands

```bash
$REPO_ROOT/fprime-venv/bin/python $HOME/.codex/skills/.system/skill-creator/scripts/quick_validate.py .codex/skills/classic-component-ut-pattern

bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local

openspec validate classic-component-ut-skill-v1

openspec validate --specs
```

## Expected Results

- `quick_validate.py` reports successful validation for `classic-component-ut-pattern`.
- the skill contents align with:
  - [`AGENTS.md`](../../../CONTRIBUTING.md)
  - [`delivery-workflow spec`](../../../openspec/specs/delivery-workflow/spec.md)
  - [`verification-evidence spec`](../../../openspec/specs/verification-evidence/spec.md)
  - [`codex-skills spec`](../../../openspec/specs/codex-skills/spec.md)
- the repository baseline gate remains green after the skill/spec/docs updates.

## Notes

- This skill intentionally focuses on the classic component harness step.
- It is designed to be reusable inside future component-creation workflows, but it is not a large all-in-one component-development skill.
