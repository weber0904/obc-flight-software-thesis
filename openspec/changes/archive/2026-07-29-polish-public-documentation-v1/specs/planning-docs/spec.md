## ADDED Requirements

### Requirement: Public Engineering Direction Is Architecture Integrated

Applicable engineering extensions SHALL be summarized with the system
architecture instead of being maintained as a separate reader-facing roadmap
family.

#### Scenario: Reviewer looks for natural extensions
- **WHEN** a reviewer reads the architecture
- **THEN** applicable extensions SHALL be presented in the context of the
  implemented system and formal OpenSpec history

## MODIFIED Requirements

### Requirement: Formal Sources Win Conflicts

Formal sources SHALL govern implementation and validation when
engineering-direction prose conflicts with code, topology, scripts, main
specs, the verification-path registry, or recorded evidence.

#### Scenario: Direction conflicts with registered evidence
- **WHEN** an engineering direction describes a capability absent from the
  registry or evidence
- **THEN** the repository SHALL require a governed change and matching
  verification before treating it as implemented behavior

## REMOVED Requirements

### Requirement: Non-Normative Planning Space
**Reason**: A separate `docs/roadmap/` reader family duplicates architecture
and formal OpenSpec work history.

**Migration**: Use `docs/architecture.md` for system-context extensions and
OpenSpec changes for governed work.

### Requirement: Planning Notes Declare Freshness
**Reason**: Active standalone planning notes are not part of the repository
reader model.

**Migration**: Use Git history and OpenSpec change metadata.

### Requirement: Roadmaps Preserve Narrow Change Boundaries
**Reason**: Narrow change boundaries are governed by OpenSpec and evidence.

**Migration**: Express candidate work as an OpenSpec change.

### Requirement: Retired Planning Redirects Do Not Carry Active Content
**Reason**: Both planning redirects and the roadmap family are absent.

**Migration**: Route current readers through `docs/README.md`.

### Requirement: Closed Practical Capability Lines Leave The Active Queue Clean
**Reason**: The repository does not expose a separate active roadmap queue.

**Migration**: Implemented behavior is described in current docs and remaining
formal work is represented by OpenSpec.

### Requirement: Public Roadmap Is Release Aligned
**Reason**: There is no separate reader-facing roadmap.

**Migration**: Keep applicable extensions in `docs/architecture.md`.
