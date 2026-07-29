# resource-storage Specification Delta

## ADDED Requirements

### Requirement: Payload Capture Sidecar Metadata Shares The Governed Root

Successful payload captures SHALL store metadata sidecars under the same
governed payload capture root as the JPEG artifact.

#### Scenario: Capture metadata lives under payload capture storage

- **WHEN** the payload contract records successful still-capture metadata
- **THEN** it SHALL store that metadata under
  `<runtime-root>/persistent-data/payload/camera/`
- **AND** it SHALL NOT require a separate database or release-payload write
  path
