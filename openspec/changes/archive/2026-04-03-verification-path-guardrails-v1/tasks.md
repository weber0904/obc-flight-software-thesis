## 1. Workflow Guardrails

- [x] 1.1 Update the narrative delivery workflow to require dedicated `feature/*`, `fix/*`, `docs/*`, or `hotfix/*` branches for formal work.
- [x] 1.2 Update the narrative delivery workflow to state that repository evidence, not generic upstream knowledge, governs validation-path reuse.

## 2. Verification Path Registry

- [x] 2.1 Add a repo-owned verification-path registry document under `docs/` that lists the currently proven paths, their scope, and their governing evidence.
- [x] 2.2 Link the new registry from the repository documentation index so future conversations can find it quickly.

## 3. Evidence And Lessons Alignment

- [x] 3.1 Update the formal delivery-workflow and verification-evidence specs to match the new branch, path-registry, and path-distinction rules.
- [x] 3.2 Extend the debugging-lessons document so the CLI/TTS confusion is recorded as a repository rule, not just a one-off note.
- [x] 3.3 Update the affected evidence records to cite the new verification-path registry where reuse of an older path matters.

## 4. Validation And Closeout

- [x] 4.1 Run `openspec validate verification-path-guardrails-v1`.
- [x] 4.2 Run `openspec validate --specs`.
- [x] 4.3 Archive the change and collect the work into a single Conventional Commit without pushing.
