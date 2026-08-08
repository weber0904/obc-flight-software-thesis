## MODIFIED Requirements

### Requirement: Payload FDP Reuses Current Official Catalog Ownership
Payload official delivery SHALL continue to reuse the current
`DpCatalog -> CommController -> FileDownlink` ownership chain.

#### Scenario: Preview and raw payload products keep stock catalog ownership
- **WHEN** the controller publishes preview or raw payload `.fdp` artifacts
- **THEN** the official operator flow SHALL remain `BUILD_CATALOG` and
  `START_XMIT_CATALOG`
- **AND** the payload slice SHALL NOT introduce a second payload-specific file
  owner or generic arbitrary-file delivery path

### Requirement: Stock File Downlink Packet Budget Is Raised For Official FDP Families
The active comm baseline SHALL use a larger stock file/downlink packet budget
for official `.fdp` families.

#### Scenario: File packet and TM frame sizing are raised together
- **WHEN** the active baseline builds the stock file/downlink path
- **THEN** `FW_FILE_BUFFER_MAX_SIZE` SHALL be `2048`
- **AND** `ComCfg::TmFrameFixedSize` SHALL be at least `4096` so the CCSDS
  framer remains valid with that file-buffer budget
- **AND** the S-band file-buffer pool SHALL use `4096`-byte buffers with a
  governed count of `32`

### Requirement: This Change Does Not Add Payload-Specific QoS Governance
The active comm baseline SHALL keep payload delivery on the existing stock file
path without adding a new token-bucket or file QoS policy in this change.

#### Scenario: Larger file packets do not imply new QoS claims
- **WHEN** payload preview or raw official `.fdp` delivery uses the stock
  file-downlink path
- **THEN** the change SHALL rely only on the larger stock packet budget
- **AND** it SHALL NOT claim a new payload-specific shaping or rate-governance
  mechanism unless a later explicit change adds it
