# codex-skills-v1 Verification Record

## Scope

This record captures the first-version creation and validation of two repo-local Codex skills:

- `change-closeout`
- `hosted-probe-workflow`

It proves:

- both skills exist under `.codex/skills/`
- both skills use Codex-compatible folder structure and frontmatter
- both skills include `agents/openai.yaml` matching the Codex reference schema
- both skill folders pass the official quick validator

It does **not** prove:

- that the skills are perfect or final
- that larger future skills such as a `subsystem-slice` skill are justified

## Governing Change

- Change: [`2026-04-06-codex-skills-v1`](../../../openspec/changes/archive/2026-04-06-codex-skills-v1)

## Validation Commands

```bash
python3 $HOME/.codex/skills/.system/skill-creator/scripts/init_skill.py change-closeout --path .codex/skills --interface display_name="Change Closeout" --interface short_description="Close out repo changes safely" --interface default_prompt="Use this skill when implementation is done and the repo change needs local verification, OpenSpec sync/archive, commit readiness, and PR/CI completion."

python3 $HOME/.codex/skills/.system/skill-creator/scripts/init_skill.py hosted-probe-workflow --path .codex/skills --interface display_name="Hosted Probe Workflow" --interface short_description="Run governed hosted probe workflows" --interface default_prompt="Use this skill when adding or maintaining a hosted probe script for this repo and you need the governed pattern for runtime roots, fresh builds, bounded assertions, and evidence."

$REPO_ROOT/fprime-venv/bin/python $HOME/.codex/skills/.system/skill-creator/scripts/quick_validate.py .codex/skills/change-closeout

$REPO_ROOT/fprime-venv/bin/python $HOME/.codex/skills/.system/skill-creator/scripts/quick_validate.py .codex/skills/hosted-probe-workflow
```

## Expected Results

- each skill folder includes a valid `agents/openai.yaml`.
- `quick_validate.py` reports successful validation for both skill folders.
- `SKILL.md` contents align with:
  - [`obc-dev-spec/08_delivery_workflow.md`](../../../obc-dev-spec/08_delivery_workflow.md)
  - [`delivery-workflow spec`](../../../openspec/specs/delivery-workflow/spec.md)
  - [`verification-path-registry.md`](../../verification-path-registry.md)
  - [`verification-debugging-lessons.md`](../../../docs/verification.md)

## Notes

- This change intentionally keeps the skills narrow.
- It does not introduce a catch-all `subsystem-slice` skill.
- `agents/openai.yaml` follows the Codex `openai_yaml.md` reference schema.
