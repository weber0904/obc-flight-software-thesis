# project-reporting Specification

## Purpose
Define checked-in professor or PM-facing reporting packages as refreshable point-in-time review artifacts that summarize validated project scope, architecture, verification posture, and stable live-demo plans without over-claiming unfinished hardware work.
## Requirements
### Requirement: Public Reporting Uses Release-Aligned Canonical Surfaces
The public repository SHALL provide reviewer-facing project scope,
architecture, verification posture, and demo routing through its README,
canonical documentation, thesis claim map, and release evidence instead of a
separate point-in-time reporting package.

#### Scenario: Professor or reviewer needs a project briefing
- **WHEN** the public repository is reviewed at the tagged boundary
- **THEN** current claims SHALL route to release-aligned canonical documents
- **AND** archived reporting changes SHALL remain available as historical
  provenance rather than current status
