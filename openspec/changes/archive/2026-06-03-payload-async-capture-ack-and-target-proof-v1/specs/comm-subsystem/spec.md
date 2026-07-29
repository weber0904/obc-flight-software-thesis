## MODIFIED Requirements

### Requirement: Payload FDP Reuses Current Official Catalog Ownership

The comm subsystem SHALL treat canonical payload `.fdp` artifacts as part of
the existing official `DpCatalog`-owned data-product delivery path.

#### Scenario: Governed target node-5 payload proof reuses the stock official path

- **WHEN** the repository proves canonical payload `.fdp` delivery on the
  governed target node-`5` S-band path
- **THEN** the proof SHALL use the stock `BUILD_CATALOG` plus
  `START_XMIT_CATALOG` delivery chain on that path
- **AND** it SHALL keep payload official closure distinct from Pi-local direct
  payload capture evidence
