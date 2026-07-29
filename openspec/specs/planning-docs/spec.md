# planning-docs Specification

## Purpose
Define the repository's non-normative planning-note and roadmap-handoff space, including required freshness metadata and conflict handling against formal sources.
## Requirements
### Requirement: Formal Sources Win Conflicts

Formal sources SHALL govern implementation and validation when
engineering-direction prose conflicts with code, topology, scripts, main
specs, the verification-path registry, or recorded evidence.

#### Scenario: Direction conflicts with registered evidence
- **WHEN** an engineering direction describes a capability absent from the
  registry or evidence
- **THEN** the repository SHALL require a governed change and matching
  verification before treating it as implemented behavior

### Requirement: Public Engineering Direction Is Architecture Integrated

Applicable engineering extensions SHALL be summarized with the system
architecture instead of being maintained as a separate reader-facing roadmap
family.

#### Scenario: Reviewer looks for natural extensions
- **WHEN** a reviewer reads the architecture
- **THEN** applicable extensions SHALL be presented in the context of the
  implemented system and formal OpenSpec history

