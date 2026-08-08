## ADDED Requirements

### Requirement: Current baseline transport ceilings are path-specific and source-derived

The comm subsystem SHALL freeze current transport ceilings as path-specific
repo-local contracts derived from checked-in constants and serializer formulas,
not by reinterpreting hosted framing facts as universal MTU truth.

#### Scenario: Current command ceilings are derived from the command-envelope budget

- **WHEN** the current `sband-primary` or `uhf-backup` command-ingress ceiling
  is documented
- **THEN** it SHALL be derived from
  `FW_CMD_ARG_BUFFER_MAX_SIZE - command-envelope-fixed-overhead`
- **AND** the current admitted serialized inner `Fw::CmdPacket` ceiling SHALL
  be `446` bytes
- **AND** the current admitted inner command-argument ceiling SHALL be `440`
  bytes

#### Scenario: Current file/downlink ceiling is derived from stock FileDownlink sizing

- **WHEN** the current `uhf-primary-after-failover` official file/downlink
  ceiling is documented
- **THEN** it SHALL be derived from `FW_FILE_BUFFER_MAX_SIZE`,
  `Fw::FilePacket::DataPacket::HEADERSIZE`, and
  `sizeof(FwPacketDescriptorType)`
- **AND** the current admitted `Fw::FilePacket::DATA` file-data ceiling SHALL
  be `243` bytes per packet

#### Scenario: Hosted CCSDS frame size stays a framing fact

- **WHEN** the current hosted CCSDS `1024`-byte frame size is described
- **THEN** it SHALL remain a hosted/configured framing fact
- **AND** the wording SHALL NOT treat that frame size as the admitted command
  or file inner-payload ceiling for the active baseline

### Requirement: Current APID governance freezes reservation truth with proof split

The comm subsystem SHALL freeze `ComCfg.Apid` as the current baseline APID
reservation source while distinguishing active path-proven flows from reserved
current-code values and future allocation work.

#### Scenario: Current active flows stay explicit

- **WHEN** the current active CCSDS APID flows are described
- **THEN** command `0`, telemetry `1`, log/event `2`, and file `3` SHALL be
  identified as active path-proven operational flows

#### Scenario: Reserved values stay distinct from active proof

- **WHEN** the current APID reservation map is documented
- **THEN** packetized telemetry `4`, data product `5`, idle `6`, handshake
  `0x00FE`, unknown `0x00FF`, idle packet `0x07FF`, and invalid values
  `>= 0x0800` SHALL be identified as governed reserved or invalid classes
- **AND** the wording SHALL NOT imply that those values are already active
  operational flows on a proven path

#### Scenario: Future APID claims remain governed

- **WHEN** later work introduces a new active APID claim or reassigns an
  existing reserved value
- **THEN** that work SHALL update code, current docs, formal specs, and fresh
  evidence through a governed change instead of treating the checked-in enum as
  self-approving runtime truth

## MODIFIED Requirements

### Requirement: Reliable transfer uses bounded segment-window semantics

The comm subsystem SHALL define first-version reliable-transfer behavior in
terms of fixed-size file segments and cumulative ACK progress.

#### Scenario: Reliable transfer progresses within one transfer context

- **WHEN** a v1 reliable transfer is active
- **THEN** the sender SHALL emit `Fw::FilePacket`-aligned `START`, `DATA`,
  `END`, and `CANCEL` vocabulary over narrow node-`5` sidecar services
- **AND** the data-segment payload ceiling SHALL be `160` bytes on that
  bounded reliable-transfer sidecar path only
- **AND** the sender SHALL bound the cumulative ACK resend window to `2`
  segments
- **AND** the sender SHALL treat lack of forward ACK progress for one timeout
  interval as a resend trigger
- **AND** the sender SHALL stop claiming success unless final receiver
  verification passes
- **AND** the wording SHALL NOT treat the `160`-byte segment size as a generic
  repo-wide transport MTU
