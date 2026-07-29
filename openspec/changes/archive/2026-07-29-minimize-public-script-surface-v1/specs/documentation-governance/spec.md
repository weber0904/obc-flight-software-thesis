## MODIFIED Requirements

### Requirement: Documentation Governance Checker Enforces The Rules

The repository SHALL provide a documentation checker that enforces the
canonical current-document set, required navigation, valid relative links,
capability-first reader prose, and concise script documentation.

#### Scenario: Documentation structure drifts
- **WHEN** a reader document is missing, an overlapping document is added, a
  canonical link is broken, release-process commentary enters the current
  layer, or script documentation describes removed and internal cleanup state
- **THEN** the checker SHALL fail and identify the affected file
