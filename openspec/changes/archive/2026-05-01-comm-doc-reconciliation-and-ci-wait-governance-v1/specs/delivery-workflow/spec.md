## ADDED Requirements

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
