## ADDED Requirements

### Requirement: Canonical Payload Artifacts Use The Official Data-Products Root
The resource-storage baseline SHALL store canonical payload artifacts under the official data-products runtime root instead of the payload persistent-data capture root.

#### Scenario: Canonical payload artifact stays distinct from local capture residue
- **WHEN** `PayloadOpsController` publishes a canonical payload `.fdp`
- **THEN** `DpWriter` SHALL place that artifact under `<runtime-root>/data-products/`
- **AND** the payload persistent-data capture root SHALL remain reserved for the local `.jpg + .json` artifacts only

### Requirement: Local Payload Capture Root Remains A Diagnostic Surface
The governed payload persistent-data capture root SHALL remain available after canonical payload promotion without being restated as the formal delivery root.

#### Scenario: Persistent-data payload files do not replace canonical payload delivery
- **WHEN** reviewers inspect payload artifacts under `<runtime-root>/persistent-data/payload/camera/`
- **THEN** they SHALL be able to use those files for local diagnosis and parity checks
- **AND** active baseline wording SHALL NOT describe that root as the formal stored/downlink payload artifact root once canonical payload `.fdp` promotion exists
