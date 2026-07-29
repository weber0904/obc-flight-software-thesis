## Context

The `core-system-contracts` capability already defines the contract boundary for shared types and core public symbol families, but the codebase still contains no project-owned FPP definitions or components. Subsequent subsystem changes would otherwise have to invent their own temporary type and command interfaces, creating drift against the newly archived formal baseline.

The first implementation slice should therefore establish only the shared types, identifiers, and core component shells needed to anchor later work. It should not attempt to implement the full CSP stack, deployment wiring, or hardware-driven health logic yet.

## Goals / Non-Goals

**Goals:**

- Create the first project-local FPP type module for shared enums and constants.
- Implement minimal but real F' components for `ModeManager`, `HealthMonitor`, and `CspBridge`.
- Ensure those components build in the normal project build and in unit-test mode.
- Exercise the main contract paths required by the current spec: mode changes, health-threshold handling, and CSP init/ping/send command flows.
- Preserve single ownership of the `MODE_*`, `HEALTH_*`, `SYS_*`, and `CSP_*` families.

**Non-Goals:**

- Do not implement a full deployment topology in this change.
- Do not integrate with libcsp or ZMQ transport yet.
- Do not implement subsystem-specific command/tlm/event families.
- Do not attempt real host resource sampling or hardware health polling yet.

## Decisions

### Decision: Keep shared types in a dedicated project-local FPP module

The first shared enums and constants will live under `OBC/Types/` so later components can reference one project-owned location for `SatMode`, `AdcsMode`, `CommBand`, `BootSlot`, `HealthItem`, base ID ranges, and CSP node allocations.

Alternative considered:

- Define the types inline in each component. Rejected because it would immediately violate the single-owner contract boundary.

### Decision: Use lightweight component skeletons before full subsystem integration

`ModeManager`, `HealthMonitor`, and `CspBridge` will implement the public command, telemetry, and event families now, while their internal logic remains intentionally simple and integration-friendly.

Alternative considered:

- Wait to implement these until the subsystem changes arrive. Rejected because later changes need a stable shared contract anchor.

### Decision: Model CSP behavior as a local diagnostic shim in this change

`CspBridge` will implement initialization state, counters, and command outcomes without binding to the real CSP stack yet. This satisfies the current diagnosability requirement while avoiding premature network-stack coupling.

Alternative considered:

- Pull in libcsp and ZMQ transport now. Rejected because transport integration belongs in later platform and subsystem changes.

### Decision: Use unit tests as the first implementation evidence

Each new core component will ship with a focused unit-test harness, and this change will record the results under `evidence/records/`.

Alternative considered:

- Rely on build-only evidence. Rejected because the core contracts already include command and telemetry behavior that should be exercised automatically.

## Risks / Trade-offs

- **The initial component behaviors are intentionally shallow** -> Record that these are contract-establishing skeletons and leave deeper behavior to later changes.
- **No deployment exists yet to drive real uptime or health sampling** -> Provide explicit helper methods and unit tests so the public interfaces are still executable and reviewable.
- **UT helper naming is generated from FPP** -> Keep component and symbol names close to the formal capability wording to avoid surprising generated interfaces.

## Migration Plan

1. Create the OpenSpec artifacts for `core-system-contracts-v1`.
2. Add the project-local shared type and constant definitions under `OBC/Types/`.
3. Implement `ModeManager`, `HealthMonitor`, and `CspBridge` under `OBC/Components/`.
4. Add unit tests for the minimum required behaviors.
5. Run the normal build and unit-test build.
6. Capture evidence under `evidence/records/core-system-contracts-v1/`.
7. Validate the change and archive it when complete.

## Open Questions

None that block this implementation slice. The current behavior intentionally leaves real transport binding, deployment wiring, and host resource sampling for later changes.
