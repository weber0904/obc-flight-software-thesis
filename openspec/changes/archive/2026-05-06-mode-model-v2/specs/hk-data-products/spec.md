## ADDED Requirements

### Requirement: HK Trend Mode V2 Compatibility
The HK trend official F' data product SHALL store the current primary spacecraft mode using the `SatMode` v2 enum while preserving the `HkTrendRecord` product record name and id and advancing the payload schema version to distinguish the new mode semantics.

#### Scenario: HK trend stores v2 mode
- **WHEN** `HkTrendProductProducer` serializes a record from a state snapshot
- **THEN** the `mode` field SHALL use one of `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, or `TTC`
- **AND** it SHALL NOT serialize retired primary mode names as current mission modes

#### Scenario: HK trend product identity remains stable
- **WHEN** the mode-model-v2 change updates the mode enum used by `HkTrendRecord`
- **THEN** the product record SHALL remain `HkTrendRecord` id `0`
- **AND** the evidence SHALL decode the current `version = 3` record with the regenerated dictionary
