# delivery-workflow Specification

## Purpose
Define the formal delivery lifecycle, Git/PR/CI expectations, OpenSpec workflow, and the repository traceability rules that keep the development record reviewable without promoting every documentation task into a governed implementation change.
## Requirements
### Requirement: Formal Delivery Governance
The project SHALL treat `openspec/specs/` as the only normative workflow source, `obc-dev-spec/` as the narrative companion layer, and `openspec/changes/` as the only sanctioned formal change workspace. Formal OpenSpec changes SHALL be required for product behavior, architecture, verification-path, CI-gate, release-governance, or formal-spec changes, while documentation-only changes MAY use the normal branch/PR flow without opening a formal change.

#### Scenario: Formal governance work still uses OpenSpec
- **WHEN** a repository change modifies the delivery rules, required verification gates, branch/commit policy, release governance, or a main spec under `openspec/specs/`
- **THEN** that work SHALL be tracked as a formal OpenSpec change instead of being made as an undocumented direct edit

#### Scenario: Pure documentation maintenance does not require OpenSpec
- **WHEN** a repository change is limited to audit/report prose, README wording, or other non-normative documentation that does not alter formal workflow, product behavior, or traceability requirements
- **THEN** the repository MAY complete that work through the normal branch and PR flow without creating a formal OpenSpec change

### Requirement: OpenSpec Lifecycle
The official change lifecycle SHALL use the real OpenSpec CLI flow of `new change`, `status`, `instructions`, `validate`, and `archive`, and the project SHALL NOT use manual directory moves as the formal archive mechanism.

#### Scenario: Change completion uses archive command
- **WHEN** a change is ready to sync into the main specs
- **THEN** the project SHALL use `openspec archive` to update `openspec/specs/` instead of manually moving the change directory

### Requirement: Delivery Gates

The delivery workflow SHALL keep a single required GitHub CI job named
`baseline-gate`, SHALL require build and test coverage for product-impacting
changes, SHALL require OpenSpec spec validation and core repository traceability
checks for all governed changes, and SHALL require pull requests to summarize
intent, OpenSpec status, verification, and any constrained validation evidence.

#### Scenario: Required gate focuses on product and traceability risk

- **WHEN** the repository CI workflow validates the baseline gate for a
  product-impacting or otherwise unknown change scope
- **THEN** it SHALL run the shared verification script for F' generate/build,
  unit test generation/build, `fprime-util check --all`, OpenSpec spec
  validation, and core traceability checks
- **AND** it SHALL run the documentation governance checker together with the
  other repository consistency checks
- **AND** it SHALL NOT fail the PR solely for agent-link wording or
  verification-matrix prose drift

### Requirement: Change Completion Checkpoints
The delivery workflow SHALL distinguish a `local-ready` checkpoint from formal completion. A change SHALL be considered `local-ready` only after the implementation is finished, the relevant local verification passes, the OpenSpec change has been archived when one exists, the worktree is clean, and the branch is ready to be proposed as a pull request.

#### Scenario: Documentation-only work reaches local-ready without archive
- **WHEN** a documentation-only branch does not require a formal OpenSpec change
- **THEN** it MAY reach `local-ready` after the relevant local verification passes and the worktree is clean, without an OpenSpec archive step

### Requirement: Push And CI Gate
The delivery workflow SHALL treat push as a separate step after the `local-ready` checkpoint, and a change SHALL NOT be treated as formally complete until the pushed commit has passed the required GitHub CI workflow.

#### Scenario: Change waits for developer confirmation before push
- **WHEN** a change has reached the `local-ready` checkpoint
- **THEN** the workflow SHALL wait for explicit developer confirmation before pushing the commit to GitHub

#### Scenario: Change becomes formally complete only after CI green
- **WHEN** a change has been pushed to GitHub
- **THEN** it SHALL remain in a not-yet-complete state until the relevant CI workflow reports success for that pushed commit

### Requirement: Release And Tag Timing
The delivery workflow SHALL create releases and tags only after the relevant pushed commit has completed the required CI workflow successfully.

#### Scenario: Release work waits for CI green
- **WHEN** a change is being considered for release or tag creation
- **THEN** the workflow SHALL wait until the relevant pushed commit has passed CI before creating the release or tag

### Requirement: Formal Work Starts On A Dedicated Branch
The delivery workflow SHALL begin each formal repository change from a dedicated branch named `feature/<change-name>`, `fix/<change-name>`, `docs/<change-name>`, or `hotfix/<change-name>`, where `<change-name>` matches the formal OpenSpec change name. Documentation-only branches that do not require OpenSpec MAY use `docs/<topic>` or `fix/<topic>`.

#### Scenario: Formal change branch matches the OpenSpec change name
- **WHEN** a formal repository change is created under `openspec/changes/<change-name>/`
- **THEN** the active branch SHALL use one of the allowed prefixes and SHALL reuse the same `<change-name>` suffix for traceability

### Requirement: Repository Evidence Governs Validation Path Reuse
The delivery workflow SHALL require developers to consult repository evidence before reusing a validation path, and it SHALL NOT treat generic upstream F' expectations as sufficient proof that a repository-specific path is already established.

#### Scenario: Generic framework knowledge is not enough
- **WHEN** a developer believes a path is already proven because it is common in generic F' usage
- **THEN** the workflow SHALL require them to cite repository evidence or the verification-path registry before treating that path as established in this repository

#### Scenario: Adjacent validation paths stay distinct
- **WHEN** a change validates one path that sits next to another path in the same ground or transport stack
- **THEN** the workflow SHALL keep those paths distinct unless archived evidence proves each one separately

### Requirement: Hosted Probe Cleanup Migration Follows Touch-On-Use Policy
The delivery workflow SHALL modernize repository-owned hosted probe cleanup on a touch-on-use basis instead of requiring a batch rewrite of every legacy probe before unrelated feature work can continue.

#### Scenario: Change reuses a legacy-cleanup probe
- **WHEN** a current change needs a repository-owned probe whose cleanup still uses the legacy ad hoc model
- **THEN** that change SHALL migrate the probe to the repository's current managed cleanup pattern before treating the probe result as governed evidence for the change
- **AND** the change SHALL verify at least one immediate rerun plus absence of owned-helper orphan or high-CPU residual processes for that migrated probe path

#### Scenario: Migrated probe still fails for unrelated path reasons
- **WHEN** a probe has already been migrated to the current cleanup model but still fails because of known architecture drift, stale assumptions, or unrelated instability outside the current change scope
- **THEN** the change MAY record bounded blocker evidence instead of forcing unrelated deep debug
- **AND** it SHALL NOT claim that failing path as passing proof for the current change

### Requirement: Baseline Reconciliation Matrix

The repository SHALL keep
`openspec/reconciliation/baseline-reconciliation-matrix.json` as the manually
maintained reconciliation source and
`openspec/reconciliation/baseline-reconciliation-matrix.md` as its generated
review surface.

#### Scenario: Generated review surface stays aligned with the JSON source
- **WHEN** the reconciliation JSON is edited
- **THEN** the Markdown review surface SHALL be regenerated before review
- **AND** repository consistency checks SHALL validate both files

### Requirement: Repository Consistency Checks
The delivery workflow SHALL provide a repo-local consistency check that fails when a main spec still carries placeholder Purpose text, when the current capability list diverges from `openspec/specs/`, when an archived change with `tasks.md` is missing from the reconciliation matrix, when the matrix cites a nonexistent capability or evidence path, or when the checked-in reconciliation Markdown no longer matches the generated output derived from the JSON source.

#### Scenario: Generated reconciliation markdown drift fails the check
- **WHEN** the checked-in reconciliation Markdown differs from the generated output for the checked-in JSON source
- **THEN** the repo-local consistency check SHALL fail and instruct the developer to regenerate the Markdown surface

### Requirement: Component-Test Baseline Checker
The delivery workflow SHALL provide a repo-local checker that enumerates real repository components and fails when one lacks the required classic F' component harness registration.

#### Scenario: New component without classic UT fails the checker
- **WHEN** a real component derived from `*ComponentBase` is present in `OBC/Components/` but its module does not register classic F' UT
- **THEN** the repo-local component-test-baseline checker SHALL fail and report the offending component name

#### Scenario: Shared gate runs the component-test checker
- **WHEN** the repository executes the shared baseline gate
- **THEN** that gate SHALL run the component-test-baseline checker before declaring the baseline verification complete

### Requirement: Complex Change Commit Boundaries
The delivery workflow SHALL use squash-merge to keep `main` history reviewable, and the final mainline commit message SHALL come from the PR title using Conventional Commit form. Branch-local history MAY keep multiple development commits, and rebasing/squashing that branch history SHALL remain optional unless it is needed to remove sensitive content or materially improve review readability.

#### Scenario: Published PR keeps branch history but main stays clean
- **WHEN** a formal branch contains multiple development or review-follow-up commits
- **THEN** the workflow MAY keep those commits on the branch, and the final `main` history SHALL still be reduced to the PR-title Conventional Commit through squash-merge

#### Scenario: Branch history cleanup remains optional
- **WHEN** a branch is ready for review and its existing commits do not expose secrets and do not materially obstruct reviewer understanding
- **THEN** the workflow SHALL NOT require a pre-push history rewrite before the PR is opened

### Requirement: Execution Permission Classification
The delivery workflow SHALL classify agent-driven repository commands before execution, and runtime-bearing or network-bearing command classes SHALL default to unrestricted execution instead of relying on an initial sandboxed failure to discover required permissions.

#### Scenario: Repository-owned runtime verification commands start unrestricted
- **WHEN** an agent runs a repository-owned probe, stack script, or the shared verification gate and that command may start hosted runtimes, bind local ports, open PTYs or serial devices, touch SocketCAN-adjacent interfaces, or otherwise require local runtime resources outside pure file access
- **THEN** the workflow SHALL treat that command as unrestricted by default rather than first attempting the same command under the default sandbox

#### Scenario: Networked operational commands start unrestricted
- **WHEN** an agent runs a repository command that depends on external network access or remote sessions such as `gh`, `ssh`, remote Raspberry Pi scripts, or equivalent repository-operated tooling
- **THEN** the workflow SHALL treat that command as unrestricted by default rather than waiting for a sandbox-related network failure

#### Scenario: Static file-oriented commands may stay sandboxed
- **WHEN** an agent runs file inspection, file editing, OpenSpec validation, repo-local consistency checks, or other commands that operate entirely within the writable workspace without binding ports, opening device handles, or requiring external network access
- **THEN** the workflow MAY keep those commands in the default sandbox

### Requirement: PR Review-Ready Creation
After an agent pushes a reviewable branch, the delivery workflow SHALL open a ready-for-review, non-draft pull request by default so CI and reviewer automation can run. Draft pull requests MAY be used only when the developer explicitly requests a draft or when the branch is intentionally not ready for review.

#### Scenario: Agent opens PR for review automation
- **WHEN** an agent has pushed a branch that reached the review-ready boundary
- **THEN** the agent SHALL open or update a non-draft PR unless the developer explicitly requested a draft
- **AND** the workflow SHALL NOT rely on automated reviewer feedback until any draft PR has been marked ready for review

### Requirement: PR CI Wait Handoff
After an agent pushes a branch and opens or updates a pull request, the delivery workflow SHALL allow the agent to stop active monitoring once it has reported the PR link, pushed commit, and current CI state, unless the developer explicitly asks the agent to continue monitoring or act on CI results. This handoff SHALL NOT weaken the rule that formal completion, merge, release, and tag work require the pushed commit's required CI to pass.

#### Scenario: Agent stops after reporting pending CI
- **WHEN** an agent has pushed a review-ready branch, opened or updated the PR, and observed that required hosted CI is pending or running
- **THEN** the agent SHALL report the PR/check state and MAY stop the turn instead of repeatedly polling CI
- **AND** the change SHALL remain not formally complete until the developer reports CI green or asks the agent to resume CI handling

#### Scenario: Developer requests continued CI handling
- **WHEN** the developer explicitly asks the agent to monitor CI, debug a failed check, merge after green, or otherwise continue after push
- **THEN** the agent MAY continue using the relevant GitHub workflow while preserving the rule that merge, release, and tag actions wait for CI green

### Requirement: Verification Scope Classification

The delivery workflow SHALL classify each pull request verification scope as
`full` or `lightweight` from the changed-file set. The classifier SHALL use a
whitelist strategy: only explicitly listed documentation and governance paths
may use `lightweight`; empty, unknown, product, build, workflow, script, or
active OpenSpec change-workspace paths SHALL use `full`.

#### Scenario: Documentation and governance-only changes use lightweight verification

- **WHEN** a PR changes only files within the lightweight-eligible set such as
  documentation prose, governance specs, archived changes, test records,
  reconciliation surfaces, or agent skill Markdown
- **THEN** the CI gate SHALL skip F' generate/build, unit test
  generation/build, and `fprime-util check --all`
- **AND** it SHALL still run `check_repo_consistency.py`,
  `check_component_test_baseline.py`, `check_legacy_zmq_retired.py`, the
  documentation governance checker, and `openspec validate --specs`

### Requirement: Mainline Narrative Documentation Reconciliation

The delivery workflow SHALL update branch-verifiable documentation on the
originating branch before review, including formal specs, product behavior,
architecture, verification, evidence, and operator procedures.

#### Scenario: Branch changes a documented contract
- **WHEN** a change alters formal workflow, product behavior, architecture,
  verification, evidence, or operator procedures
- **THEN** the relevant documentation SHALL be updated on the originating
  branch
- **AND** the update SHALL use the normal review workflow

### Requirement: Public Thesis Release Uses A Separate Governed Repository
The thesis release SHALL be assembled on a dedicated branch of the public
repository from a fixed source commit and SHALL use PR, CI, merge, annotated
tag, and GitHub Release gates.

#### Scenario: Public release reaches local-ready
- **WHEN** implementation, documentation, evidence, OpenSpec sync/archive, and
  clean-clone validation are complete
- **THEN** the branch SHALL remain local until explicit push approval
- **AND** the eventual PR SHALL be ready-for-review rather than draft

#### Scenario: Public release reaches tag-ready
- **WHEN** the public PR is merged and required CI is green
- **THEN** the exact merged commit SHALL be the only allowed tag target
