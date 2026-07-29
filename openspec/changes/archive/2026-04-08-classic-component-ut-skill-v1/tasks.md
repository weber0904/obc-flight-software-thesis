## 1. Skill scaffolding

- [x] 1.1 Add `classic-component-ut-pattern` under `.codex/skills/` using the standard Codex skill folder structure.
- [x] 1.2 Author `SKILL.md` so it teaches when the skill applies, how to distinguish component vs helper, how to scaffold classic harness files, and what contract coverage to include.
- [x] 1.3 Add `agents/openai.yaml` metadata matching the skill purpose.

## 2. Formal repo updates

- [x] 2.1 Update `openspec/specs/codex-skills/spec.md` to formally record the new skill and its role.
- [x] 2.2 Update `AGENTS.md` so future sessions can discover the skill from the repo entrypoint.
- [x] 2.3 Add verification evidence under `docs/test-records/classic-component-ut-skill-v1/`.

## 3. Verification and closeout

- [x] 3.1 Validate the new skill folder with the official quick validator and capture the exact commands used.
- [x] 3.2 Run the repository baseline gate after the skill/spec/docs updates.
- [x] 3.3 Run `openspec validate classic-component-ut-skill-v1` and `openspec validate --specs`, then sync/archive the change.
