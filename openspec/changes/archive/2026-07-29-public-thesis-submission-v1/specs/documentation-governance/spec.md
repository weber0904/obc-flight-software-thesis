## ADDED Requirements

### Requirement: Public Documentation Has One Canonical Current Layer
The public repository SHALL use English canonical architecture, interface,
verification, contribution, and operator documents, with Traditional Chinese
limited to the repository summary, thesis claim map, and integrated demo guide.

#### Scenario: Current documentation is indexed
- **WHEN** a reviewer starts from the root or documentation README
- **THEN** each current topic SHALL route to one canonical document
- **AND** historical planning, reporting, review, and thesis-writing packages
  SHALL NOT appear as current entrypoints

### Requirement: Removed Documents Have Explicit Successors
Every high-value removed document SHALL have an exclusion reason and, where
applicable, a canonical successor in the publication manifest.

#### Scenario: Archived material names an excluded path
- **WHEN** preserved historical OpenSpec or evidence mentions an excluded
  document
- **THEN** the publication manifest SHALL explain its disposition
