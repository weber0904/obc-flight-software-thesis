## Context

The formal comm subsystem baseline separates the external communications link from the internal CSP network, reserves TCP mock for the first software-only development path, and requires PTY-backed UART verification as a hardware-free substitute for real UART integration. The repository does not yet include any comm-specific components or host-side transport abstraction that can switch between those two validation modes.

This change implements the first validated comm slice without introducing a full deployment or hardware-specific radio adapter. The goal is to prove the controller roles, the transport-switch boundary, and the verification pattern rather than a flight-ready radio stack.

## Goals / Non-Goals

**Goals:**

- Implement `CommController` as the owner of `COMM_*` state, pass-window timing, and band selection behavior.
- Implement `RadioController` as the owner of `RADIO_*` commands, telemetry, and events using a transport-hidden mock radio backend.
- Implement `UartDriver` as the owner of `UART_*` telemetry and events around byte-stream link status and error accounting.
- Add a host-side byte-stream layer that can use TCP mock or PTY with the same radio mock protocol.
- Verify the comm slice with focused unit tests and one host integration test covering both TCP and PTY paths.

**Non-Goals:**

- Do not wire the comm slice into a full deployment or the F' com stack in this change.
- Do not require real Raspberry Pi hardware, GPIO, I2C, or a vendor-specific radio protocol.
- Do not declare KISS mandatory or implement a flight framing adapter.
- Do not replace the internal ZMQ-backed CSP simulator baseline.

## Decisions

### Decision: Use a host-side line protocol for the first radio mock

The first `RadioController` implementation will speak a small text-based request/response protocol over a generic byte-stream transport. This keeps transport switching visible in tests while avoiding premature commitment to a vendor framing format.

Alternative considered:

- Jump directly to a radio-specific binary protocol. Rejected because the narrative explicitly keeps KISS and other vendor framing out of the first mandatory baseline.

### Decision: Keep TCP and PTY behind a shared byte-stream interface

The project will add a reusable byte-stream abstraction with TCP mock and PTY implementations so that transport choice stays below the controller layer and can be switched in tests without changing controller logic.

Alternative considered:

- Put transport-specific logic directly in `RadioController`. Rejected because the comm baseline requires the transport difference to remain in the driver or adapter layer.

### Decision: Let `UartDriver` own connectivity accounting, not vendor protocol semantics

`UartDriver` will focus on connectivity, byte counters, and error counters while `RadioController` owns radio-specific state and commands. This matches the narrative split between generic driver families and radio-controller families.

Alternative considered:

- Collapse UART and radio behavior into one component. Rejected because it would erase the separation required by the formal baseline.

### Decision: Model pass windows entirely inside `CommController`

`CommController` will manage active band, pass-active state, remaining seconds, and total passes using a scheduled decrement model. That keeps pass state testable without a full deployment clock hierarchy.

Alternative considered:

- Defer pass-window logic until deployment integration. Rejected because pass-state visibility is already part of the formal public contract.

## Risks / Trade-offs

- **The host radio protocol is intentionally simple** -> Document it as a first-slice validation protocol only and leave hardware-specific adapters to later changes.
- **PTY support differs slightly across host OSes** -> Use POSIX PTY APIs that are available on the current macOS development environment and record real UART as `Blocked-HW`.
- **The comm slice is not yet wired into the deployment com stack** -> Keep the change scoped to controller, driver, and host-validation boundaries and mark deployment integration as deferred.

## Migration Plan

1. Add the OpenSpec proposal, design, tasks, and delta specs for `comm-subsystem-v1`.
2. Add the shared comm host transport and mock radio helpers under `simulators/comm/`.
3. Add `CommController`, `RadioController`, and `UartDriver` plus their unit tests.
4. Add a host integration test that exercises both TCP mock and PTY-backed transport paths.
5. Run the normal build, UT build, and `fprime-util check --all`.
6. Record evidence under `evidence/records/comm-subsystem-v1/`.
7. Validate and archive the change.

## Open Questions

None that block the first comm slice. Full deployment wiring and real hardware adapter selection remain for later changes.
