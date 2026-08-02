## MODIFIED Requirements

### Requirement: Documentation Governance Checker Enforces The Rules

The repository SHALL provide a documentation checker that enforces the
canonical current-document set, required navigation, valid relative links,
capability-first reader prose, concise script documentation, and file-level
coverage of the retained script surface.

#### Scenario: Documentation structure drifts
- **WHEN** a reader document is missing, an overlapping document is added, a
  canonical link is broken, release-process commentary enters the current
  layer, script documentation describes removed cleanup state, or a retained
  script file lacks a catalog entry
- **THEN** the checker SHALL fail and identify the affected file

#### Scenario: Operator command drifts
- **WHEN** a hosted or target/laboratory operator guide names a script command
- **THEN** the referenced path SHALL exist
- **AND** the catalog SHALL classify it as a public entrypoint
