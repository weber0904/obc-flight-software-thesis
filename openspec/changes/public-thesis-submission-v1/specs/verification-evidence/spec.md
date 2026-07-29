## ADDED Requirements

### Requirement: Public Evidence Separates Summary From Raw Artifacts
Public test-record summaries, claims, verdicts, and provenance SHALL remain in
Git while raw `artifacts/` trees SHALL be externalized to a checksummed release
asset.

#### Scenario: Test record has raw artifacts
- **WHEN** a test record had a source `artifacts/` subtree
- **THEN** the public record SHALL contain an `ARTIFACTS.json` descriptor
- **AND** the global catalog SHALL bind it to the tagged release asset

### Requirement: Evidence Redaction Is Reviewable
Text evidence sanitization SHALL use deterministic rules and SHALL record both
source and public digests.

#### Scenario: Personal environment text is replaced
- **WHEN** a source text artifact contains a personal path, account, private IP,
  or serial identifier
- **THEN** the public artifact SHALL use the declared role placeholder
- **AND** the redaction ledger SHALL record original and sanitized SHA-256

### Requirement: Reused Hardware Evidence Does Not Become A Fresh Tag Claim
Target evidence gathered before the public release commit SHALL retain its
original scope and SHALL NOT be promoted to fresh release verification.

#### Scenario: Release delta affects a target claim
- **WHEN** public curation changes a runtime, packaging, protocol, or oracle
  surface used by an old target proof
- **THEN** that proof SHALL be labeled historical for the public release
