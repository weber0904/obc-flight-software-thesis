## ADDED Requirements

### Requirement: Registry Distinguishes Governed Sequence Upload And Execution Path

The verification path registry SHALL distinguish the new governed hosted sequence-upload and official-sequence-execution path from adjacent command, file-downlink, or TTC policy paths.

#### Scenario: New hosted path is registered with bounded claims

- **WHEN** `official-sequencing-system-resources-v1` evidence is recorded
- **THEN** the registry SHALL identify the path as the current active hosted CCSDS file-upload to sequence staging plus admitted official-sequence execution path
- **AND** it SHALL describe what it proves and what it does not prove
- **AND** it SHALL keep this path distinct from command-auth/session paths, file-downlink paths, and TTC pass-window policy evidence
