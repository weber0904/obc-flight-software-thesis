## Why

The repository has reached a point where its software-only, split-host, external comm, and live GPS UART baselines are all usable, but the overall communication architecture is still described through adjacent paths rather than one coherent system story. In particular, the current direct `GDS -> TCP -> OBC` development path, the external `comm` subsystem path, the internal `libcsp` subsystem path, and the direct `GPS` sensor path are all valid but not yet formally realigned around the intended long-term spacecraft architecture.

This change is needed now to prevent future hardware bring-up work from drifting into contradictory assumptions. The project needs a governed architecture that preserves the existing development baselines while clearly defining how ground TT&C, internal subsystem networking, GPS ingest, and later physical-link migration are supposed to fit together.

## What Changes

- Formalize four distinct communication domains and their boundaries:
  - direct `GDS` development path
  - omitted-RF ground TT&C path through the `comm` subsystem
  - internal subsystem `libcsp` path
  - direct `GPS -> OBC` sensor path
- Reclassify `comm` as the long-term spacecraft-side ground-facing subsystem instead of treating the current `mock-text` path as the end-state architecture.
- Add a new governed capability for a `Ground TT&C Gateway` that will sit between ground-side tools and the lab-side omitted-RF ingress path, instead of making a custom `fprime-gds` communication plugin the first implementation step.
- Establish the near-term internal spacecraft bus direction:
  - `EPS`, `ADCS`, and `COMM` are future CSP-facing subsystems
  - `GPS` remains a direct OBC-attached UART source for the next phase
  - shared `CAN FD` becomes the planned physical carrier direction for spacecraft-side subsystem traffic
- Capture the confirmed lab hardware allocation:
  - `obc.local` uses one CAN FD controller plus one direct GPS UART
  - `subsystem.local` uses two CAN FD channels, with `EPS/ADCS` sharing one channel group and `COMM` using the other, plus one UART for omitted-RF lab ingress
- Define follow-on implementation slices that preserve Stage 1 software-only and Stage 2 split-host validation even after Stage 3 physical-link migration begins.
- Update the architecture story to reflect that `gps-live-uart-source-v1` has already established the first governed direct `GPS -> OBC` hardware path.

## Capabilities

### New Capabilities
- `ground-ttc-gateway`: governed omitted-RF gateway architecture between ground-side tooling and the spacecraft-side `comm` subsystem

### Modified Capabilities
- `platform-baseline`: redefine the near-term system architecture so software-only, split-host, and physical-link stages are explicit and non-conflicting
- `comm-subsystem`: change the long-term role of `comm` from a standalone external-link baseline into the spacecraft-side TT&C subsystem while preserving current development evidence as bounded history
- `core-system-contracts`: clarify subsystem roles, carrier boundaries, and where future TT&C and CSP-facing responsibilities live
- `gps-subsystem`: lock near-term GPS direction to direct OBC UART instead of internal CSP integration
- `verification-evidence`: require future evidence to keep direct GDS, omitted-RF TT&C, internal CSP, and GPS paths distinct
- `verification-path-registry`: register the future path families and guard against reusing the wrong evidence when physical-link work starts

## Impact

- Affected systems:
  - OBC communication architecture and future integration order
  - ground-side tooling and future gateway processes
  - subsystem-host deployment strategy on `obc.local` and `subsystem.local`
  - future `CAN FD` / `UART` hardware validation work
- Affected code areas in later slices:
  - `OBC/Components/CommController`
  - `OBC/Components/RadioController`
  - `OBC/Components/UartDriver`
  - `OBC/Components/GpsBridge`
  - future CSP carrier/runtime layers
  - future ground-side gateway tooling
- No immediate F' public API or command dictionary change is required in this architecture-realignment slice itself, but it establishes the contract for later implementation changes.
