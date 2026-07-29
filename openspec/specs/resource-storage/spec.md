# resource-storage Specification

## Purpose
Define the Linux/Raspberry Pi resource model, storage layout, metadata persistence rules, and constrained-validation handling for the first version.
## Requirements
### Requirement: Resource Budgets
The project SHALL express first-version resource limits using Linux-oriented budgets for RSS, thread count, open file count, and buffer allocations rather than MCU-style raw memory maps.

#### Scenario: Target resource model remains Linux-oriented
- **WHEN** the resource baseline is reviewed for Raspberry Pi 3B+
- **THEN** the documented limits SHALL use process and filesystem terms such as RSS, threads, files, and buffer pools

### Requirement: Storage Layout And Metadata
The system SHALL define storage roles for Slot A, Slot B, staging, persistent data, and logs/evidence, boot metadata v1 SHALL be stored as structured files in persistent storage rather than in a database, and the runtime SHALL allow hosted and Raspberry Pi profiles to override the staging, persistent-data, and evidence/log roots without changing `BootManager` logic.

#### Scenario: Boot metadata storage choice
- **WHEN** the boot/update subsystem persists `active_slot`, `pending_slot`, or related metadata
- **THEN** that metadata SHALL be written to file-backed persistent storage and SHALL NOT require an embedded database

#### Scenario: Raspberry Pi runtime uses target-specific roots
- **WHEN** the `integ-rpi` profile launches on a Raspberry Pi target
- **THEN** the runtime SHALL be able to place staging data, persistent metadata, and evidence/log outputs under target-specific filesystem roots while preserving the same logical storage roles

### Requirement: Monitoring And Constrained Validation
The resource baseline SHALL define monitoring thresholds for memory, CPU, CSP free buffers, staging capacity, and log capacity, SHALL record the actual target-side storage and evidence paths used during Raspberry Pi validation, and SHALL allow hardware-limited checks to be marked as `Blocked-HW` or `Deferred-RPi` with replacement evidence.

#### Scenario: Hardware-limited storage validation
- **WHEN** a storage or target-platform validation cannot run because Raspberry Pi hardware is unavailable
- **THEN** the validation record SHALL include the correct constrained status and replacement evidence instead of silently dropping the test

#### Scenario: Target evidence names the real storage roots
- **WHEN** a Raspberry Pi target integration run is recorded
- **THEN** the evidence SHALL identify which target filesystem roots were used for staging, persistent metadata, and logs/evidence collection

### Requirement: Installed Release Layout
The Raspberry Pi target SHALL separate immutable installed release payloads from mutable runtime state by using a fixed install root with distinct release and runtime areas, and the installed stack SHALL keep persistent data, staging data, and logs outside the versioned release payload so release switching does not overwrite mutable state.

#### Scenario: Install root preserves runtime data across releases
- **WHEN** a new Raspberry Pi bundle is installed under the governed install root
- **THEN** the release payload SHALL live under a versioned release directory while staging, persistent-data, and logs SHALL remain under shared runtime roots outside that release directory

### Requirement: Retired Housekeeping Archive Runtime Roots Stay Inactive
The runtime storage model SHALL NOT treat legacy housekeeping archive files or
the legacy housekeeping archive index as active runtime-root storage after the
HK fallback retirement. Historical evidence may still cite the old `hk/` root,
but current hosted or target-facing runtime storage SHALL use official
`data-products`, `persistent-data`, `staging`, and `logs` roots instead.

#### Scenario: Release switching does not recreate retired HK history
- **WHEN** the governed runtime launches from a hosted or Raspberry Pi runtime root
- **THEN** it SHALL NOT create `runtime/hk`, `hk/index.csv`, or HK slot files as active mission-history artifacts
- **AND** release switching SHALL preserve current mutable roots without relying on the retired HK archive root

### Requirement: Governed Runtime-Root Storage Monitoring
The resource-storage baseline SHALL provide observability for the governed
`persistent-data`, `staging`, `logs`, and `data-products` runtime roots, and
that path SHALL surface root-specific warning or degraded states instead of
requiring direct filesystem inspection as the only review method.

#### Scenario: Runtime-root storage state becomes reviewable
- **WHEN** the first storage-health slice completes a scan on the hosted runtime
- **THEN** the project SHALL be able to review bounded root statistics and warning status for the governed runtime roots through repository-owned runtime contracts and evidence

### Requirement: Derived Storage Capabilities Reuse The Shared Baseline
The resource-storage baseline SHALL remain the governing source for storage roles, mutable runtime-root boundaries, and constrained-validation terminology even when later dedicated capabilities such as storage observability or archive management are introduced.

#### Scenario: Storage-oriented capability extends the baseline without redefining roots
- **WHEN** a later capability adds storage-oriented behavior such as runtime-root observability or archive indexing
- **THEN** that capability SHALL reuse the shared storage roles from `resource-storage` instead of redefining staging, persistent-data, logs, or governed runtime-root ownership independently

### Requirement: Official Data Product Runtime Root
The resource-storage baseline SHALL reserve a governed mutable runtime-root path for official F' data product files and catalog state, and that root SHALL remain outside immutable installed release payloads.

#### Scenario: Data product files live under mutable runtime root
- **WHEN** the OBC runtime is launched with a governed runtime root
- **THEN** official F' data product files SHALL be written under a mutable `data-products` runtime-root directory
- **AND** those files SHALL NOT be written into an immutable installed release payload

#### Scenario: Data product catalog state lives with data products
- **WHEN** the official F' data product catalog persists transmission state
- **THEN** the catalog state file SHALL live under the governed data-products runtime-root area

### Requirement: Storage Roles Stay Distinct
Official HK trend data products SHALL remain distinct from persistent
boot/update data, staging data, logs, and the retired housekeeping archive
ring/index history.

#### Scenario: Retired housekeeping archive remains separate
- **WHEN** the HK trend data-product path writes files
- **THEN** it SHALL use the official data-products root
- **AND** it SHALL NOT write into, replace, or recreate the retired `hk/` ring archive root

### Requirement: Data Products Runtime Root Policy Surface
The governed runtime-root storage model SHALL include `<runtime-root>/data-products/` as the official F' data-product file and catalog-state location with observe-only quota visibility.

#### Scenario: Data products root is governed storage
- **WHEN** the OBC topology configures official F' data products
- **THEN** `DpWriter` output and `DpCatalog` state SHALL remain under `<runtime-root>/data-products/`
- **AND** storage-health SHALL scan that same root rather than a hard-coded `runtime/data-products` path

#### Scenario: Quota configuration is non-destructive
- **WHEN** `OBC_DATA_PRODUCTS_QUOTA_BYTES` is set
- **THEN** storage-health SHALL expose the configured quota and over-quota status for `<runtime-root>/data-products/`
- **AND** the system SHALL NOT delete data-product files as part of this change

### Requirement: Recovery Boot Metadata Reuses The Shared File-Backed Store
The resource-storage baseline SHALL keep recovery-related boot metadata in the same governed persistent-data storage model as the rest of boot metadata instead of introducing a parallel recovery database or separate persistence root.

#### Scenario: Recovery boot fields stay under governed persistent-data root
- **WHEN** the runtime persists reset-cause, boot-count, consecutive-reset, or boot-safe-fallback truth
- **THEN** those fields SHALL live in the existing boot metadata file under the governed persistent-data runtime root
- **AND** they SHALL remain subject to the same hosted and Raspberry Pi root-override model already used for boot metadata

### Requirement: Persistent Fault Ring Uses Governed Recovery Root
The resource-storage baseline SHALL reserve
`<persistent-data-root>/recovery/` for the persistent fault ring and SHALL keep
`fault-ring-a.bin` and `fault-ring-b.bin` under that governed mutable root
outside immutable installed release payloads.

#### Scenario: Hosted and target profiles share the same logical recovery root
- **WHEN** the runtime launches under hosted or Raspberry Pi profiles with
  target-specific root overrides
- **THEN** the persistent fault ring SHALL resolve under the configured
  persistent-data root as `recovery/fault-ring-a.bin` and
  `recovery/fault-ring-b.bin`
- **AND** it SHALL NOT require a separate install-time release payload location

### Requirement: Persistent Fault Ring Stays Distinct From Other Stores
The governed storage model SHALL keep persistent fault history distinct from
boot metadata, staging data, logs, and official `.fdp` data-product storage.

#### Scenario: Recovery ring does not replace boot metadata or official data products
- **WHEN** the runtime persists recovery breadcrumbs in this change
- **THEN** the persistent fault ring SHALL use the governed recovery root under
  persistent-data
- **AND** it SHALL NOT rewrite boot metadata into the ring, write `.fdp` files,
  or reuse staging or logs roots as its primary store

### Requirement: Payload Captures Use A Governed Persistent-Data Root

The resource-storage baseline SHALL reserve a governed persistent-data path for
camera payload captures in the first payload operation slice.

#### Scenario: Payload capture files stay under persistent-data

- **WHEN** the payload contract writes a successful still image
- **THEN** the file SHALL be written under
  `<runtime-root>/persistent-data/payload/camera/`
- **AND** it SHALL NOT be written into immutable release payloads, staging,
  logs, or the official data-products runtime root

### Requirement: Payload Capture Naming Is Deterministic In V1

The first payload operation slice SHALL use a deterministic local naming rule
for camera captures.

#### Scenario: Capture file name is reviewable from runtime state

- **WHEN** a still capture completes successfully
- **THEN** the payload implementation SHALL assign the file a deterministic
  governed name derived from the ground-selected `captureIndex`
- **AND** the last-result readback SHALL allow reviewers to correlate payload
  status with that stored artifact

### Requirement: Payload Capture Stores Both Local Raw And Preview Artifacts

Successful payload captures SHALL store both local raw bytes and a local
preview JPEG under the governed payload capture root.

#### Scenario: Capture artifacts use dual local files

- **WHEN** the payload contract records a successful still capture
- **THEN** it SHALL store one local raw artifact such as `PIC%02X.bin`
- **AND** it SHALL store one local preview JPEG such as `PIC%02X.jpg`
- **AND** both artifacts SHALL live under
  `<runtime-root>/persistent-data/payload/camera/`
- **AND** the change SHALL retire legacy `capture-<bootCount>-<captureCount>`
  payload naming and local `.json` sidecars from the active payload baseline

### Requirement: Canonical Payload Artifacts Use The Official Data-Products Root
The resource-storage baseline SHALL store canonical payload artifact families under the official data-products runtime root instead of the payload persistent-data capture root.

#### Scenario: Canonical payload artifact stays distinct from local capture residue
- **WHEN** `PayloadOpsController` publishes a canonical payload `.fdp` family
- **THEN** `DpWriter` SHALL place that artifact family under `<runtime-root>/data-products/`
- **AND** the payload persistent-data capture root SHALL remain reserved for the local `.bin + .jpg` artifacts only

### Requirement: Local Payload Capture Root Remains A Diagnostic Surface
The governed payload persistent-data capture root SHALL remain available after canonical payload promotion without being restated as the formal delivery root.

#### Scenario: Persistent-data payload files do not replace canonical payload delivery
- **WHEN** reviewers inspect payload artifacts under `<runtime-root>/persistent-data/payload/camera/`
- **THEN** they SHALL be able to use those files for local diagnosis and parity checks
- **AND** active baseline wording SHALL NOT describe that root as the formal stored/downlink payload artifact root once canonical payload `.fdp` family promotion exists

### Requirement: Payload Capture Manifest Persistence Stays Under Governed Storage

The payload baseline SHALL keep capture-index metadata persistence under the
governed payload capture root so later raw promotion can recover capture
identity without requiring a database or a local `.json` sidecar.

#### Scenario: Capture-index catalog stays under payload persistent storage

- **WHEN** a later command promotes a stored raw artifact into an official
  payload `.fdp` family
- **THEN** the runtime SHALL be able to recover the required capture metadata
  from governed persistent storage under the payload capture root
- **AND** it SHALL NOT require a separate database service or immutable
  release-payload write path

### Requirement: Storage Cache Maintenance And Operator Readback Stay Distinct

The current resource-storage baseline SHALL distinguish scheduled storage cache
maintenance from explicit operator readback and SHALL keep the latter
fresh-by-default.

#### Scenario: Scheduled storage refresh remains background maintenance
- **WHEN** current runtime-root storage visibility is described
- **THEN** the baseline SHALL allow `StorageHealthBridge` to keep a scheduled
  scan cadence for onboard cache maintenance, reduced-state consumers, and
  threshold detection
- **AND** it SHALL NOT restate that background cadence as the operator's only
  status-readback mechanism.

#### Scenario: Operator storage readback is explicitly fresh
- **WHEN** the current baseline describes operator storage status behavior
- **THEN** it SHALL identify `STORAGE_GET_STATUS` as the fresh scan readback
  command
- **AND** it SHALL NOT require a separate current `STORAGE_SCAN_NOW` operator
  command to obtain a fresh scan.

