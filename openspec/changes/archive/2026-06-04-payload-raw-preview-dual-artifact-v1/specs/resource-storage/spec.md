## MODIFIED Requirements

### Requirement: Canonical Payload Artifacts Use The Official Data-Products Root
The resource-storage baseline SHALL store canonical payload artifacts under the
official data-products runtime root instead of the payload persistent-data
capture root.

#### Scenario: Official payload artifacts stay distinct from local dual-artifact residue
- **WHEN** `PayloadOpsController` publishes preview or raw official payload
  `.fdp`
- **THEN** `DpWriter` SHALL place that artifact under
  `<runtime-root>/data-products/`
- **AND** the payload persistent-data capture root SHALL remain reserved for
  local `PIC%02X.bin/.jpg` review artifacts only

### Requirement: Local Payload Capture Root Uses Deterministic Dual-Artifact Names
The governed payload persistent-data capture root SHALL use deterministic local
artifact names for raw and preview capture outputs.

#### Scenario: Capture index maps to local artifact names
- **WHEN** a still capture succeeds with `captureIndex = XX`
- **THEN** the runtime SHALL store the raw capture as `PIC%02X.bin`
- **AND** it SHALL store the preview JPEG as `PIC%02X.jpg`
- **AND** legacy `capture-<boot>-<captureId>.jpg/.json` naming SHALL NOT remain
  the active baseline naming rule for new captures
