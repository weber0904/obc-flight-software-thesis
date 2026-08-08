## MODIFIED Requirements

### Requirement: Official HK `.fdp` transfers may use the bounded reliable path

The HK data-product baseline SHALL allow current official HK `.fdp` whole-file
transfers to traverse the bounded reliable sidecar on either current exact
allowed path:

- default S-band node-`5`
- explicit-switched `uhf-primary-after-failover` node-`6`

#### Scenario: Official HK `.fdp` generation remains unchanged

- **WHEN** `uhf-reliable-transfer-v1` is enabled for a selected official HK
  `.fdp` request
- **THEN** `DpWriter`, `DpCatalog`, and the current official HK `.fdp`
  generation flow SHALL remain unchanged before transmit admission
- **AND** `DpCatalog` SHALL still own catalog build and selected transmit
  initiation

#### Scenario: Completion truth remains whole-file at the catalog boundary

- **WHEN** a selected official HK `.fdp` request completes through either
  bounded reliable path
- **THEN** `DpCatalog` SHALL still receive one final whole-file completion
  result
- **AND** that result SHALL be derived from reliable-transfer success or final
  failure instead of stock ground-side file-store completion alone

#### Scenario: Local ceiling remains bounded on both current paths

- **WHEN** a selected official HK `.fdp` request exceeds the bounded helper
  ceiling of `64` segments / `10,240` bytes
- **THEN** `CommController` SHALL reject the reliable-transfer start before
  in-transfer delivery begins
- **AND** node-`5` or node-`6` reliable-transfer reception SHALL reject a
  `BEGIN` that advertises a larger file or segment count
