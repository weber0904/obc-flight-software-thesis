## MODIFIED Requirements

### Requirement: Public Evidence Separates Summary From Raw Artifacts
Public test-record summaries, results, and provenance SHALL remain in Git under
`evidence/records/`, while raw `artifacts/` trees SHALL be distributed through
a checksummed release asset.

#### Scenario: Test record has raw artifacts
- **WHEN** a source test record includes an `artifacts/` subtree
- **THEN** its public `evidence/records/<id>/` directory SHALL contain an
  `ARTIFACTS.json` descriptor
- **AND** `evidence/catalog.json` SHALL bind the record and descriptor to the
  release asset
- **AND** source and public digests SHALL remain machine-verifiable
