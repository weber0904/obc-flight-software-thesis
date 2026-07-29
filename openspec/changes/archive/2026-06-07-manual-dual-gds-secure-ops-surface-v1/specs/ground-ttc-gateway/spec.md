## ADDED Requirements

### Requirement: Manual Dual-GDS Operator Surface Lives In A Dedicated Subtree

The ground TT&C gateway capability SHALL provide its maintained manual
dual-GDS operator entrypoints from a dedicated `scripts/manual_ops/` subtree
rather than by scattering new operator wrappers across the root `scripts/`
surface.

#### Scenario: Manual operator family is easy to discover
- **WHEN** an operator needs the maintained manual dual-GDS surface
- **THEN** the repo SHALL provide hosted and target entrypoints under
  `scripts/manual_ops/`
- **AND** the root `scripts/README.md` SHALL route readers to that subtree
  instead of introducing new root-level `run_*` wrappers for this family.

### Requirement: Manual Dual-GDS Surface Keeps Stock GDS Plus Gateway Boundary

The maintained manual operator family SHALL keep stock `fprime-gds` plus
repo-owned `ground_ttc_gateway` as the current ground operator boundary.

#### Scenario: Manual operator UI does not replace stock GDS
- **WHEN** the manual dual-GDS surface is started in hosted or target mode
- **THEN** each band surface SHALL still use stock `fprime-gds` northbound of
  `ground_ttc_gateway`
- **AND** the operator truth SHALL remain two separate per-band GDS surfaces
  rather than one custom simultaneous multiplexer.

### Requirement: Manual Dual-GDS Surface Supports UI And Headless GDS Modes

The maintained manual dual-GDS operator family SHALL support `GDS_UI_MODE`
selection while preserving the current headless launcher defaults used by
existing proof and automation surfaces.

#### Scenario: Manual surface can run in human-facing UI mode
- **WHEN** an operator starts the maintained manual dual-GDS surface with
  `GDS_UI_MODE=ui`
- **THEN** the surface SHALL launch stock `fprime-gds` in UI-capable mode
- **AND** it SHALL still publish the same manifest/status contract as the
  corresponding headless run.

#### Scenario: Existing proof defaults remain headless
- **WHEN** existing maintained proof or probe wrappers use the shared GDS
  launchers without requesting UI mode
- **THEN** those wrappers SHALL keep the current headless behavior by default
- **AND** the manual operator addition SHALL NOT silently redefine them as GUI
  workflows.
