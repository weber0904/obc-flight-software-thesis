## ADDED Requirements

### Requirement: Public Submodule Revisions Are Recursively Obtainable
Every submodule Gitlink declared by the public release SHALL resolve from its
configured public remote in a fresh recursive checkout of the release commit.

#### Scenario: Reviewer clones the release recursively
- **WHEN** a reviewer clones the candidate release with recursive submodules
- **THEN** every declared submodule SHALL checkout to the Gitlink revision
- **AND** the resulting checkout SHALL not contain modified or uninitialized
  submodules

### Requirement: Public Automation Has No Local Thesis Workspace Exception
The public script surface SHALL not allow, synchronize, or test an ignored
local thesis-writing workspace as an exception to provenance or release rules.

#### Scenario: Reviewer inspects public provenance automation
- **WHEN** a reviewer scans the public Route 1 provenance and target-sync
  entrypoints
- **THEN** no `.codex_thesis_work` exception or exclusion SHALL be present
- **AND** campaign identity SHALL still reject undeclared untracked roots
