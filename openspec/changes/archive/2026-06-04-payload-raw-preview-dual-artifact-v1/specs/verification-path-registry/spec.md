## MODIFIED Requirements

### Requirement: Payload Proof Paths Distinguish Preview And Raw Official Products
The verification registry SHALL register payload proof paths in terms of the
official artifact kind being proven.

#### Scenario: Hosted proof records preview and raw parity separately
- **WHEN** the hosted payload proof claims official payload downlink closure
- **THEN** the registry entry SHALL identify which cases prove
  `PREVIEW_JPEG` and which cases prove `RAW_FRAME`
- **AND** each case SHALL include `.fdp` byte-match plus repo-owned decode and
  extracted-artifact parity

#### Scenario: Governed target proof keeps full raw as explicit non-claim
- **WHEN** the governed target node-`5` payload proof records the active
  baseline
- **THEN** it SHALL include a bounded official preview proof and a bounded
  official raw proof
- **AND** it SHALL keep `FULL` raw explicitly outside the active claim until a
  later change widens the payload family model
