## MODIFIED Requirements

### Requirement: Formal Delivery Governance
The project SHALL treat `openspec/specs/` as the only normative workflow source, `obc-dev-spec/` as the narrative companion layer, and `openspec/changes/` as the only sanctioned formal change workspace. Formal OpenSpec changes SHALL be required for product behavior, architecture, verification-path, CI-gate, release-governance, or formal-spec changes, while documentation-only changes MAY use the normal branch/PR flow without opening a formal change.

#### Scenario: Formal governance work still uses OpenSpec
- **WHEN** a repository change modifies the delivery rules, required verification gates, branch/commit policy, release governance, or a main spec under `openspec/specs/`
- **THEN** that work SHALL be tracked as a formal OpenSpec change instead of being made as an undocumented direct edit

#### Scenario: Pure documentation maintenance does not require OpenSpec
- **WHEN** a repository change is limited to audit/report prose, README wording, or other non-normative documentation that does not alter formal workflow, product behavior, or traceability requirements
- **THEN** the repository MAY complete that work through the normal branch and PR flow without creating a formal OpenSpec change

### Requirement: Delivery Gates
The delivery workflow SHALL require a build gate, unit/component test coverage, OpenSpec spec validation, and core repository traceability checks, and pull requests SHALL summarize intent, OpenSpec status, verification, and any constrained validation evidence.

#### Scenario: Required gate focuses on product and traceability risk
- **WHEN** the repository CI workflow validates the baseline gate
- **THEN** it SHALL run the shared verification script for build, test, spec validation, and core traceability checks, and it SHALL NOT fail the PR solely for agent-link wording or verification-matrix prose drift

### Requirement: Change Completion Checkpoints
The delivery workflow SHALL distinguish a `local-ready` checkpoint from formal completion. A change SHALL be considered `local-ready` only after the implementation is finished, the relevant local verification passes, the OpenSpec change has been archived when one exists, the worktree is clean, and the branch is ready to be proposed as a pull request.

#### Scenario: Documentation-only work reaches local-ready without archive
- **WHEN** a documentation-only branch does not require a formal OpenSpec change
- **THEN** it MAY reach `local-ready` after the relevant local verification passes and the worktree is clean, without an OpenSpec archive step

### Requirement: Formal Work Starts On A Dedicated Branch
The delivery workflow SHALL begin each formal repository change from a dedicated branch named `feature/<change-name>`, `fix/<change-name>`, `docs/<change-name>`, or `hotfix/<change-name>`, where `<change-name>` matches the formal OpenSpec change name. Documentation-only branches that do not require OpenSpec MAY use `docs/<topic>` or `fix/<topic>`.

#### Scenario: Formal change branch matches the OpenSpec change name
- **WHEN** a formal repository change is created under `openspec/changes/<change-name>/`
- **THEN** the active branch SHALL use one of the allowed prefixes and SHALL reuse the same `<change-name>` suffix for traceability

### Requirement: Baseline Reconciliation Matrix
The repository SHALL keep `openspec/reconciliation/baseline-reconciliation-matrix.json` as the only manually maintained reconciliation source and SHALL keep `openspec/reconciliation/baseline-reconciliation-matrix.md` as a generated review surface derived from that JSON source.

#### Scenario: Generated review surface stays aligned with the JSON source
- **WHEN** the reconciliation JSON is edited
- **THEN** the generated Markdown review surface SHALL be regenerated from the JSON source before the change is considered ready for review

### Requirement: Repository Consistency Checks
The delivery workflow SHALL provide a repo-local consistency check that fails when a main spec still carries placeholder Purpose text, when the current capability list diverges from `openspec/specs/`, when an archived change with `tasks.md` is missing from the reconciliation matrix, when the matrix cites a nonexistent capability or evidence path, or when the checked-in reconciliation Markdown no longer matches the generated output derived from the JSON source.

#### Scenario: Generated reconciliation markdown drift fails the check
- **WHEN** the checked-in reconciliation Markdown differs from the generated output for the checked-in JSON source
- **THEN** the repo-local consistency check SHALL fail and instruct the developer to regenerate the Markdown surface

### Requirement: Repository Agent Onboarding Entrypoint
The repository SHALL provide a repo-root `AGENTS.md` file that acts as the first checked-in onboarding surface for future agents, and that file SHALL point agents at the project README, the formal delivery workflow spec, the validation-path registry, and the relevant repository-owned skills without acting as a competing second workflow specification. OpenSpec-generated Codex skills under `.codex/skills/openspec-*` MAY coexist there, but they SHALL be treated as OpenSpec-managed artifacts refreshed by `openspec update`, not as hand-curated repository governance files.

#### Scenario: AGENTS read-first list stays narrow
- **WHEN** a new agent starts from a fresh checkout of the repository
- **THEN** `AGENTS.md` SHALL direct that agent first to `README.md`, `openspec/specs/delivery-workflow/spec.md`, and `evidence/verification-path-registry.md`, while allowing the narrative workflow document to remain an optional follow-up reference

#### Scenario: OpenSpec-generated Codex skills stay tool-managed
- **WHEN** the repository uses OpenSpec-generated Codex skills under `.codex/skills/openspec-*`
- **THEN** those files SHALL be refreshed through OpenSpec tooling such as `openspec update`, and governance cleanup SHALL NOT remove them by hand

### Requirement: Complex Change Commit Boundaries
The delivery workflow SHALL use squash-merge to keep `main` history reviewable, and the final mainline commit message SHALL come from the PR title using Conventional Commit form. Branch-local history MAY keep multiple development commits, and rebasing/squashing that branch history SHALL remain optional unless it is needed to remove sensitive content or materially improve review readability.

#### Scenario: Published PR keeps branch history but main stays clean
- **WHEN** a formal branch contains multiple development or review-follow-up commits
- **THEN** the workflow MAY keep those commits on the branch, and the final `main` history SHALL still be reduced to the PR-title Conventional Commit through squash-merge

#### Scenario: Branch history cleanup remains optional
- **WHEN** a branch is ready for review and its existing commits do not expose secrets and do not materially obstruct reviewer understanding
- **THEN** the workflow SHALL NOT require a pre-push history rewrite before the PR is opened
