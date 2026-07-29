## ADDED Requirements

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

## REMOVED Requirements

### Requirement: Change Closeout Skill
**Reason**: Agent-specific skill implementations are development-source tools,
not public product content.

**Migration**: Preserve the governed closeout steps in `CONTRIBUTING.md` and
delivery-workflow.

### Requirement: Hosted Probe Workflow Skill
**Reason**: Agent-specific skill implementations are excluded from the public
portfolio distribution.

**Migration**: Preserve hosted probe rules in verification documentation.

### Requirement: Classic Component UT Pattern Skill
**Reason**: Agent-specific skill implementations are excluded from the public
portfolio distribution.

**Migration**: Preserve component test rules in contributor documentation and
CI checks.

### Requirement: Hosted Probe GDS CLI Startup Order
**Reason**: Startup order is an operator and verification contract, not a
Codex-skill distribution requirement.

**Migration**: Preserve the startup order in the hosted verification runbook.
