## 1. Agent Entrypoint

- [x] 1.1 Add repo-root `AGENTS.md` as the checked-in onboarding entrypoint for future agents.
- [x] 1.2 Add a narrow repo-local checker that validates the required `AGENTS.md` references.

## 2. Workflow Alignment

- [x] 2.1 Add the delivery-workflow delta spec for the agent-entrypoint and onboarding-check requirements.
- [x] 2.2 Update repository entrypoint documents so they point to `AGENTS.md` without duplicating a second workflow spec.

## 3. Evidence And Validation

- [x] 3.1 Add a verification record for the new onboarding entrypoint and checker.
- [x] 3.2 Run the onboarding checker, `openspec validate agent-onboarding-governance-v1`, and `openspec validate --specs`.

## 4. Finalization

- [x] 4.1 Archive the change after the new entrypoint and docs are aligned.
- [x] 4.2 Commit the onboarding-governance update using the repository's Conventional Commit rule.
