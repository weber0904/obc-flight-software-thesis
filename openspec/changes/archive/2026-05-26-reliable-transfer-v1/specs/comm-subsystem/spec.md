## ADDED Requirements

### Requirement: Bounded reliable transfer on default S-band node 5 for official `.fdp`

The comm subsystem SHALL provide one bounded reliable-transfer path for current
official `.fdp` whole-file requests on the default S-band node-`5` baseline.

#### Scenario: Selected official `.fdp` requests use the reliable sidecar

- **WHEN** `CommController` admits a whole-file `.fdp` request while the
  current primary file link is S-band
- **THEN** it SHALL route that request through the bounded reliable-transfer
  sidecar instead of delegating the active sender to stock `FileDownlink`
- **AND** it SHALL keep `DpCatalog` as the upstream request/completion owner
- **AND** it SHALL preserve the current non-selected baseline path for other
  requests

### Requirement: Reliable transfer uses bounded segment-window semantics

The comm subsystem SHALL define first-version reliable-transfer behavior in
terms of fixed-size file segments and cumulative ACK progress.

#### Scenario: Reliable transfer progresses within one transfer context

- **WHEN** a v1 reliable transfer is active
- **THEN** the sender SHALL emit `Fw::FilePacket`-aligned `START`, `DATA`,
  `END`, and `CANCEL` vocabulary over narrow node-`5` sidecar services
- **AND** the data-segment payload ceiling SHALL be `160` bytes
- **AND** the sender SHALL bound the cumulative ACK resend window to `2`
  segments
- **AND** the sender SHALL treat lack of forward ACK progress for one timeout
  interval as a resend trigger
- **AND** the sender SHALL stop claiming success unless final receiver
  verification passes

### Requirement: Reliable transfer completion is distinct from whole-command retry

The comm subsystem SHALL keep ground whole-command retry separate from v1
reliable-transfer success semantics.

#### Scenario: Transfer success is established inside one admitted attempt

- **WHEN** a reliable-transfer attempt is admitted
- **THEN** success SHALL require `BEGIN` acceptance, cumulative ACK completion,
  and receiver final verification for file size, checksum, and SHA-256
- **AND** an operator reissuing the original command SHALL be treated as a new
  transfer attempt rather than a hidden part of the same success claim

### Requirement: Reliable transfer exposes bounded failure truth

The comm subsystem SHALL expose reviewable failure truth for the v1 path.

#### Scenario: Reliable transfer stops after bounded no-progress retries

- **WHEN** cumulative ACK progress does not advance and the resend budget is
  exhausted
- **THEN** the sender SHALL mark the transfer as failed
- **AND** it SHALL report partial progress in terms of highest contiguous
  segment and acknowledged bytes
- **AND** it SHALL abort the receiver-side temp transfer context instead of
  promoting a final artifact

#### Scenario: Duplicate segments arrive within the active transfer context

- **WHEN** the receiver observes a duplicate segment for the active transfer id
- **THEN** it SHALL ignore duplicate payload data
- **AND** it SHALL preserve the current contiguous ACK boundary
- **AND** duplicate observation by itself SHALL NOT convert the transfer into
  failure

### Requirement: Reliable transfer remains bounded to current baseline

The comm subsystem SHALL keep v1 bounded to the current default S-band node-`5`
baseline.

#### Scenario: Non-claims remain explicit

- **WHEN** `reliable-transfer-v1` is cited
- **THEN** it SHALL NOT be interpreted as proof of UHF reliable transfer,
  simultaneous dual-link runtime arbitration, RF closure, restart-persistent
  resume, generic arbitrary-file authority redesign, or broad CFDP platform
  adoption
