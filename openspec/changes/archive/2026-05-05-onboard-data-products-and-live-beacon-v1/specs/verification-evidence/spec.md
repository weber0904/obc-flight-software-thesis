## ADDED Requirements

### Requirement: Replacement Change Evidence
The verification evidence baseline SHALL record reviewable evidence for reduced-state behavior, live beacon broadcast decode, official F' HK trend data product generation, catalog behavior, and explicit scope exclusions.

#### Scenario: Reduced-state evidence is reviewable
- **WHEN** focused helper and component tests pass
- **THEN** the evidence SHALL identify reduced-state classification, missing-source behavior, and stale-state invalidation coverage

#### Scenario: Beacon broadcast evidence is reviewable
- **WHEN** the live beacon hosted probe passes
- **THEN** the evidence SHALL identify the simulated COMM-facing broadcast path, captured beacon sequence, decoded payload fields, CRC result, and source values used for comparison
- **AND** the evidence SHALL state that RF behavior and ground ACK behavior are not covered

#### Scenario: Official HK data product evidence is reviewable
- **WHEN** the HK trend hosted probe passes
- **THEN** the evidence SHALL identify generated official F' data product files, `DpCatalog` catalog behavior, `DpCatalog` file queueing, and the absence of the superseded repo-local `catalog.csv` baseline

#### Scenario: Telemetry and event regression remains separate
- **WHEN** this change validates live beacon and HK data products
- **THEN** the evidence SHALL also confirm that original command, event, and telemetry paths continue to use the existing `TlmChan` and `ComFprime` baseline
