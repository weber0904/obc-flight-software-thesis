## Why

The repository now has a formal baseline and a working F' project scaffold, but it still lacks the first project-owned types and component contracts that later subsystem work depends on. This change turns the narrative `core-system-contracts` capability into concrete F' artifacts by defining shared enums and constants, plus the first implementation skeletons for `ModeManager`, `HealthMonitor`, and `CspBridge`.

## What Changes

- Add the shared core FPP types and constants for base ID ranges, CSP node allocations, shared modes, boot slots, and health item identifiers.
- Implement project-local F' component skeletons for `ModeManager`, `HealthMonitor`, and `CspBridge`.
- Expose the `MODE_*`, `HEALTH_*`, `SYS_*`, and `CSP_*` command, event, and telemetry families from those components without duplicating subsystem-specific public symbols.
- Add unit tests that exercise the minimum core command and telemetry flows needed by later changes.
- Capture implementation-phase evidence for the new core contracts.

## Capabilities

### Modified Capabilities

- `core-system-contracts`: add concrete F' types, constants, component interfaces, and minimum validated behaviors for shared core contracts.
- `verification-evidence`: store the implementation-phase evidence for the new core contract components and their unit-test gate.

## Impact

- Affected files: `OBC/Types/`, `OBC/Components/`, `openspec/changes/core-system-contracts-v1/`, `evidence/records/`
- Affected systems: shared OBC type definitions, command/tlm/event ownership, future subsystem integration points
- Dependencies: F' v4.1.0 build environment, generated project-local virtual environment, `fprime-util` regular and unit-test build flows
