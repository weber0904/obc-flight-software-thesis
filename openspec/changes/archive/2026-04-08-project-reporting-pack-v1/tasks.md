## 1. Change Artifacts

- [x] 1.1 Create the proposal, design, and delta spec for the new `project-reporting` capability and the modified `verification-evidence` expectations.
- [x] 1.2 Write task breakdowns that keep the reporting package tied to checked-in repository truth instead of chat-only summaries.

## 2. Reporting Package

- [x] 2.1 Add a `docs/reporting/` index and a `project-reporting-pack-v1/` package directory.
- [x] 2.2 Write the main professor/PM briefing document with the 10-15 minute talk flow, the one-sentence project positioning, and the reporting guidance for non-domain audiences.
- [x] 2.3 Write one Mermaid diagram document containing the context diagram, internal architecture diagram, and workflow/verification diagram.
- [x] 2.4 Write a capability status matrix that summarizes completed scope, proven paths, current limits, and next steps in PM/professor-friendly language.
- [x] 2.5 Write a governed demo runbook with the primary hosted path, the command sequence, what each step proves, and the fallback plan.

## 3. Evidence And Review Surfaces

- [x] 3.1 Add a test/evidence record for the reporting package that cites the checked-in truth sources used to build it.
- [x] 3.2 Update repository review surfaces so the new `project-reporting` capability and this archived change stay visible after archive.
- [x] 3.3 Update any top-level documentation indices needed so the reporting package is discoverable from the repository root.

## 4. Validation And Finalization

- [x] 4.1 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.2 Run `bash scripts/run_verification_ci.sh build-artifacts/project-reporting-pack-v1`.
- [x] 4.3 Run `openspec validate project-reporting-pack-v1` and `openspec validate --specs`.
- [x] 4.4 Sync and archive `project-reporting-pack-v1`, then prepare the branch for governed closeout.
