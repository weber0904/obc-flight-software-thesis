## ADDED Requirements

### Requirement: Interface index separates numeric transport derivation from path proof

The interface contract index SHALL describe each frozen current transport
ceiling with both its numeric derivation source and the narrower path-scope
proof class that makes the ceiling relevant to the active baseline.

#### Scenario: Reviewers can audit a derived ceiling without mistaking it for direct runtime proof

- **WHEN** `docs/interfaces.md` records a current command or file/downlink
  ceiling
- **THEN** the row SHALL identify the checked-in constants or serializer
  formula that produced the number
- **AND** it SHALL separately identify the governed path whose contract uses
  that ceiling

#### Scenario: Hosted framing facts stay distinct from admitted payload ceilings

- **WHEN** the interface index records CCSDS frame size, SCID, or VCID facts
- **THEN** it SHALL keep hosted/configured framing facts distinct from the
  admitted current command or file/downlink payload ceilings
- **AND** it SHALL NOT present a hosted frame-size value as the same proof
  class as a source-derived inner-payload ceiling

### Requirement: Interface index records current APID governance and change control

The interface contract index SHALL record the current `ComCfg.Apid`
reservation policy, distinguish active path-proven operational flows from
reserved or invalid classes, and state how future APID claims are governed.

#### Scenario: Reviewers can audit the current APID split in one place

- **WHEN** a reviewer inspects the APID section of `docs/interfaces.md`
- **THEN** they SHALL be able to see which APIDs are active path-proven flows
- **AND** they SHALL be able to see which values are reserved current-code
  classes, special reserved values, or invalid

#### Scenario: Future APID expansion does not collapse into code drift

- **WHEN** the index documents the current APID map
- **THEN** it SHALL state that new active APID claims require a governed change
  that updates code, current docs, formal specs, and evidence together

## MODIFIED Requirements

### Requirement: Interface contract index records the reliable-transfer boundary

The interface contract index SHALL record the owner and retry boundary for
`reliable-transfer-v1`.

#### Scenario: Owner boundary is reviewed

- **WHEN** the current node-`5` reliable-transfer slice is described
- **THEN** the index SHALL state that `CommController` remains the policy owner
- **AND** it SHALL state that a bounded helper owns in-transfer send/ACK/retry
  execution
- **AND** it SHALL state that `DpCatalog` remains the file-selection owner

#### Scenario: Retry boundary is reviewed

- **WHEN** the change documents retries
- **THEN** it SHALL distinguish ground whole-command retry from reliable
  transfer timeout/resend semantics
- **AND** it SHALL state that the reliable-transfer slice does not reopen
  command authority, accepted `SESSION_OPEN(seq0)`, or gateway role policy

#### Scenario: Reliable-transfer segment size stays path-local

- **WHEN** the interface index records the `160`-byte reliable-transfer
  segment ceiling
- **THEN** it SHALL identify that value as belonging only to the bounded
  default S-band node-`5` helper path
- **AND** it SHALL NOT present that segment size as a repo-wide transport MTU
