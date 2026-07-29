## ADDED Requirements

### Requirement: Payload FDP Reuses Current Official Catalog Ownership
The comm subsystem SHALL treat canonical payload `.fdp` artifacts as part of the existing official `DpCatalog`-owned data-product delivery path.

#### Scenario: Payload data products do not create a second downlink owner
- **WHEN** `DpCatalog` selects a canonical payload `.fdp` file for downlink
- **THEN** `CommController` SHALL handle that request through the same current official `DpCatalog -> FileDownlink` ownership boundary used for other official data products
- **AND** the payload slice SHALL NOT introduce a payload-specific direct file-send command or a second payload file owner

### Requirement: Payload FDP Does Not Broaden Reliable Transfer Scope
The first payload end-to-end closure slice SHALL keep the current reliable-transfer family bounded to its existing official HK `.fdp` scope.

#### Scenario: Payload official downlink stays on the stock path
- **WHEN** a canonical payload `.fdp` is downlinked on the current official path
- **THEN** the repository MAY prove byte-match and decode on the stock `DpCatalog` and `FileDownlink` path
- **AND** it SHALL NOT describe that proof as widening the current selected reliable-transfer family from official HK `.fdp` to payload `.fdp`
