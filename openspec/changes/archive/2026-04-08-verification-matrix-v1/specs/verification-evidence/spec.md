## ADDED Requirements

### Requirement: Formal Verification Matrix
The repository SHALL provide a checked-in verification matrix that summarizes each current formal capability's L1, L2, L3, and L4 coverage together with any explicitly constrained or currently weak areas.

#### Scenario: Capability coverage is reviewable in one place
- **WHEN** a reviewer asks what verification currently exists for a capability
- **THEN** the repository SHALL provide one matrix that identifies the capability's current L1, L2, L3, and L4 coverage and its known constrained gaps

#### Scenario: Weak coverage areas stay explicit
- **WHEN** a capability relies more heavily on integration tests, hosted probes, or constrained evidence than on component-level tests
- **THEN** the matrix SHALL describe that unevenness explicitly instead of implying the capability has balanced coverage at every layer

### Requirement: Repository Verification Inventory Report
The repository SHALL provide a repo-local inventory/report tool that aggregates the currently registered tests, baseline verification gate entrypoint, hosted probe scripts, and checked-in evidence directories from the repository state.

#### Scenario: Inventory report reflects registered tests and probes
- **WHEN** a developer runs the verification inventory tool
- **THEN** it SHALL report the repository's registered UT/IT entrypoints, baseline verification gate script, hosted probe scripts, and evidence directories using the current checked-in sources
