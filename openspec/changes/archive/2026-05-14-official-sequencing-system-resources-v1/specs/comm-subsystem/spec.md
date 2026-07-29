## ADDED Requirements

### Requirement: Active CCSDS File Ingress Has Repo-Owned Sequence-Staging Governance

The active CCSDS file-uplink path SHALL be governed by a repo-owned owner before packets reach `Svc::FileUplink`.

#### Scenario: START packet destination is governed before file creation

- **WHEN** the active `ComCcsds` file ingress receives a `Fw::FilePacket` `START`
- **THEN** a repo-owned file-ingress owner SHALL parse the packet
- **AND** it SHALL reject absolute paths, `..`, non-allowlisted logical destinations, and physical symlink escape
- **AND** it SHALL rewrite accepted logical sequence-staging destinations to the configured physical staging root before forwarding to `FileUplink`

#### Scenario: Denied transfer remains dropped until the next START

- **WHEN** a `START` packet is denied
- **THEN** subsequent `DATA`, `END`, and `CANCEL` packets for that transfer SHALL be dropped until a later `START` is received

### Requirement: File-Ingress Governance Claim Stays On The Current CCSDS Path Only

This change SHALL keep file-ingress governance claims bounded to the currently wired active CCSDS file path.

#### Scenario: Docs do not over-claim other links

- **WHEN** this change is documented or evidenced
- **THEN** it SHALL NOT claim UHF file upload governance, arbitrary unknown-packet uplink governance, or all-link file-uplink closure
