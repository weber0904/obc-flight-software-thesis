# codex-skills Specification

## Purpose

Define the repo-local Codex skills that capture stable development workflows for this repository without collapsing unrelated work into a single oversized skill.
## Requirements
### Requirement: Public Workflow Is Tool-Neutral
The public repository SHALL preserve the engineering rules formerly encoded
by agent-specific skills in contributor, operator, verification, and delivery
documentation without distributing a required agent-automation layer.

#### Scenario: Contributor uses a different development tool
- **WHEN** a contributor follows the public repository workflow
- **THEN** build, test, probe, and closeout requirements SHALL remain
  discoverable without installing repository-specific agent skills
- **AND** archived skill changes SHALL remain available as historical
  provenance
