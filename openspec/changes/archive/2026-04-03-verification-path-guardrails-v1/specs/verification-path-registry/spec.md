## ADDED Requirements

### Requirement: Repository Verification Path Registry
The repository SHALL maintain a reviewable verification-path registry under the checked-in documentation tree that names each formally proven validation path, the transport or port layering it covers, the governing archived evidence, and the adjacent paths it does not prove.

#### Scenario: Reviewer checks whether a path is already proven
- **WHEN** a developer or reviewer needs to know whether a validation path is already established in this repository
- **THEN** they SHALL be able to consult one repo-owned registry document that states the path name, scope, governing evidence, and out-of-scope neighboring paths

### Requirement: Path Registry Distinguishes Ground-Link Layers
The verification-path registry SHALL explicitly distinguish at least the direct `OBC -> GDS` TCP adapter path, the `fprime-cli -> GDS` command/uplink path, and any transparent-UART or framed-transparent external-comm path so they cannot be treated as interchangeable.

#### Scenario: GDS connectivity does not imply command-path coverage
- **WHEN** the repository has evidence proving direct `OBC -> GDS` TCP connectivity
- **THEN** the registry SHALL still describe `fprime-cli -> GDS` command/uplink behavior as a distinct path unless separate evidence has proven it

### Requirement: Registry Entries Cite Archived Evidence
Each verification-path registry entry SHALL cite the specific archived change or evidence record that proves the path instead of relying on generic framework knowledge.

#### Scenario: Registry entry points to governing record
- **WHEN** a registry entry declares a path formally proven
- **THEN** it SHALL cite the archived change or evidence file that reviewers can inspect to confirm that claim
